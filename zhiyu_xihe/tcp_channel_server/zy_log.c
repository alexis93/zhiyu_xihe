/*
 * gnss_log.c
 *
 *  Created on: 2014年3月23日
 *      Author: zhuyong
 */
//#include "zlog/zlog.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "zy_log.h"

#define LOG_BASE_PATH "/storage/sd_internal/log/"
#define LOG_BUF_SIZE  1024
#define MAXFILENAME   1300

int tcp_server_log;

int open_fd (char *fileName)
{
    int fd;
    char fileSt[MAXFILENAME]= LOG_BASE_PATH;
    char * fileStr = fileSt;
    char tbuf[LOG_BUF_SIZE] = "";
	struct tm * now = 0;
	time_t t;
	time(&t);
    t+=8*60*60;
	now = localtime(&t);
	int c = strftime(tbuf, LOG_BUF_SIZE, "%F-%H-%M-%S", now);
	sprintf(fileStr, "%s%s/%s_%s%s.txt",
			LOG_BASE_PATH, "tcp","t", fileName, tbuf);
			
    fd = open(fileStr, O_WRONLY|O_CREAT|O_NONBLOCK,06666);
    if(fd < 0)
    {
       
        return -1;
    }
   

    return fd;
}


int zy_log_init(void)
{
	int rc;
	tcp_server_log = open_fd("tcpchannel");
	if (tcp_server_log<0) {
		fprintf(stderr, "初始化日志出错,请检查文件[%s]的格式\n", LOG_CONF);
		puts("初始化日志出错,请检查文件的格式");
		return -1;
	}

	return 0;
}

