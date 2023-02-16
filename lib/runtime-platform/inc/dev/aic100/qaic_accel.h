/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
 *
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef QAIC_ACCEL_H_
#define QAIC_ACCEL_H_

#include <linux/ioctl.h>
#include <linux/types.h>
#include <drm/drm.h>

#if defined(__CPLUSPLUS)
extern "C" {
#endif

#define QAIC_MANAGE_MAX_MSG_LENGTH 0x1000	/**<
						  * The length(4K) includes len and
						  * count fields of qaic_manage_msg
						  */

enum qaic_sem_flags {
	SEM_INSYNCFENCE =	0x1,
	SEM_OUTSYNCFENCE =	0x2,
};

enum qaic_sem_cmd {
	SEM_NOP =		0,
	SEM_INIT =		1,
	SEM_INC =		2,
	SEM_DEC =		3,
	SEM_WAIT_EQUAL =	4,
	SEM_WAIT_GT_EQ =	5, /**< Greater than or equal */
	SEM_WAIT_GT_0 =		6, /**< Greater than 0 */
};

enum qaic_manage_transaction_type {
	TRANS_UNDEFINED =			0,
	TRANS_PASSTHROUGH_FROM_USR =		1,
	TRANS_PASSTHROUGH_TO_USR =		2,
	TRANS_PASSTHROUGH_FROM_DEV =		3,
	TRANS_PASSTHROUGH_TO_DEV =		4,
	TRANS_DMA_XFER_FROM_USR =		5,
	TRANS_DMA_XFER_TO_DEV =			6,
	TRANS_ACTIVATE_FROM_USR =		7,
	TRANS_ACTIVATE_FROM_DEV =		8,
	TRANS_ACTIVATE_TO_DEV =			9,
	TRANS_DEACTIVATE_FROM_USR =		10,
	TRANS_DEACTIVATE_FROM_DEV =		11,
	TRANS_STATUS_FROM_USR =			12,
	TRANS_STATUS_TO_USR =			13,
	TRANS_STATUS_FROM_DEV =			14,
	TRANS_STATUS_TO_DEV =			15,
	TRANS_TERMINATE_FROM_DEV =		16,
	TRANS_TERMINATE_TO_DEV =		17,
	TRANS_DMA_XFER_CONT =			18,
	TRANS_VALIDATE_PARTITION_FROM_DEV =	19,
	TRANS_VALIDATE_PARTITION_TO_DEV =	20,
	TRANS_MAX =				21
};

struct qaic_manage_trans_hdr {
	__u32 type;	/**< in, value from enum manage_transaction_type */
	__u32 len;	/**< in, length of this transaction, including the header */
};

struct qaic_manage_trans_passthrough {
	struct qaic_manage_trans_hdr hdr;
	__u8 data[];	/**< in, userspace must encode in little endian */
};

struct qaic_manage_trans_dma_xfer {
	struct qaic_manage_trans_hdr hdr;
	__u32 tag;	/**< in, device specific */
	__u32 count;	/**< in */
	__u64 addr;	/**< in, address of the data to transferred via DMA */
	__u64 size;	/**< in, length of the data to transferred via DMA */
};

struct qaic_manage_trans_activate_to_dev {
	struct qaic_manage_trans_hdr hdr;
	__u32 queue_size;	/**<
				  * in, number of elements in DBC request
				  * and respose queue
				  */
	__u32 eventfd;		/**< in */
	__u32 options;		/**< in, device specific */
	__u32 pad;		/**< pad must be 0 */
};

struct qaic_manage_trans_activate_from_dev {
	struct qaic_manage_trans_hdr hdr;
	__u32 status;	/**< out, status of activate transaction */
	__u32 dbc_id;	/**< out, Identifier of assigned DMA Bridge channel */
	__u64 options;	/**< out */
};

struct qaic_manage_trans_deactivate {
	struct qaic_manage_trans_hdr hdr;
	__u32 dbc_id;	/**< in, Identifier of assigned DMA Bridge channel */
	__u32 pad;	/**< pad must be 0 */
};

struct qaic_manage_trans_status_to_dev {
	struct qaic_manage_trans_hdr hdr;
};

struct qaic_manage_trans_status_from_dev {
	struct qaic_manage_trans_hdr hdr;
	__u16 major;	/**< out, major vesrion of NNC protocol used by device */
	__u16 minor;	/**< out, minor vesrion of NNC protocol used by device */
	__u32 status;	/**< out, status of query transaction  */
	__u64 status_flags;	/**<
				  * out
				  * 0    : If set then device has CRC check enabled
				  * 1:63 : Unused
				  */
};

struct qaic_manage_msg {
	__u32 len;	/**< in, Length of valid data - ie sum of all transactions */
	__u32 count;	/**< in, Number of transactions in message */
	__u64 data;	/**< in, Pointer to array of transactions */
};

struct qaic_create_bo {
	__u64 size;	/**< in, Size of BO in byte */
	__u32 handle;	/**< out, Returned GEM handle for the BO */
	__u32 pad;	/**< pad must be 0 */
};

struct qaic_mmap_bo {
	__u32 handle;	/**< in, Handle for the BO being mapped. */
	__u32 pad;	/**< pad must be 0 */
	__u64 offset;	/**<
			  * out, offset into the drm node to use for
			  * subsequent mmap call
			  */
};

/**
 * @brief semaphore command
 */
struct qaic_sem {
	__u16 val;	/**< in, Only lower 12 bits are valid */
	__u8  index;	/**< in, Only lower 5 bits are valid */
	__u8  presync;	/**< in, 1 if presync operation, 0 if postsync */
	__u8  cmd;	/**< in, See enum sem_cmd */
	__u8  flags;	/**< in, See sem_flags for valid bits.  All others must be 0 */
	__u16 pad;	/**< pad must be 0 */
};

struct qaic_attach_slice_entry {
	__u64 size;		/**< in, Size memory to allocate for this BO slice */
	struct qaic_sem	sem0;	/**< in, Must be zero if not valid */
	struct qaic_sem	sem1;	/**< in, Must be zero if not valid */
	struct qaic_sem	sem2;	/**< in, Must be zero if not valid */
	struct qaic_sem	sem3;	/**< in, Must be zero if not valid */
	__u64 dev_addr;		/**< in, Address in device to/from which data is copied */
	__u64 db_addr;		/**< in, Doorbell address */
	__u32 db_data;		/**< in, Data to write to doorbell */
	__u32 db_len;		/**<
				  * in, Doorbell length - 32, 16, or 8 bits.
				  * 0 means doorbell is inactive
				  */
	__u64 offset;		/**< in, Offset from start of buffer */
};

struct qaic_attach_slice_hdr {
	__u32 count;	/**< in, Number of slices for this BO */
	__u32 dbc_id;	/**< in, Associate this BO with this DMA Bridge channel */
	__u32 handle;	/**< in, Handle of BO to which slicing information is to be attached */
	__u32 dir;	/**< in, Direction of data: 1 = DMA_TO_DEVICE, 2 = DMA_FROM_DEVICE */
	__u64 size;	/**<
			  * in, Total length of BO
			  * If BO is imported (DMABUF/PRIME) then this size
			  * should not exceed the size of DMABUF provided.
			  * If BO is allocated using DRM_IOCTL_QAIC_CREATE_BO
			  * then this size should be exactly same as the size
			  * provided during DRM_IOCTL_QAIC_CREATE_BO.
			  */
};

struct qaic_attach_slice {
	struct qaic_attach_slice_hdr hdr;
	__u64 data;	/**<
			  * in, Pointer to a buffer which is container of
			  * struct qaic_attach_slice_entry[]
			  */
};

struct qaic_execute_entry {
	__u32 handle;	/**< in, buffer handle */
	__u32 dir;	/**< in, 1 = to device, 2 = from device */
};

struct qaic_partial_execute_entry {
	__u32 handle;	/**< in, buffer handle */
	__u32 dir;	/**< in, 1 = to device, 2 = from device */
	__u64 resize;	/**< in, 0 = no resize */
};

struct qaic_execute_hdr {
	__u32 count;	/**< in, number of executes following this header */
	__u32 dbc_id;	/**< in, Identifier of assigned DMA Bridge channel */
};

struct qaic_execute {
	struct qaic_execute_hdr hdr;
	__u64 data;	/**< in, qaic_execute_entry or qaic_partial_execute_entry container */
};

struct qaic_wait {
	__u32 handle;	/**< in, handle to wait on until execute is complete */
	__u32 timeout;	/**< in, timeout for wait(in ms) */
	__u32 dbc_id;	/**< in, Identifier of assigned DMA Bridge channel */
	__u32 pad;	/**< pad must be 0 */
};

struct qaic_perf_stats_hdr {
	__u16 count;	/**< in, Total number BOs requested */
	__u16 pad;	/**< pad must be 0 */
	__u32 dbc_id;	/**< in, Identifier of assigned DMA Bridge channel */
};

struct qaic_perf_stats {
	struct qaic_perf_stats_hdr hdr;
	__u64 data;	/**< in, qaic_perf_stats_entry container */
};

struct qaic_perf_stats_entry {
	__u32 handle;			/**< in, Handle of the memory request */
	__u32 queue_level_before;	/**<
					  * out, Number of elements in queue
					  * before submission given memory request
					  */
	__u32 num_queue_element;	/**<
					  * out, Number of elements to add in the
					  * queue for given memory request
					  */
	__u32 submit_latency_us;	/**<
					  * out, Time taken by kernel to submit
					  * the request to device
					  */
	__u32 device_latency_us;	/**<
					  * out, Time taken by device to execute the
					  * request. 0 if request is not completed
					  */
	__u32 pad;			/**< pad must be 0 */
};

struct qaic_part_dev {
	__u32 partition_id;	/**< in, reservation id */
	__u16 remove;		/**< in, 1 - Remove device 0 - Create device */
	__u16 pad;		/**< pad must be 0 */
};

#define DRM_QAIC_MANAGE				0x00
#define DRM_QAIC_CREATE_BO			0x01
#define DRM_QAIC_MMAP_BO			0x02
#define DRM_QAIC_ATTACH_SLICE_BO		0x03
#define DRM_QAIC_EXECUTE_BO			0x04
#define DRM_QAIC_PARTIAL_EXECUTE_BO		0x05
#define DRM_QAIC_WAIT_BO			0x06
#define DRM_QAIC_PERF_STATS_BO			0x07
#define DRM_QAIC_PART_DEV			0x08

#define DRM_IOCTL_QAIC_MANAGE			DRM_IOWR(DRM_COMMAND_BASE + DRM_QAIC_MANAGE, struct qaic_manage_msg)
#define DRM_IOCTL_QAIC_CREATE_BO		DRM_IOWR(DRM_COMMAND_BASE + DRM_QAIC_CREATE_BO,	struct qaic_create_bo)
#define DRM_IOCTL_QAIC_MMAP_BO			DRM_IOWR(DRM_COMMAND_BASE + DRM_QAIC_MMAP_BO, struct qaic_mmap_bo)
#define DRM_IOCTL_QAIC_ATTACH_SLICE_BO		DRM_IOW(DRM_COMMAND_BASE + DRM_QAIC_ATTACH_SLICE_BO, struct qaic_attach_slice)
#define DRM_IOCTL_QAIC_EXECUTE_BO		DRM_IOW(DRM_COMMAND_BASE + DRM_QAIC_EXECUTE_BO,	struct qaic_execute)
#define DRM_IOCTL_QAIC_PARTIAL_EXECUTE_BO	DRM_IOW(DRM_COMMAND_BASE + DRM_QAIC_PARTIAL_EXECUTE_BO,	struct qaic_execute)
#define DRM_IOCTL_QAIC_WAIT_BO			DRM_IOW(DRM_COMMAND_BASE + DRM_QAIC_WAIT_BO, struct qaic_wait)
#define DRM_IOCTL_QAIC_PERF_STATS_BO		DRM_IOWR(DRM_COMMAND_BASE + DRM_QAIC_PERF_STATS_BO, struct qaic_perf_stats)
#define DRM_IOCTL_QAIC_PART_DEV			DRM_IOWR(DRM_COMMAND_BASE + DRM_QAIC_PART_DEV, struct qaic_part_dev)

#if defined(__CPLUSPLUS)
}
#endif

#endif /* QAIC_ACCEL_H_ */
