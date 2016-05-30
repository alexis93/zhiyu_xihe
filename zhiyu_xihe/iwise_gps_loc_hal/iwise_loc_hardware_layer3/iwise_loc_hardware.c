/*-----------------------------------------------------------------
 *	iwise_loc_hardware.c :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "iwise_loc_hardware.h"
#include "iwise_loc_base/Parserlib/rtklib.h"

/*
 *	参数		:	int DEVICE		I		gps芯片设备号
 * 	返回		:	int		0:	打开失败，没有该设备	-1:打开失败	>0:打开成功,返回文件描述符
 * 	描述		:	打开gps芯片设备
 * 	历史		:
 * 	备注		: 	参考linux系统的open
 */
int	iwise_hardware_open(int DEVICE)
{
    switch(DEVICE) {
        case STRFMT_UBX :   return open_ubx();
        case STRFMT_OEM4:   return open_oem4();    
    }

    return 0;
}

/*
 *	参数		:	int DEVICE		I		gps芯片设备号
 * 				int handle		I		gps设备描述符
 * 	返回		:	int
 * 	描述		:	关闭gps芯片设备
 * 	历史		:
 * 	备注		: 	参考linux系统的close
 */
int iwise_hardware_close(int DEVICE, int handle)
{
    switch(DEVICE) {
        case STRFMT_UBX :   return close_ubx(handle);
        case STRFMT_OEM4:   return close_oem4(handle);
    }

    return 0;
}

/*
 *	参数		:	int DEVICE		I		gps芯片设备号
 * 				int handle		I		gps设备描述符
 * 				void* buf		I		读取数据的缓冲区
 * 				size_t count	I		读取数据的缓冲区的字符个数
 * 	返回		:
 * 	描述		:	读取gps芯片的数据
 * 	历史		:
 * 	备注		: 	参考linux系统read
 */
ssize_t	iwise_hardware_read(int DEVICE, int handle, void* buf, size_t count)
{
    return read(handle, buf, count);
}

/*
 *	参数		:	int DEVICE		I		gps芯片设备号
 * 				int handle		I		gps设备描述符
 * 				void* buf		I		写入数据的缓冲区
 * 				size_t count	I		写入数据的缓冲区的字符个数
 * 	返回		:
 * 	描述		:	向gps芯片写入数据
 * 	历史		:
 * 	备注		: 	参考linux系统write
 */
ssize_t iwise_hardware_write(int DEVICE, int handle, const void* buf, size_t count)
{
    return write(handle, buf, count);
}


/*
 *	参数		:	int DEVICE				I		gps芯片设备号
 * 				int handle				I		gps设备描述符
 * 				unsigned long request	I		向gps芯片写入的命令字
 * 				void* buf				I		写入数据的缓冲区
 * 	返回		:
 * 	描述		:	向gps芯片发命令
 * 	历史		:
 * 	备注		: 	参考linux系统ioctl
 */
int	iwise_hardware_ioctl(int DEVICE, int handle, unsigned long request, void* buf)
{
    return ioctl(handle, request, buf);
}
