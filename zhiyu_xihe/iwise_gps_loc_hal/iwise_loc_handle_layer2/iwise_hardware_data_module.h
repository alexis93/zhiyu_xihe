/*-----------------------------------------------------------------
 *	iwise_hardware_data_module.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#ifndef _IWISE_HARDWARE_MODULE_H_
#define _IWISE_HARDWARE_MODULE_H_

#include <pthread.h>
#include "iwise_loc_base/iwise_serial_parser.h"
/*
 * 硬件数据获取模块的上下文结构体
 * */
typedef struct{
	iwise_serial_parser_t serial_parser;  // 硬件数据解析器
	pthread_t	hardware_thread;
}iwise_hardware_data_context_t;

/*
 * 初始化硬件数据获取模块
 * 在该初始化函数中会启动数据读取线程
 * */
int iwise_hardware_module_init();
int iwise_hardware_module_destroy();

#endif
