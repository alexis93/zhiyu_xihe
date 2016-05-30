/*-----------------------------------------------------------------
 *	iwise_serial_parser.c :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version :
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/

#include <stdbool.h>

#include "Parserlib/rtklib.h"
#include "iwise_nmea_parser.h"
#include "iwise_serial_parser.h"

/*
 *	参数		:	iwise_serial_parser_t* parser		I		解析器数据数据存储结构
 * 	返回		:	void
 * 	描述		:	初始化解析器
 * 	历史		:
 * 	备注		: 
 */
void iwise_serial_parser_init(iwise_serial_parser_t* parser)
{
	parser->is_nmea = false;
    parser->is_raw = false;
    parser->nmea_length = 0;
    memset(parser->nmea, 0, 1000);
	init_raw(&(parser->raw_parser));
	iwise_nmea_parser_init(&(parser->nmea_parser));
}

/*
 *	参数		:	iwise_serial_parser_t* parser		I		解析器数据数据存储结构
 * 	返回		:	void
 * 	描述		:	销毁解析器
 * 	历史		:
 * 	备注		: 
 */
void iwise_serial_parser_destroy(iwise_serial_parser_t* parser)
{
	iwise_nmea_parser_destroy(&(parser->nmea_parser));
	free_raw(&(parser->raw_parser));
}

/*
 *	参数		:	int DEVICE							I		需要解析的芯片的设备号
				iwise_serial_parser_t* parser		I		解析的数据存储结构
				unsigned char buf					I		输入需要解析的数据
 * 	返回		:	int	-2:解析nmea失败	-1：解析原始数据失败	1：解析原始观测数据成功		2：解析星历成功
 * 					10：解析GGA成功	11：解析GST成功		12：解析GSV成功			13：解析RMC成功
 * 					21：解析成功了部分GSV
 * 	描述		:	将数据注入解析器，返回解析结果
 * 	历史		:
 * 	备注		: 
 */
int iwise_serial_parser_input(int DEVICE, iwise_serial_parser_t* parser, unsigned char buf)
{
	int ret = 0;

	if (!parser->is_nmea && !parser->is_raw)
	{
		//GPS_LOGD("first ret=%d, buf=%d, bf=%c, \n",ret,buf,buf);
		/*对于未知数据，根据第一个字符来判断*/
		if (buf == '$')
		{
			//GPS_LOGD("NMEA start \n");
			parser->nmea[parser->nmea_length++] = buf;
			ret = iwise_nmea_parser_input(&(parser->nmea_parser), buf);
			parser->is_nmea = true;
		}

		else //if (buf==0xAA) //zhuyong
		{
			//GPS_LOGD("raw start \n");
			parser->is_raw = true;
			ret = input_raw(&(parser->raw_parser), DEVICE, buf);
        }
	}
	else if (parser->is_nmea)
	{
		/*nmea 解析分支*/

		parser->nmea[parser->nmea_length] = buf;
		ret = iwise_nmea_parser_input(&(parser->nmea_parser), buf);
		//if (ret==10) GPS_LOGD("ret=%d, buf=%d, bf=%c, \n",ret,buf,buf);
		parser->nmea_length++;
		if(parser->nmea_length >=MAX_NMEA_LEN )
		{
			GPS_LOGD("parse_nmea length too long: %d", parser->nmea_length);
			parser->is_nmea = false;
			parser->nmea_length = 0;
			memset(parser->nmea, 0, MAX_NMEA_LEN);
		} else if(parser->nmea[parser->nmea_length-1] == '\n')
		{
			//GPS_LOGD("get a line nmea!");
			parser->nmea[parser->nmea_length] = '\0';
			parser->is_nmea = false;
			parser->nmea_length = 0;
			memset(parser->nmea, 0, MAX_NMEA_LEN);
		}

	}

	else if (parser->is_raw)
	{	
		//始数据 解析分支
		ret = input_raw(&(parser->raw_parser), DEVICE, buf);
		//GPS_LOGD("raw ret=%d, buf=%d, bf=%c, \n",ret,buf,buf);
	}
	return ret;
}
