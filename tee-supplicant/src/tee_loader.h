/*
 * Copyright (c) 2016 GlobalLogic
 */
#ifndef _TEE_LOADER_H_
#define _TEE_LOADER_H_

#define MODNUMS	2
struct modparams {
	const char *module;
};

#define OPTEE "/sbin/optee.ko"
#define OPTEE_TZ "/sbin/optee_armtz.ko"

/*OPTEE modules loading*/
int tee_load_modules(struct modparams *modules);

#endif
