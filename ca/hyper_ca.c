/*
 * Copyright (c) 2014, Linaro Limited
 * Copyright (c) 2016 GlobalLogic
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <tee_client_api.h>
#include <sys/stat.h>
#include <zlib.h>

#define TA_NAME	"hyper.ta"

#define HYPER_UUID \
		{ 0xd96a5b40, 0xe2c7, 0xb1af, \
			{ 0x87, 0x94, 0x10, 0x02, 0xa5, 0xd5, 0xc7, 0x1f } }


#define HYPER_CMD_INIT_DRV		0
#define HYPER_CMD_READ			1
#define HYPER_CMD_WRITE			2
#define HYPER_CMD_ERASE			3

#define HYPER_FN "hyper.bin"

/* sector size */

#define SECTOR_SIZE			0x00040000U

enum {
	IMG_PARAM,
	IMG_IPL2,
	IMG_CERT,
	IMG_BL31,
	IMG_OPTEE,
	IMG_UBOOT,
	IMG_SSTDATA,
};

typedef struct img_param {
	uint32_t img_id;
	const char *img_name;
	uint32_t flash_addr;
	uint32_t size;
	uint8_t *buf;
}img_param_t;

/*We can flash 7 images*/
#define MAX_IMAGES 7

/*Set legal address and maximum size for every image*/
static struct img_param img_params[MAX_IMAGES] = {
		{.img_id = IMG_PARAM, .img_name = "PARAM",
		.flash_addr = 0, .size =1*SECTOR_SIZE,},
		{.img_id = IMG_IPL2, .img_name = "BL2",
		.flash_addr = 0x40000, .size =1*SECTOR_SIZE,},
		{.img_id = IMG_CERT, .img_name = "CERT",
		.flash_addr = 0x180000, .size =1*SECTOR_SIZE,},
		{.img_id = IMG_BL31, .img_name = "BL31",
		.flash_addr = 0x1C0000, .size =1*SECTOR_SIZE,},
		{.img_id = IMG_OPTEE, .img_name = "OPTEE",
		.flash_addr = 0x200000, .size =2*SECTOR_SIZE,},
		{.img_id = IMG_UBOOT, .img_name = "UBOOT",
		.flash_addr = 0x640000, .size =4*SECTOR_SIZE,},
		{.img_id = IMG_SSTDATA, .img_name = "SSTDATA",
		.flash_addr = 0x300000, .size =4*SECTOR_SIZE,},
};

int read_hyper_flash(uint32_t start_addr, uint32_t size, const char *hyper_fn);
int write_hyper_flash(struct img_param *img);
int erase_hyper_flash(struct img_param *img);
void usage(const char *name);
int write_boot(struct img_param *img, const char *fname);
int erase_data(struct img_param *img);

#define MAX_SIZE	0x4000000
int read_hyper_flash(uint32_t start_addr, uint32_t size, const char *hyper_fn)
{
	uint32_t sectors = size/SECTOR_SIZE, i;
//	uint32_t rest = size%SECTOR_SIZE;
	TEEC_Result ret;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = HYPER_UUID;
	uint32_t err, wsize;
	FILE *hyper;
	uint8_t *buf = NULL;

	if(start_addr %SECTOR_SIZE) {
		printf("Error: Start address has to be aligned to sector size(0x%x)!\n", SECTOR_SIZE);
		return -1;
	}

	if(size + start_addr> MAX_SIZE) {
		printf("Error: Requested Size is over HyperFlash Limits!\n");
		return -1;
	}


	if (size%SECTOR_SIZE)
		sectors++;


	hyper = fopen(hyper_fn, "wb");
	if (!hyper ) {
		printf("Can't create file(%s)\n", hyper_fn);
		return -1;
	}

	buf = malloc(size < SECTOR_SIZE ? size: SECTOR_SIZE);
	if (buf == NULL) {
			printf("Memory allocate error\n");
			return -1;
	}

	/* Initialize a context connecting us to the TEE */
	ret = TEEC_InitializeContext(NULL, &ctx);
	if (ret != TEEC_SUCCESS) {
		printf( "TEEC_InitializeContext failed with code 0x%x", ret);
		return (int) ret;
	}

	ret = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err);
	if (ret != TEEC_SUCCESS) {
		printf("TEEC_Opensession failed with code 0x%x Error: 0x%x", ret, err);
		TEEC_FinalizeContext(&ctx);
		return (int) ret;
	}

