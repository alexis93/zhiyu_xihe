
/*
 * nmea_parse.h
 *
 * 	Created on:2014年6月16号
 * 		Author:zhangcc
 */
#ifndef NMEA_PARSE_H_
#define NMEA_PARSE_H_
 
#include "iwise_loc_base.h"
#include "ins_common.h"

#define SV_USED 8
#define FIX_STATUS 1
#define pi 3.1415926535898 

#define SEND_GNSS_SIZE 52
#define SEND_M_SIZE 200
#define SEND_INS_SIZE 300

struct iwise_loc_nmea_gga_s{
	double latitude;
	double longitude;
	double altitude;
	char NS_ind;
	char EW_ind;	
	time_t utc;
	unsigned char sv_used;
	unsigned char fix_status;
	double accuracy;
}__attribute((aligned (1)));
typedef struct iwise_loc_nmea_gga_s iwise_loc_nmea_gga_s_t;



extern void GpsLocatio2nmea(iwise_loc_nmea_gga_s_t *nmea, GpsLocation* location);
extern int transnmea_gga(unsigned char *buff, const iwise_loc_nmea_gga_s_t* nmea_info);
extern int do_send_GpsLocation(GpsLocation * gpslocation);
extern int do_send_gnss_NMEA(char* nmea,int nmea_len);
extern int do_send_NMEA(char* nmea,int nmea_len);
extern int do_send_ins(double* sol,double* fil);
extern int do_send_ins_NMEA(INS_RESULT* ins_result);


#endif
