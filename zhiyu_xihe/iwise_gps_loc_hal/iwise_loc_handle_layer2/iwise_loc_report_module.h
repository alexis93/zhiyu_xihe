/*-----------------------------------------------------------------
 *	iwise_loc_report_module.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#ifndef _IWISE_LOC_REPORT_MODULE_H_
#define _IWISE_LOC_REPORT_MODULE_H_

#include "iwise_loc_base/iwise_loc_base.h"

int iwise_loc_report_sol(iwise_sol_t* sol);

int iwise_loc_report_nmea(iwise_nmea_parser_t* nmea_parser);

int iwise_loc_report_message(iwise_message_t* msg);

int iwise_loc_report_sol_ins(iwise_sol_t* sol);

#endif
