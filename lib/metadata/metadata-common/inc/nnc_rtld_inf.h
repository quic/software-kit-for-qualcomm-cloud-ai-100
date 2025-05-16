// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
/** @file nnc_rtld_inf
 *  @brief NNC RTLD INTERFACE to Network Header file
 */
#ifndef NNC_RTLD_INF_H_
#define NNC_RTLD_INF_H_

/*
 * Values for dlopen `flags'
 */
#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_GLOBAL 0x100 /* Allow global searches in object */
#define RTLD_LOCAL 0x200

/*
 * Special handle arguments for dlsym()
 */
#define RTLD_NEXT ((void *)-1)    /* Search subsequent objects */
#define RTLD_DEFAULT ((void *)-2) /* Use default search algorithm */
#define RTLD_SELF ((void *)-3)    /* Search the caller itself */

/*
 * type of requests supported by dlinfo()
 */
/* returns load address of module, p returns int */
#define RTLD_DI_LOAD_ADDR 3
/* returns mapped size of module, p returns int */
#define RTLD_DI_LOAD_SIZE 4

/*
 * symbol info returned by dladdr
 */
typedef struct _dl_info {
  const char *dli_fname; /* File defining the symbol */
  void *dli_fbase;       /* Base address */
  const char *dli_sname; /* Symbol name */
  const void *dli_saddr; /* Symbol address */
} Dl_info;

/* Function pointers for calling RTLD functions*/
typedef void *(*dlopen_fp)(const char *name, int flags);
typedef void *(*dlopenbuf_fp)(const char *name, const char *buf, int len,
                              int flags);
typedef int (*dlclose_fp)(void *handle);
typedef void *(*dlsym_fp)(void *__restrict handle, const char *__restrict name);
typedef int (*dladdr_fp)(const void *__restrict addr, Dl_info *__restrict info);
typedef char *(*dlerror_fp)(void);
typedef int (*dlinfo_fp)(void *__restrict handle, int request,
                         void *__restrict p);

#endif /* NNC_RTLD_INF_H_ */