/*	sesion open, prepare parameters*/

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);

	/* send init flash*/
	ret = TEEC_InvokeCommand(&sess, HYPER_CMD_INIT_DRV, &op,
				 &err);
	if (ret == TEEC_SUCCESS) {
		for(i=0;i<sectors;i++) {
			printf( "size = %d, sectors = %d\n", size, sectors);
			memset(&op, 0, sizeof(op));
			op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
				TEEC_NONE, TEEC_NONE, TEEC_MEMREF_TEMP_OUTPUT);

			op.params[0].value.a = start_addr+i*SECTOR_SIZE;
			op.params[0].value.b =  size < SECTOR_SIZE ? size: SECTOR_SIZE;

			op.params[3].tmpref.buffer = (void*)buf;
			op.params[3].tmpref.size = op.params[0].value.b;

			/*Invoke read command*/
			ret = TEEC_InvokeCommand(&sess, HYPER_CMD_READ, &op,
						 &err);
			if (ret != TEEC_SUCCESS) {
				printf("Read sector %d failed (0x%x) Error:0x%x", i, ret, err);
				break;
			}
			wsize = fwrite(buf, 1, op.params[3].tmpref.size, hyper);
			if (wsize != op.params[3].tmpref.size) {
				printf("File write error %d\n", wsize);
				break;
			}
			size -=op.params[0].value.b;
		}
	}
	fclose(hyper);
	free(buf);

	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);
	printf("File %s created in current folder\n", hyper_fn);
	return 0;
}

int write_hyper_flash( struct img_param *img)
{
	TEEC_Result ret;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = HYPER_UUID;
	uint32_t err;
	uint32_t crc = crc32(0L, Z_NULL, 0);


	/* Initialize a context connecting us to the TEE */
	ret = TEEC_InitializeContext(NULL, &ctx);
	if (ret != TEEC_SUCCESS) {
		printf( "TEEC_InitializeContext failed with code 0x%x", ret);
		return (int) ret;
	}

	ret = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err);
	if (ret != TEEC_SUCCESS) {
		printf("TEEC_Opensession failed with code 0x%x Error: 0x%x", ret, err);
		TEEC_FinalizeContext(&ctx);
		return (int) ret;
	}

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
					 TEEC_NONE, TEEC_MEMREF_TEMP_INOUT);

	op.params[0].value.a = img->flash_addr;
	op.params[0].value.b = img->size;
	op.params[1].value.a = crc32(crc, img->buf, img->size);
	op.params[1].value.b = img->img_id;
	op.params[3].tmpref.buffer = img->buf;
	op.params[3].tmpref.size   = img->size;
	/*Invoke read command*/
	ret = TEEC_InvokeCommand(&sess, HYPER_CMD_WRITE, &op,
				 &err);
	if (ret != TEEC_SUCCESS) {
		printf("Writing failed with code 0x%x Error: 0x%x", ret, err);
	}

	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);
	return ret;
}

int erase_hyper_flash( struct img_param *img)
{
	TEEC_Result ret;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = HYPER_UUID;
	uint32_t err;


	/* Initialize a context connecting us to the TEE */
	ret = TEEC_InitializeContext(NULL, &ctx);
	if (ret != TEEC_SUCCESS) {
		printf( "TEEC_InitializeContext failed with code 0x%x", ret);
		return (int) ret;
	}

	ret = TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err);
	if (ret != TEEC_SUCCESS) {
		printf("TEEC_Opensession failed with code 0x%x Error: 0x%x", ret, err);
		TEEC_FinalizeContext(&ctx);
		return (int) ret;
	}

	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
					 TEEC_NONE, TEEC_NONE);

	op.params[0].value.a = img->flash_addr;
	op.params[0].value.b = img->size;
	op.params[1].value.b = img->img_id;
	/*Invoke read command*/
	ret = TEEC_InvokeCommand(&sess, HYPER_CMD_ERASE, &op,
				 &err);
	if (ret != TEEC_SUCCESS) {
		printf("Writing failed with code 0x%x Error: 0x%x", ret, err);
	}

	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);
	return ret;

}

#define FLASH_DATA_READ_BYTE_COUNT_8	8U
int write_boot(struct img_param *img, const char *fname)
{
	struct stat st;
	int ret;
	uint8_t *buf = NULL;
	FILE *boot;
	off_t size;

	ret = stat(fname, &st);
	if(ret) {
		printf("Error: File access denied (%s)\n", fname);
		return TEEC_ERROR_GENERIC;
	}

	if (st.st_size > img->size) {
		printf("Error: File size overflow (%ld)\n", st.st_size);
		return TEEC_ERROR_GENERIC;
	}
	img->size = st.st_size;
	/*This is to handle case that hyperflash driver need size aligned to 8*/
	if (img->size % FLASH_DATA_READ_BYTE_COUNT_8)
		img->size += (FLASH_DATA_READ_BYTE_COUNT_8-
					img->size %FLASH_DATA_READ_BYTE_COUNT_8);
	buf = malloc(img->size);
	if(!buf) {
		printf("Error:Memory allocation failed\n");
		return TEEC_ERROR_GENERIC;
	}
	memset(buf, 0xFF,img->size);
	boot = fopen(fname, "rb");
	if (!boot ) {
		printf("%s file open error\n", fname);
		free(buf);
		return TEEC_ERROR_GENERIC;
	} else {
		size = fread(buf, 1U, (size_t)st.st_size, boot);
		fclose(boot);

		if (size != st.st_size) {
			printf("File read error %ld\n", size);
			free(buf);
			return TEEC_ERROR_GENERIC;
		}
	}
	img->buf = buf;
	printf("Write %s, addr = 0x%x, size = %ld\n", img->img_name, img->flash_addr, size);
	ret = write_hyper_flash(img);
	free(buf);
	return ret;
}

