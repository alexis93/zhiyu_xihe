/*-------------------------------------------------------------------
 *	iwise_loc_base.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version :
 * 	history	: 2015-05-26 created by chenxi
 *-------------------------------------------------------------------*/
 
#ifndef _IWISE_LOC_BASE_H
#define _IWISE_LOC_BASE_H

#include <stdbool.h>
#include "rtklib.h"
//#include "clock_orbit_rtcm.h"
//#include "iwise_pool.h"
#include "iwise_loc_log.h"
#include "iwise_loc_type.h"


// 统一变量命名
typedef struct ClockOrbit		iwise_clockorbit_t;
typedef obs_t  					iwise_gnss_obs_t;
typedef nav_t  					iwise_gnss_nav_t;
typedef raw_t					iwise_raw_parser_t;
typedef sol_t					iwise_sol_t;
typedef ssat_t					iwise_ssat_t;


#endif
