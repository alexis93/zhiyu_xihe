#ifndef __IWISE_UTIL_H__
#define __IWISE_UTIL_H__

#include "Parserlib/gps_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--fuctions--------------------------------------------------------------*/
extern int strftime_now(char *tbuf, int length);
extern void cal_lon_lat_alt(sol_t *sol, double *lat, double *lon, double *hgt);
extern void updatenav(nav_t *nav);
extern void weeksecond2gtime(const int weeksecond, gtime_t * time_temp);
extern obs_t* checkobs(obs_t* obs);

#ifdef __cplusplus
}
#endif

#endif