int erase_data(struct img_param *img)
{
	/*This is to handle case that hyperflash driver need size aligned to 8*/
	if (img->size % FLASH_DATA_READ_BYTE_COUNT_8)
		img->size += (FLASH_DATA_READ_BYTE_COUNT_8 -
					img->size % FLASH_DATA_READ_BYTE_COUNT_8);
	img->buf = NULL;
	printf("Erase %s, addr = 0x%x, size = %du\n", img->img_name, img->flash_addr, img->size);
	return erase_hyper_flash(img);
}

#define U_BOOT_SIZE (2*SECTOR_SIZE)
#define FLASH_READ "-r"
#define FLASH_WRITE "-w"
#define FLASH_ERASE "-e"

void usage(const char *name)
{
	printf("usage for read: %s -r  <flash_addr> <size> [<file>]\n", name);
	printf("read address has to be aligned to HyperFlash sector size (0x40000)\n");

	printf("\nusage for write: hyper_ca -w <Image ID> [<file>]\n");
	printf("Image IDs:PARAM, BL2, CERT, BL31, OPTEE, UBOOT, SSTDATA\n");
	printf("Only binary files(*.bin) supported\n");
	printf("%s -w PARAM ./bootparam_sa0.bin\n", name);
	printf("%s -w CERT ./cert_header_sa6.bin\n", name);
	printf("%s -w BL2 ./bl2.bin\n", name);
	printf("%s -w BL31 ./bl31.bin\n", name);
	printf("%s -w UBOOT ./u-boot.bin\n", name);

	printf("\nusage for erase: %s -e <Image ID>\n", name);
	printf("%s -e SSTDATA\n", name);
}

int main(int argc, char *argv[])
{
	const char *fname;
	char *strptr;
	uint32_t size, addr, cmd = 0;
	int i, ret = TEEC_ERROR_GENERIC;

	if (argc == 1) {
		usage(argv[0]);
		exit (TEEC_ERROR_GENERIC);
	}

	if (!strncmp(argv[1], FLASH_READ, sizeof(FLASH_READ))) {
		cmd = HYPER_CMD_READ;
	} else if (!strncmp(argv[1], FLASH_WRITE, sizeof(FLASH_WRITE))) {
		cmd = HYPER_CMD_WRITE;
	} else if (!strncmp(argv[1], FLASH_ERASE, sizeof(FLASH_ERASE))) {
		cmd = HYPER_CMD_ERASE;
	}

	switch(cmd) {
		case HYPER_CMD_ERASE:
			if (argc!=3)
				break;
			for(i=0; i<MAX_IMAGES; i++) {
				/*Search for image by name*/
				if (!strncmp (argv[2], img_params[i].img_name,
					strlen(img_params[i].img_name))) {
						ret = erase_data(&img_params[i]);
						exit(ret);
				}
			}
			printf("Erase error: Image ID is wrong(%s)\n", argv[2]);
			break;

		case HYPER_CMD_WRITE:
			if (argc!=4)
				break;
			fname = argv[3];
			for(i=0; i<MAX_IMAGES; i++) {
				/*Search for image by name*/
				if (!strncmp (argv[2], img_params[i].img_name,
					strlen(img_params[i].img_name))) {
						ret = write_boot(&img_params[i], fname);
						exit(ret);
				}
			}
			printf("Write error: Image ID is wrong(%s)\n", argv[2]);
			break;

		case HYPER_CMD_READ:
			if (argc == 5)
				fname = argv[argc-1];
			else if (argc == 4)
				fname = HYPER_FN;
			else
				break;

			addr = strtol(argv[2],&strptr, 0);
			size = strtol(argv[3],&strptr, 0);
			printf("Reading Flash Address: 0x%x, Size: 0x%x\n", addr, size);
			ret = read_hyper_flash(addr, size, fname);
	}
	if (ret) {
		usage(argv[0]);
		exit(TEEC_ERROR_GENERIC);
	}
}
