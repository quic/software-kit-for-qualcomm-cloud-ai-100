// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QOSAL_UTILS_H
#define QOSAL_UTILS_H

#include <cstdlib>
#include <sys/mman.h>
#include <sys/stat.h>

namespace qaic {
namespace QOsal {
std::size_t strlcpy(char *dst, const char *src, std::size_t size);
std::size_t strlcat(char *dst, const char *src, std::size_t size);
int shm_open(const char *name, int oflag, mode_t mode);
void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           off_t offset);
int munmap(void *addr, size_t length);
}
}

#endif // QOSAL_UTILS_H