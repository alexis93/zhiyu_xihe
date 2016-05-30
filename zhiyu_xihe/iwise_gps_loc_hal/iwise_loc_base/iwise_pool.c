/*-----------------------------------------------------------------
 *	iwise_pool.c :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	:	2015-05-18	created by chenxi
 * 				2015-06-12	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#include <stdio.h>
#include <errno.h>
#include "iwise_pool.h"
#include "iwise_loc_log.h"

/*
 *	参数		:	void
 * 	返回		:	iwise_pool_t*	NULL：分配内存失败；	pool:分配内存成功
 * 	描述		:	初始化存储钟差改正数的内存池
 * 	历史		:
 * 	备注		: 
 */
iwise_pool_t * iwise_pool_init()
{
	iwise_pool_t * pool = calloc(1, sizeof(iwise_pool_t));
	if(pool == NULL) {
		GPS_LOGD("malloc pool failed: %s", strerror(errno));
		return NULL;
	}
	return pool;
}

/*
 *	参数		:	iwise_pool_t * pool			I		存储钟差改正数的内存池的指针
 * 	返回		:	int		-1：释放内存失败；		0：释放内存成功
 * 	描述		:	销毁存储钟差改正数的内存池
 * 	历史		:
 * 	备注		: 
 */
int iwise_pool_destroy(iwise_pool_t * pool)
{
	if( pool == NULL) {
		GPS_LOGD("malloc pool NULL");
		return -1;
	}
	free(pool);
	return 0;
}

/*
 *	参数		:	iwise_pool_t * pool			I		存储钟差改正数的内存池的指针
 * 	返回		:	int		0：复制内存成功
 * 	描述		:	移动和复制内存
 * 	历史		:
 * 	备注		: 
 */
int iwise_pool_compress(iwise_pool_t * pool)
{
	if(MAX_BUFFER-pool->end < PACKAGE_SIZE){
		memmove(pool->buffer, pool->buffer+pool->start, pool->end-pool->start);
		pool->end = pool->end-pool->start;
		pool->start = 0;
	}
	return 0;
}
