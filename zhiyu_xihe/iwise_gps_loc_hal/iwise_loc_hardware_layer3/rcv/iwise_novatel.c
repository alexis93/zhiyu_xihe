/*-----------------------------------------------------------------
 *	iwise_novatel.c :
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

 /*
 *	参数		:	int	serial_fd		I		串口描述符
 * 	返回		:	int	status（0：ok; -1:write error）
 * 	描述		:	向novatel发送命令
 * 	历史		:
 * 	备注		: 
 */
static int send_cmd2novatel(int serial_fd)
{
	int ret = -1;
#ifdef SINAN
	unsigned char msg0[] = "unlogall\r\n";
	unsigned char msg1[] = "log gpgga ontime 1\r\n";
	//unsigned char msg2[] = "log gpgsv ontime 1\r\n";
    //unsigned char msg0[] = "log gpgst ontime 1\r\n";
    unsigned char msg3[] = "log rangeb ontime 1\r\n"; 	/* 卫星观测数据 */
    //unsigned char msg4[] = "log rangecmpb ontime 1\r\n"; /* 卫星观测数据 */
    unsigned char msg5[] = "log rawephemb ontime 1\r\n"; 	/* 卫星的星历 */
    //unsigned char msg6[] = "log rawsbasframeb onnew\r\n";
    //unsigned char msg7[] = "log ionutcb onchanged\r\n";
    unsigned char msg8[] = "interfacemode com1 auto auto on\r\n";
#else	
	unsigned char msg1[] = "log gpgga ontime 1\n";
	unsigned char msg2[] = "log gpgsv ontime 1\n";
    unsigned char msg0[] = "log gpgst ontime 1\n";
    unsigned char msg3[] = "log rangeb ontime 1\n"; 	/* 卫星观测数据 */
    //unsigned char msg4[] = "log rangecmpb ontime 1\n"; /* 卫星观测数据 */
    unsigned char msg5[] = "log rawephemb ontime 1\n"; 	/* 卫星的星历 */
    unsigned char msg6[] = "log rawsbasframeb onnew\n";
    unsigned char msg7[] = "log ionutcb onchanged\n";
#endif
	ret = write(serial_fd, msg0, sizeof(msg0));
	if(ret < 0) {
		GPS_LOGE("send unlogall failed!\n");
		return -1;
	}
	
	ret = write(serial_fd, msg1, sizeof(msg1));
	if(ret < 0) {
		GPS_LOGE("send gsv to novatel failed!\n");
		return -1;
	}
#ifndef SINAN
	ret = write(serial_fd, msg2, sizeof(msg2));
	if(ret < 0) {
		GPS_LOGE("send gst to novatel failed!\n");
		return -1;
	}
#endif
	
	ret = write(serial_fd, msg3, sizeof(msg3));
	if(ret < 0) {
		GPS_LOGD("send rangeb to novatel failed!\n");
		return -1;
	}

/*
	ret = write(parser, msg4, sizeof(msg4));
	if(ret < 0) {
		GPS_LOGD("send rangecmpb to novatel failed!\n");
		return -1;
	}
*/	
	ret = write(serial_fd, msg5, sizeof(msg5));
	if(ret < 0) {
		GPS_LOGE("send rawephemb to novatel failed!\n");
		return -1;
	}

#ifdef SINAN
	ret = write(serial_fd, msg8, sizeof(msg8));
		if(ret < 0) {
			GPS_LOGE("send rtk to novatel failed!\n");
			return -1;
		}
#endif

#ifndef SINAN
	ret = write(serial_fd, msg6, sizeof(msg6));
	if(ret < 0) {
		GPS_LOGE("send rawsbasframeb to novatel failed!\n");
		return -1;
	}
	
	ret = write(serial_fd, msg7, sizeof(msg7));
	if(ret < 0) {
		GPS_LOGE("send ionutcb to novatel failed!\n");
		return -1;
	}
#endif
	return 0;
}

/*
 *	参数		:	int fd			I		串口描述符
 * 				int nSpeed		I		波特率
 * 				int nBits		I		数据位
 * 				char nEvent		I		校验位
 * 				int nStop		I		硬件停止位
 * 	返回		:	-1：error	0:ok
 * 	描述		:	配置串口
 * 	历史		:
 * 	备注		: 
 */
