/*-----------------------------------------------------------------
 *	iwise_loc_hal.c :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-23	
 *-------------------------------------------------------------------*/
#ifndef _IWISE_LOC_HAL_H_
#define _IWISE_LOC_HAL_H_

#include "iwise_loc_base/iwise_loc_base.h"

const GpsInterface* gps__get_gps_interface(struct gps_device_t* dev);

#endif
