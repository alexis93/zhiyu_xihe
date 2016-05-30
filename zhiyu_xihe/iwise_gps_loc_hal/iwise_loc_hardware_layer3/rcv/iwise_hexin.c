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


static int set_usb_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio,oldtio;
    if  ( tcgetattr( fd,&oldtio)  !=  0) {
        GPS_LOGE("SetupSerial 1");
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
 *	参数		:	int serial_fd		I		串口描述符
 * 	返回		:	int		0:写入成功	-1;写入失败
 * 	描述		:	向hexin发送命令
 * 	历史		:
 * 	备注		: 
 */

static int send_cmd2hexin(int serial_fd)
{
    //在ublox里面默认ublox是输出nmea格式的数据，因此在设置是要关掉除gga和gsv以外的数据，
    //同时开启输出原始观测数据和星厉
    int ret = -1;

	unsigned char msg[] =  "$CFGSYS,h01\n"; /*默认是开启gps和北斗，现在只开启了gps定位系统 */
	//unsigned char msg[] =  "$CFGSYS,h11\n"; /* 开启gps和北斗定位系统 */
	ret = write(serial_fd, msg, 12);
    if(ret < 0)
    {
		GPS_LOGD("send config set sys gps");
        return ret;
    }
     return ret;
}


/*
 *	参数		:	void
 * 	返回		:	int		>0:打开设备成功；	-1:打开设备失败
 * 	描述		:	初始化串口并向gps芯片发送命令
 * 	历史		:
 * 	备注		:
 */

extern int open_hexin()
{
	//TODO 打开串口
	//TODO 发送初始化命令
	GPS_LOGE("open hexin");
	int serial_fd;
	serial_fd = open("/dev/ttyUSB0",O_RDWR | O_NOCTTY);
	if (serial_fd <0) {
		GPS_LOGE("open ttyUSB0 failed:");
		serial_fd = open("/dev/ttyUSB1",O_RDWR | O_NOCTTY);
		if(serial_fd<0){
			GPS_LOGE("open ttyUSB1 failed");
			return -1;
		}else {
			GPS_LOGV("open ttyUSB1 success");
		}
	} else {
		GPS_LOGV("open ttyUSB0 success");
	}
	//* 设置参数 *
	set_usb_opt(serial_fd,115200, 8, 'N', 1);
	send_cmd2hexin(serial_fd);  //TODO add hexin init cmd
	return serial_fd;
}


 /*
 *	参数		:	int	handle		I		串口描述符
 * 	返回		:
 * 	描述		:	关闭gps芯片描述符
 * 	历史		:
 * 	备注		: 	参考linux系统的close
 */
extern int close_hexin(int handle)
{
	return close(handle);
}


#define MAXHEXINBUF 4096
#define MAXCMDBUF 4096
#define DATA_STATUS_WAIT   0
#define DATA_STATUS_CMD      1
#define DATA_STATUS_NMEA_START  2
#define DATA_STATUS_RAWSFR_START   3
#define DATA_STATUS_RAWMSR_START   4

char hexin_tempbuf[MAXHEXINBUF]; //原始数据缓冲区
char hexin_cmdbuf[MAXCMDBUF]; //命令缓冲区
int buf_p=0;
int cmd_p=0;
int dataflag=DATA_STATUS_WAIT;

#define FIELDCNT 32
#define FIELDLEN 32
char field[FIELDCNT][FIELDLEN];
typedef union union_word_u{
	uint32_t word;
	uint8_t byte[4];
} union_word_t;

union_word_t tempword;

int do_RAMSFR_prehandle(char* cmdbuf,int cmdcount, unsigned char* buf){
	int cmd_p;
	int field_cnt=0;
	int buf_p=0;
	int i,j;
	uint8_t satID=0;
	union_word_t tempword;
	buf[buf_p++]='#'; //原始观测量起始标志1
	buf[buf_p++]='S'; //原始观测量起始标志2,S代表星历
	buf[buf_p++]=41;  //TODO 原始观测量长度
	cmd_p=8; //0-7="#RAMSFR,",8开始是卫星号
	//取卫星号
	while (cmdbuf[cmd_p]!=',') {
		satID=satID*10+(cmdbuf[cmd_p++]-48);
	}

	buf[buf_p++]=satID;

	//循环处理word0-word9
	for (i=0;i<10;i++) {
		tempword.word=0;
		cmd_p++;
		if (cmdbuf[cmd_p]=='h') {
			//处理word字段
			cmd_p++;
			//转换字符串到二进制
			for (j=0;j<8;j++) {
				if ((cmdbuf[cmd_p]>='0')&&(cmdbuf[cmd_p]<='9')) {
					tempword.word=tempword.word * 16+(cmdbuf[cmd_p]-'0');
				} else if((cmdbuf[cmd_p]>='A')&&(cmdbuf[cmd_p]<='F')){
					tempword.word=tempword.word * 16+(cmdbuf[cmd_p]-'A'+10);
				}
				cmd_p++;  //添加 每次转换后，解析下一个字符，应加一
			}

			tempword.word = tempword.word << 2;
			/* 由于参照ublox，原始数据的word应该由32bit组成，高八位为零，低24位顺序不变(MSB)*/
			buf[buf_p++]=tempword.byte[1];
			buf[buf_p++]=tempword.byte[2];
			buf[buf_p++]=tempword.byte[3];
			buf[buf_p++]=tempword.byte[0] && 0x00;

		}
	}

	if (satID > 37) return 0; //gps卫星编号是1-37 屏蔽北斗(160-)信号
	return buf_p;
}

int do_RAMMSR_prehandle(char* cmdbuf,int cmdcount,unsigned char* buf){
		int cmd_p;
		int field_cnt=0;
		int buf_p=0;
		int i,j;
		uint8_t satID=0;
		union_word_t tempword;
		buf[buf_p++]='#'; //原始观测量起始标志1
		buf[buf_p++]='M'; //原始观测量起始标志2
		//buf[buf_p++]=((unsigned char *)&cmdcount)[0]-8; //原始观测量长度
		buf[buf_p++]=cmdcount-8;

		for(i = 7; i < cmdcount; i++) {
			if(cmdbuf[i] == '\r') {
				continue; //由于MSR的结束是以\r\n结束，但是对解析nmea有影响，过滤掉\r
			}
			buf[buf_p++] = cmdbuf[i];
		}

		return buf_p;
}

int do_NMEA_prehandle(char* cmdbuf,int cmdcount,unsigned char* buf){
	int i;
	for (i=0;i<cmdcount;i++){
		buf[i]=cmdbuf[i];
	}
	return cmdcount;
}

int do_CMD_prehandle(char* cmdbuf,int cmdcount,unsigned char* buf){
	int i;
	for (i=0;i<cmdcount;i++){
		buf[i]=cmdbuf[i];
	}
	return cmdcount;
}

int read_hexin(int handle, unsigned char* buf, size_t count)
{
	int readcount,tempcount;
	int i,j;
	buf_p=0;
	if (count<MAXHEXINBUF)
	{
		readcount=count;
	} else {
		readcount=MAXHEXINBUF;
	}

	tempcount=read(handle,&hexin_tempbuf,readcount);
	for (i=0;i<tempcount;i++){
		if (dataflag==DATA_STATUS_WAIT&&hexin_tempbuf[i]=='$')
		{   //判断命令起始$符
			dataflag=DATA_STATUS_CMD;
			hexin_cmdbuf[cmd_p++]='$';
		}
		else if ((dataflag==DATA_STATUS_CMD)&&(hexin_tempbuf[i]!=',')&&(hexin_tempbuf[i]!='\n'))
		{	//缓冲完整命令
			hexin_cmdbuf[cmd_p++]=hexin_tempbuf[i];
		}
		else if (dataflag==DATA_STATUS_CMD&&hexin_tempbuf[i]==',')
		{	//判断各种命令
			hexin_cmdbuf[cmd_p++]=',';
			if ((hexin_cmdbuf[1]=='R')&&(hexin_cmdbuf[2]=='A')&&(hexin_cmdbuf[3]=='W')&&(hexin_cmdbuf[4]=='S')&&(hexin_cmdbuf[5]=='F')&&(hexin_cmdbuf[6]=='R'))
			{	//判断是否是普通星历
				hexin_cmdbuf[0]='#';//改变命令提示符，使librtk判断其为原始观测量
				dataflag=DATA_STATUS_RAWSFR_START;
			}
			else if ((hexin_cmdbuf[1]=='R')&&(hexin_cmdbuf[2]=='A')&&(hexin_cmdbuf[3]=='W')&&(hexin_cmdbuf[4]=='M')&&(hexin_cmdbuf[5]=='S')&&(hexin_cmdbuf[6]=='R'))
			{	//判断是否是原始观测量
				hexin_cmdbuf[0]='#';//改变命令提示符，使librtk判断其为原始观测量
				dataflag=DATA_STATUS_RAWMSR_START;
			}
			else
			{	//是各种NMEA命令
				dataflag=DATA_STATUS_NMEA_START;
			};
		}
		else if (hexin_tempbuf[i]=='\n')
		{	//行命令结束符，开始处理缓冲命令
			hexin_cmdbuf[cmd_p++]='\n';

			if (dataflag==DATA_STATUS_CMD)
			{	//只有命令，没有数据，将命令缓冲全部导入buf，命令缓冲归零
				//TODO 执行命令处理转换函数
				buf_p=do_CMD_prehandle(hexin_cmdbuf,cmd_p,buf);

				dataflag=DATA_STATUS_WAIT;
			}
			else if (dataflag==DATA_STATUS_RAWSFR_START)
			{   //普通星历数据接收完毕，进行转换后，导入buf，命令归零
				//TODO 执行星历数据处理转换函数

				buf_p=do_RAMSFR_prehandle(hexin_cmdbuf,cmd_p,buf);

				dataflag=DATA_STATUS_WAIT;

			}
			else if (dataflag==DATA_STATUS_RAWMSR_START)
			{   //普通星历数据接收完毕，进行转换后，导入buf，命令归零
				//TODO 执行原始观测量处理转换函数
				buf_p=do_RAMMSR_prehandle(hexin_cmdbuf,cmd_p,buf);
				int i = 0;
				dataflag=DATA_STATUS_WAIT;
			}
			else if (dataflag==DATA_STATUS_NMEA_START){
				//TODO 执行NMEA处理转换函数
				buf_p=do_NMEA_prehandle(hexin_cmdbuf,cmd_p,buf);
				dataflag=DATA_STATUS_WAIT;
			}
			//清理命令缓冲区
			cmd_p=0;
			memset(hexin_cmdbuf,0x00,MAXCMDBUF);
		}
		else if (dataflag!=DATA_STATUS_WAIT){
			//将数据导入命令缓冲
			hexin_cmdbuf[cmd_p++]=hexin_tempbuf[i];
		}
		else
		{
			//扔掉其他数据
		}
	}
	return buf_p;
}

int ioctl_hexin(int handle, unsigned long request, void* buf)
{
	return ioctl(handle, request, buf);
}

