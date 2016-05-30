/*-----------------------------------------------------------------
 *	iwise_nmea_parser.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-12	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#ifndef _IWISE_SERIAL_PARSER_H_
#define _IWISE_SERIAL_PARSER_H_

#include <stdio.h>
#include <stdbool.h>

#include "iwise_nmea_parser.h"
#include "iwise_loc_base/iwise_loc_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * 解析结果的返回值
 * */
#define		IWISE_SERIAL_RAW_PARSER_ERROR		NMEA_TYPE_ERROR
#define		IWISE_SERIAL_NMEA_PARSER_ERROR		-1
#define		IWISE_SERIAL_PARSER_NULL			0

#define		IWISE_SERIAL_PARSER_GNSS_RAW		1
#define		IWISE_SERIAL_PARSER_GNSS_OBS		IWISE_SERIAL_PARSER_GNSS_RAW
#define		IWISE_SERIAL_PARSER_GNSS_NAV		IWISE_SERIAL_PARSER_GNSS_RAW + 1

#define		IWISE_SERIAL_PARSER_NMEA			NMEA_TYPE_GGA
#define		IWISE_SERIAL_PARSER_NMEA_GGA		NMEA_TYPE_GGA
#define		IWISE_SERIAL_PARSER_NMEA_GSV		NMEA_TYPE_GSV
#define		IWISE_SERIAL_PARSER_NMEA_GST		NMEA_TYPE_GST
#define		IWISE_SERIAL_PARSER_NMEA_RMC		NMEA_TYPE_RMC
#define		IWISE_SERIAL_PARSER_NMEA_GSV_PART 	IWISE_SERIAL_PARSER_NMEA

#define		MAX_NMEA_LEN		256

/*
 * GPS 芯片数据的分离和解析器
 * */
struct iwise_serial_parser_s
{
	/* 原始观测值的解析器，基于rtklib库 */
	iwise_raw_parser_t	raw_parser;
	/* nmea 数据的解析器 */
	iwise_nmea_parser_t	nmea_parser;
    bool is_nmea;
    bool is_raw;
    int nmea_length;
    char nmea[MAX_NMEA_LEN];
};

typedef struct iwise_serial_parser_s iwise_serial_parser_t;


void iwise_serial_parser_init(iwise_serial_parser_t* parser);
void iwise_serial_parser_destroy(iwise_serial_parser_t* parser);
/*
 * 将数据注入解析器，返回解析结果
 * */
int iwise_serial_parser_input(int DEVICE, iwise_serial_parser_t* parser, unsigned char buf);

#ifdef __cplusplus
}
#endif
#endif
