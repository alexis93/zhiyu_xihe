/*-----------------------------------------------------------------
 *	iwise_ublox.c :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#include <termios.h>
#include <fcntl.h>

#include "../iwise_loc_hardware.h"
#include "iwise_loc_base/iwise_loc_base.h"
#include "iwise_loc_base/iwise_loc_log.h"

 /*
 *	参数		:	unsigned char msg[]			I	消息
 * 				int len						I	消息长度
 * 	返回		:	void
 * 	描述		:	生成ublox命令的校验和和头
 * 	历史		:
 * 	备注		: 
 */
static void gen_msg(unsigned char msg[],int len)
{
    msg[0]=0xB5;
    msg[1]=0x62;
    int i=2;
    for(; i<len-2; i++)
    {
        msg[len-2]+=msg[i];
        msg[len-1]+=msg[len-2];
    }
    msg[len-2] &= 0xFF;
    msg[len-1] &= 0xFF;
}

 /*
 *	参数		:	int serial_fd		I		串口描述符
 * 	返回		:	int		0:写入成功	-1;写入失败
 * 	描述		:	向ublox发送命令
 * 	历史		:
 * 	备注		: 
 */
static int send_cmd2ublox(int serial_fd) 
{
    /*在ublox里面默认ublox是输出nmea格式的数据，因此在设置是
		要关掉除gga和gsv以外的数据，同时开启输出原始观测数据和星厉 */
    int ret = -1;

    /*关闭TXT*/
	unsigned char msg1[] = { 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x08,
			0x00, 0x00, 0x00 };
	gen_msg(msg1, 11);
	ret = write(serial_fd, msg1, 11);
    if(ret < 0)
    {
        return -1;
    }

	/*关闭GSA*/
	unsigned char msg2[] = { 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x02,
			0x00, 0x00, 0x00 };
	gen_msg(msg2, 11);
	ret = write(serial_fd, msg2, 11);
    if(ret < 0)
    {
        return -1;
    }

	/*关闭RMC*/
	unsigned char msg3[] = { 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x04,
			0x00, 0x00, 0x00 };
	gen_msg(msg3, 11);
	ret = write(serial_fd, msg3, 11);
    if(ret < 0)
    {
        return -1;
    }

#if 0
	/*关闭GGA*/ 
	 unsigned char msg4[] = { 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x00,
	 0x00, 0x00, 0x00 };
	 gen_msg(msg4, 11);
	 ret = write(serial_fd, msg4, 11);
     if(ret < 0)
     {
        return -1;
     }
#endif

	/*关闭GLL*/
	unsigned char msg5[] = { 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x01,
			0x00, 0x00, 0x00 };
	gen_msg(msg5, 11);
	ret = write(serial_fd, msg5, 11);
    if(ret < 0)
     {
        return -1;
     }

#if 0
	/*关闭GSV*/
	 unsigned char msg6[] = { 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x01,
	 0x00, 0x00, 0x00 };
	 gen_msg(msg6, 11);
	 ret = write(serial_fd, msg6, 11);
     if(ret < 0)
     {
        return -1;
     }
#endif

	/*关闭VTG*/
	unsigned char msg7[] = { 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x05,
			0x00, 0x00, 0x00 };
	gen_msg(msg7, 11);
	ret = write(serial_fd, msg7, 11);
    if(ret < 0)
     {
        return -1;
     }

     /*开启原始观测数据*/
	unsigned char msg8[] = { 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x02, 0x10,
			0x01, 0x00, 0x00 };
	gen_msg(msg8, 11);
	ret = write(serial_fd, msg8, 11);
    if(ret < 0)
     {
        return -1;
     }

    /*开启卫星星历*/
	unsigned char msg9[] = { 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x02, 0x11,
			0x01, 0x00, 0x00 };
	gen_msg(msg9, 11);
	ret = write(serial_fd, msg9, 11);
	if(ret < 0)
	 {
		return -1;
	 }

     return 0;
}

/*
 *	参数		:	void
 * 	返回		:	int		>0:打开设备成功；	-1:打开设备失败
 * 	描述		:	初始化串口并向gps芯片发送命令
 * 	历史		:
 * 	备注		: 
 */
extern int open_ubx()
{
	/*TODO 打开串口，发送初始化命令*/
	int serial_fd;
	/* 默认打开ublox设备是串口1（大的开发板）*/
#ifdef SMALL
	//serial_fd = open("/dev/s3c2410_serial2", O_RDWR);
	serial_fd = open("/dev/ttySAC2", O_RDWR);
#else
	//serial_fd = open("/dev/s3c2410_serial1", O_RDWR);
	serial_fd = open("/dev/ttyAMA2", O_RDWR);
#endif
	if (serial_fd == -1) {
		return -1;
	}

	/*设置参数 */
	struct termios cfg;
	if (tcgetattr(serial_fd, &cfg)) {
		close(serial_fd);
		return -1;
	}
	cfmakeraw(&cfg);
	
	//cfsetispeed(&cfg, B115200);
	//cfsetospeed(&cfg, B115200);

	cfsetispeed(&cfg, B9600);
	cfsetospeed(&cfg, B9600);

	if (tcsetattr(serial_fd, TCSANOW, &cfg)) {
		close(serial_fd);
		return -1;
	}
	send_cmd2ublox(serial_fd);
	return serial_fd;
}

 /*
 *	参数		:	int	handle		I		串口描述符
 * 	返回		:
 * 	描述		:	关闭gps芯片描述符
 * 	历史		:
 * 	备注		: 	参考linux系统的close
 */
extern int close_ubx(int handle)
{
	return close(handle);
}

