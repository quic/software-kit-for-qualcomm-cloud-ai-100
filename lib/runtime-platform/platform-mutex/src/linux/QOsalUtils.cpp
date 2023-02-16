// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QOsalUtils.h"
#include <cstring>
#include <algorithm>
#include <sys/mman.h>
#include "QLogger.h"

namespace qaic {
namespace QOsal {
// The strlcpy() and strlcat() functions return the total length of the string
// they tried to create. For strlcpy() that means the length of src. For
// strlcat() that means the initial length of dst plus the length of src. While
// this may seem somewhat confusing, it was done to make truncation detection
// simple.
// Note however, that if strlcat() traverses size characters without finding a
// NUL, the length of the string is considered to be size and the destination
// string will not be NUL-terminated (since there was no space for the NUL).
// This keeps strlcat() from running off the end of a string. In practice this
// should not happen (as it means that either size is incorrect or that dst is
// not a proper "C" string). The check exists to prevent potential security
// problems in incorrect code.
size_t strlcat(char *dest, const char *src, size_t size) {
  // Detect cases where size is less than or equal to original length
  size_t destLen = std::strlen(dest) > size ? size : std::strlen(dest);
  size_t srcLen = std::strlen(src);
  size_t sizeToCopy = size - destLen > srcLen ? srcLen : size - destLen - 1;

  if (destLen >= size) {
    // return the total length of the string it tried to create
    return size + srcLen; // Original string remains unchanged.
  }

  std::copy_n(src, sizeToCopy, dest + destLen);
  *(dest + destLen + sizeToCopy) = '\0';

  // Return length of string that it tried to build. Helps in detecting
  // truncation.
  return destLen + srcLen;
}

// Copy string until null termination,upto destSize - 1
// destSize should account for null termination byte
size_t strlcpy(char *dest, const char *src, size_t destSize) {
  size_t len = std::strlen(src) + 1;
  size_t sizeToCopy = len > destSize ? destSize : len;
  // If the src string is NULL or destination is null
  if (sizeToCopy == 0) {
    return 0;
  }
  std::copy_n(src, sizeToCopy, dest);
  *(dest + (sizeToCopy - 1)) = '\0';
  return len - 1; // The strlcpy() function return the total length of the
                  // string they tried to create. That means the length of
                  // src.
}

int shm_open(const char *name, int oflag, mode_t mode) {
  return ::shm_open(name, oflag, mode);
}

int munmap(void *addr, size_t length) { return ::munmap(addr, length); }

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
           off_t offset) {
  return ::mmap(addr, length, prot, flags, fd, offset);
}
}
}
