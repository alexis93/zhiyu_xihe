/*-----------------------------------------------------------------
 *	iwise_nmea_parser.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-12	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#ifndef _IWISE_NMEA_H_
#define _IWISE_NMEA_H_

#include "iwise_loc_type.h"

#define GPS_MAX_SVS 				32
#define NMEA_GGA					0x01
#define NMEA_GSV					0x02

/* GPS状态
 * 0 = 不可用； 1 = 单点定位； 2 = 差分定位； 6 = 正在估算
 */
#define FIX_STATUS_VALID            			3  //定位有效  添加 GPRMC
#define FIX_STATUS_AUTO            				4  //自主定位  添加 GPRMC
#define FIX_STATUS_INVALID						0
#define FIX_STATUS_GPS_SPS						1
#define FIX_STATUS_DGPS_SPS						2

#define FIX_STATUS_ESTIMATE_DEAD_ROCKONING		6


#define	NMEA_TYPE_ERROR							-2
#define	NMEA_TYPE_GGA							10
#define	NMEA_TYPE_GSV							11
#define	NMEA_TYPE_GST							12
#define	NMEA_TYPE_RMC							13
#define NMEA_TYPE_GSV_PART						21

struct nmea_t {
	unsigned gsv_type;
	unsigned gga_type;
	unsigned gga_count;
	unsigned nmea_parse_status;
	int NoMsg;
	int MsgNo;

	iwise_loc_nmea_gsv_s_type nmea_gsv_info;
	iwise_loc_nmea_gga_s_type nmea_gga_info;
	iwise_loc_nmea_report_s_type nmea_report_info;
	iwise_loc_nmea_gst_s_type nmea_gst_info;
	iwise_loc_nmea_rmc_s_type nmea_rmc_info;
};
typedef struct nmea_t iwise_nmea_parser_t;


/* nmea_error */
int last_error;
extern void set_last_error(int error);
extern int get_last_error(void);
extern char *get_error_string(int error);

/* nmea functions */
extern int iwise_nmea_parser_init(iwise_nmea_parser_t* nmea_parser);
extern int iwise_nmea_parser_input(iwise_nmea_parser_t* nmea_parser,
									unsigned char buf);
extern void iwise_nmea_parser_destroy(iwise_nmea_parser_t* nmea_parser);

#endif
