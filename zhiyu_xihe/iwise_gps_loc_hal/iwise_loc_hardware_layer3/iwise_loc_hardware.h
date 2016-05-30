/*-----------------------------------------------------------------
 *	iwise_loc_hardware.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#ifndef _IWISE_LOC_HARDWARE_H_
#define _IWISE_LOC_HARDWARE_H_

#include <unistd.h>

/*
 * iwise_hardware_ioctl 的命令码
 * 为区域定位留出接口
 * */
#define		IWISE_HARDWARE_IOCTL_INJECT_TIME				0X00000001
#define		IWISE_HARDWARE_IOCTL_INJECT_POSITION			0X00000002
#define		IWISE_HARDWARE_IOCTL_INJECT_ASSIST_DATA			0X00000003
#define		IWISE_HARDWARE_IOCTL_INJECT_EXTERN_DATA			0X00000004
#define		IWISE_HARDWARE_IOCTL_DELETE_ASSIST_DATA			0X00000004

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * 硬件访问层的五个接口
 * open/close/read/write/iotcl
 * 每个函数都带 DEVICE 参数表明定位芯片的类型
 * */
int		iwise_hardware_open(int DEVICE);
int 	iwise_hardware_close(int DEVICE, int handle);
ssize_t	iwise_hardware_read(int DEVICE, int handle, void* buf, size_t count);
ssize_t	iwise_hardware_write(int DEVICE, int handle, const void* buf, size_t count);
int		iwise_hardware_ioctl(int DEVICE, int handle, unsigned long request, void* buf);

/*
 * novatel 芯片的处理函数
 * */
int		open_oem4();
int		close_oem4(int handle);
int		ioctl_oem4(int handle, unsigned long request, void* buf);


/*
 * ublox 芯片的处理函数
 * */
int		open_ubx();
int		close_ubx(int handle);
int		ioctl_ubx(int handle, unsigned long request, void* buf);

#ifdef __cplusplus
}
#endif

#endif
