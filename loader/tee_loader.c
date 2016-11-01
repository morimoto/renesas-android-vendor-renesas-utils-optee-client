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

#define ERR_GENERIC (-1)

#define INSMOD(mod, len, opts) syscall(__NR_init_module, mod, len, opts)

#define OPTEE "/sbin/optee.ko"
#define OPTEE_TZ "/sbin/optee_armtz.ko"
#define SUPPLICANT "/sbin/tee-supp"

struct modparams {
	const char *module;
};
#define MODNUMS	2
static const struct modparams modules[MODNUMS] = {
				{OPTEE}, {OPTEE_TZ}};
int main()
{
	FILE *mod;
	char *buf = NULL;
	struct stat st;
	int ret, i;
	off_t size;
	pid_t child, sid;

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

	/*
	*By here we suppose to have optee.ko and optee_armtz.ko loaded
	* Load and demonize TEE supplicant.
	*/

	if ((child = fork()) == 0) {
		/*We are in child process
		* Create sid to detach from parent;
		*/
		sid = setsid();
		if (sid < 0) {
			perror("Can't create session for supplicant");
			exit(ERR_GENERIC);
		}
		execl(SUPPLICANT, "tee-supp", NULL, NULL);
		/*Should not return*/
		exit(ERR_GENERIC);
	}

	exit (0);
}


