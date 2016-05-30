/*-----------------------------------------------------------------
 *	iwise_pool.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version :
 * 	history	: 	2015-05-18 created by chenxi
 *-------------------------------------------------------------------*/
#ifndef _IWISE_POOL_H
#define _IWISE_POOL_H

#define MAX_BUFFER 		4096
#define PACKAGE_SIZE	512


/**
 * 0						start			end 			MAX_BUFFER
 * +------------------------------------------------------------+
 * |  已经解析了的数据（无用）    |  还没有解析的数据|  空闲空间			|
 * +------------------------------------------------------------+
 * 
 * */

/**
 * 缓冲池
 * 用来存放通过tcp协议接收到的数据包，
 * 先存放后解析
 * */
struct iwise_pool_s {
	unsigned char buffer[MAX_BUFFER];
	int start;
	int end;
};

typedef struct iwise_pool_s iwise_pool_t;

/**
 * 作用：	初始化缓冲池，空间动态分配
 * 作者：	chenxi
 * 修改记录：	2015/05/16
 * */
iwise_pool_t *  iwise_pool_init();

/**
 * 作用：	释放缓冲池
 * 作者：	chenxi
 * 修改记录：	2015/05/16
 * */
int iwise_pool_destroy(iwise_pool_t * pool);

/**
 * 作用：	当缓冲区内的空间不够时，将已经解析了的数据数据全部删除
 * 作者：	chenxi
 * 修改记录：	2015/05/16
 * */
int iwise_pool_compress(iwise_pool_t * pool);

#endif
