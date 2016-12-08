/*
 * Copyright (c) 2016 GlobalLogic
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <cutils/properties.h>
#include "tee_loader.h"

#define ERR_GENERIC (-1)

#define INSMOD(mod, len, opts) syscall(__NR_init_module, mod, len, opts)

#define  OPTEE_DEV_FILE "/dev/opteearmtz00"

#define MOD_LOAD_TO 5

int tee_load_modules(struct modparams *modules)
{
	FILE *mod;
	char *buf = NULL;
	struct stat st;
	int ret, i;
	off_t size;

	for (i = 0; i < MODNUMS; i++) {
		ret = stat(modules[i].module, &st);
		if(ret) {
			printf("Error: File access denied (%s)\n", modules[i].module);
			return ERR_GENERIC;
		}

		buf = malloc(st.st_size);
		if(!buf) {
			printf("Error:Memory allocation failed (%ld)\n", st.st_size);
			return ERR_GENERIC;
		}

		mod = fopen(modules[i].module, "rb");
		if (!mod ) {
			printf("%s file open error\n", modules[i].module);
			free(buf);
			return ERR_GENERIC;
		} else {
			size = fread(buf, 1U, (size_t)st.st_size, mod);
			fclose(mod);
			if (size != st.st_size) {
				printf("File read error %ld\n", size);
				free(buf);
				return ERR_GENERIC;
			}
			if (INSMOD(buf, size, "")) {
				printf("Moduile insert error (%s)\n", modules[i].module);
				free(buf);
				return ERR_GENERIC;
			}
		}
		free(buf);
		buf = NULL;
	}
	i = 0;
	do {
		/*Check if optee device file created*/
		ret = stat(OPTEE_DEV_FILE, &st);
		if (!ret)
			break;
		sleep(1);
	} while (++i<MOD_LOAD_TO);
	if (!ret)
		return 0;
	else
		return ERR_GENERIC;
}