static int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)  
{  
    struct termios newtio,oldtio;  
    if  ( tcgetattr( fd,&oldtio)  !=  0) {   
        perror("SetupSerial 1");  
        return -1;  
    }  
    bzero( &newtio, sizeof( newtio ) );  
    newtio.c_cflag  |=  CLOCAL | CREAD;  
    newtio.c_cflag &= ~CSIZE;  
  
    switch( nBits )  
    {  
    case 7:  
        newtio.c_cflag |= CS7;  
        break;  
    case 8:  
        newtio.c_cflag |= CS8;  
        break;  
    }  
  
    switch( nEvent )  
    {  
    case 'O':  
        newtio.c_cflag |= PARENB;  
        newtio.c_cflag |= PARODD;  
        newtio.c_iflag |= (INPCK | ISTRIP);  
        break;  
    case 'E':   
        newtio.c_iflag |= (INPCK | ISTRIP);  
        newtio.c_cflag |= PARENB;  
        newtio.c_cflag &= ~PARODD;  
        break;  
    case 'N':    
        newtio.c_cflag &= ~PARENB;  
        break;  
    }  
      switch( nSpeed )  
    {  
    case 2400:  
        cfsetispeed(&newtio, B2400);  
        cfsetospeed(&newtio, B2400);  
        break;  
    case 4800:  
        cfsetispeed(&newtio, B4800);  
        cfsetospeed(&newtio, B4800);  
        break;  
    case 9600:  
        cfsetispeed(&newtio, B9600);  
        cfsetospeed(&newtio, B9600);  
        break;  
    case 115200:  
        cfsetispeed(&newtio, B115200);  
        cfsetospeed(&newtio, B115200);  
        break;  
    case 460800:  
        cfsetispeed(&newtio, B460800);  
        cfsetospeed(&newtio, B460800);  
        break;  
    default:  
        cfsetispeed(&newtio, B9600);  
        cfsetospeed(&newtio, B9600);  
        break;  
    }  
    if( nStop == 1 )  
        newtio.c_cflag &=  ~CSTOPB;  
    else if ( nStop == 2 )  
    newtio.c_cflag |=  CSTOPB;  
    newtio.c_cc[VTIME]  = 0;//重要  
    newtio.c_cc[VMIN] = 100;//返回的最小值  重要  
    tcflush(fd,TCIFLUSH);  
    if((tcsetattr(fd,TCSANOW,&newtio))!=0)  
    {  
        return -1;  
    }  
    return 0;  
}  

/*
 *	参数		:	void
 * 	返回		:	int		0:打开失败	>0:打开成功,返回文件描述符
 * 	描述		:	打开novatel芯片
 * 	历史		:
 * 	备注		: 	参考linux系统的open
 */
extern int open_oem4()
{
	//TODO 打开串口
	//TODO 发送初始化命令
	int serial_fd;

#if 1
	serial_fd = open("/dev/s3c2410_serial2",O_RDWR);
	if (serial_fd <0) {
		serial_fd = open("/dev/s3c2410_serial3",O_RDWR);
		if(serial_fd<0){
			GPS_LOGD("can not open serial\n");
			return -1;
		}
	}
#endif

#if 0
	serial_fd = open("/dev/ttyUSB0",O_RDWR);
	if (serial_fd <0) {
		serial_fd = open("/dev/ttyUSB1",O_RDWR);
		if(serial_fd<0){
			serial_fd = open("/dev/ttyUSB2", O_RDWR);
			if (serial_fd <0) {
				GPS_LOGD("can not open oem3\n");
				return -1;
			}
		}
	}
#endif	
	/* 设置参数 */
	set_opt(serial_fd,115200, 8, 'N', 1);
	//set_opt(serial_fd,9600, 8, 'N', 1);
	send_cmd2novatel(serial_fd);
	return serial_fd;
}

/*
 *	参数		:	int handle		I		novatel芯片描述符
 * 	返回		:	int
 * 	描述		:	关闭novatel芯片描述符
 * 	历史		:
 * 	备注		: 	参考linux系统的close
 */
extern int close_oem4(int handle)
{
	return close(handle);
}
