/*
 * Copyright (c) 2016 GlobalLogic
 */

#include <errno.h>
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

#define INSMOD(fd, opts) syscall(__NR_finit_module, fd, opts, 0)

#define  OPTEE_DEV_FILE "/dev/opteearmtz00"

#define MOD_LOAD_TO 5

int tee_load_modules(struct modparams *modules)
{
	int fd;
	struct stat st;
	int ret, i;

	for (i = 0; i < MODNUMS; i++) {
		ret = stat(modules[i].module, &st);
		if(ret) {
			printf("Error: File access denied (%s)\n", modules[i].module);
			return ERR_GENERIC;
		}

		fd = open(modules[i].module, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
		if (fd == -1) {
			printf("%s file open error\n", modules[i].module);
			return ERR_GENERIC;
		} else {
			/*
			 * Insert kernel module. Continue if module is already inserted.
			 */
			if ((INSMOD(fd, "") == -1) && (errno != EEXIST)) {
				printf("Failed to insert (%s) module, error = (%d)\n",
						modules[i].module, errno);
				close(fd);
				return ERR_GENERIC;
			}
			close(fd);
		}
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

