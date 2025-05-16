// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef AIC_METADATA_READER
#define AIC_METADATA_READER

#include "AICMetadata.h"

// Client is responsible for allocating/deallocating buffer \p buff and copying
// metadata blob into buffer.  The \p buffSizeInBytes argument is used for
// checking out-of-bounds accesses.
//
// The return value is a AICMetadata* that can be directly used by the
// client.  After this call the AICMetadata pointers have been updated
// based on the in-memory location of the buffer.  This function should only be
// called once.
#ifdef __cplusplus
extern "C" {
#endif
AICMetadata *MDR_readMetadata(void *buff, int buffSizeInBytes, char *errBuff,
                              int errBuffSz);
#ifdef __cplusplus
} // End of extern "C"
#endif

#endif // AIC_METADATA_READER
