/*
 * file_handle.c
 *
 *  Created on: Jul 7, 2015
 *      Author: zhiyugnss
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


extern int open_file()
{

	//int fd = open("/home/zhiyugnss/eclipse_workspace/ParserRtkProject/src/202582296k.cnb", O_RDONLY);
	int fd = open("/data/ljrublox/hycj.log", O_RDONLY);
	if(fd < 0){
		perror("open hjcj.log failed:");
		return -1;
	} else {
		printf("open hycj.log success!\n");
	}
	return fd;
}


/*
 *	参数		:	int	handle		I		串口描述符
 * 	返回		:
 * 	描述		:	关闭gps芯片描述符
 * 	历史		:
 * 	备注		: 	参考linux系统的close
 */
extern int close_file(int handle)
{
	return close(handle);
}
