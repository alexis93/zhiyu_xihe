/*
 * zy_protocol.h
 *
 * 	Created on:2015年7月10号
 * 		Author:zhangcc
 */


#include <unistd.h>
#include <string.h>
#include "data_parse.h" 
#include "iwise_loc_log.h"
#include "zy_tcp_worker.h"
#include "ins_common.h"
#include "zy_gnss.h"


/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	把GpsLocation 数据转成 nmea数据
 * 	历史		:
 * 	备注		: 	()
 */

 void GpsLocatio2nmea(iwise_loc_nmea_gga_s_t* nmea, GpsLocation* location)
{
   
    //nmea->latitude = hhmm_to_convert(location->latitude);
    //nmea->longitude = hhmm_to_convert(location->longitude);
    nmea->latitude = location->latitude;
    nmea->longitude = location->longitude;
    nmea->altitude = location->altitude;
    nmea->NS_ind = (nmea->latitude>0) ?"N":"S";
    nmea->EW_ind = (nmea->longitude >0) ?"E":"W";
    //nmea->utc =(long)(floor(location->timestamp)/1000);  //
    nmea->utc =location->timestamp;    //add by zhuyong 2015.12.11 for debug time
    nmea->accuracy = location->accuracy ;
    nmea->sv_used =  SV_USED;
    nmea->fix_status = FIX_STATUS;
    // GPS_LOGI("do GpsLocatio2nnmea ok");

   
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	把GpsLocation 数据转成 nmea数据
 * 	历史		:
 * 	备注		: 	()
 */

double  ins2gps(double ins)
{ 
	double gps=0.0;
	gps= ins*180/pi;
	return gps;
}

/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	把GpsLocation 数据转成 nmea数据
 * 	历史		:
 * 	备注		: 	()
 */
void sol2nmea(iwise_loc_nmea_gga_s_t* nmea, INS_RESULT* ins_sol)
{
	nmea->latitude = ins2gps(ins_sol->sol[1]);
    nmea->longitude = ins2gps(ins_sol->sol[2]);
    nmea->altitude = ins_sol->sol[3];
    nmea->NS_ind = (nmea->latitude>0) ?"N":"S";
    nmea->EW_ind = (nmea->longitude >0) ?"E":"W";
    nmea->utc =(long)(floor(ins_sol->sol[0]));
    nmea->accuracy = 1.0 ;
    nmea->sv_used =  SV_USED;
    nmea->fix_status = FIX_STATUS;
   
	
	
	
}



/*
 *	参数		:	void
 * 	返回		:	void
 * 	描述		:	生成NMEA （GGA）语句
 * 	历史		:
 * 	备注		: 	()
 */

int transnmea_gga(unsigned char *buff, const iwise_loc_nmea_gga_s_t* nmea_info)
{   
	//GPS_LOGI("in  transnmea_gga ok");
	double  h,ep[6],pos[3],dms1[3],dms2[3],dop=1.0;
	char *p=(char *)buff,*q,sum;
    double age= 0.0;
    pos[0] = nmea_info->latitude;
    pos[1] = nmea_info->longitude;
    pos[2] = nmea_info->altitude;
    
    gtime_t time ;
    time.time = nmea_info->utc;
    time.sec  = 0.0;
    
    if (nmea_info->fix_status <= SOLQ_NONE) {
        p+=sprintf(p,"$GPGGA,,,,,,,,,,,,,,");
        for (q=(char *)buff+1,sum=0;*q;q++) sum^=*q;
        p+=sprintf(p,"*%02X%c%c",sum,0x0D,0x0A);
        return p-(char *)buff;
    }
    // GPS_LOGI("in  transnmea_gga ok");
    time2epoch(gpst2utc(time),ep);
    h=geoidh(pos);
    deg2dms(nmea_info->latitude,dms1);
    deg2dms(nmea_info->longitude,dms2);
    
   
    p+=sprintf(p,"$GPGGA,%02.0f%02.0f%05.2f,%02.0f%010.7f,%s,%03.0f%010.7f,%s,%d,%02d,%.1f,%.3f,M,%.3f,M,%.1f,",
               ep[3],ep[4],ep[5],dms1[0],dms1[1]+dms1[2]/60.0,pos[0]>=0?"N":"S",
               dms2[0],dms2[1]+dms2[2]/60.0,pos[1]>=0?"E":"W",nmea_info->fix_status,
               nmea_info->sv_used,dop,pos[2]-h,h,age);
    for (q=(char *)buff+1,sum=0;*q;q++) sum^=*q; /* check-sum */
    p+=sprintf(p,"*%02X%c%c",sum,0x0D,0x0A);
    // GPS_LOGI("do transnmea_gga ok");
    return p-(char *)buff;

}

/*
 *	参数		:	GpsLocation * gpslocation   I  安卓上报定位结果结构体
 * 	返回		:	int 0  发送成功
 * 	描述		:	发送GpsLocation(高精度定位) 数据到tcpserver
 * 	历史		:
 * 	备注		: 	()
 */
int do_send_GpsLocation(GpsLocation * gpslocation){
	char sendbuf[SEND_GNSS_SIZE];
	int sendbuflen;
	zy_gpslocation_packet_t zy_gpslocation;
		    
    zy_gpslocation.head.sync = PACKET_SYNC;
    zy_gpslocation.head.type = GPSLOCATION_TYPE;
    
    zy_gpslocation.data.size = gpslocation->size;
    zy_gpslocation.data.flags = gpslocation->flags;  
	zy_gpslocation.data.latitude = gpslocation->latitude;
	zy_gpslocation.data.longitude = gpslocation->longitude;
	zy_gpslocation.data.altitude = gpslocation->altitude;
	zy_gpslocation.data.speed = gpslocation->speed;
	zy_gpslocation.data.bearing = gpslocation->bearing;
	zy_gpslocation.data.accuracy = gpslocation->accuracy;
	zy_gpslocation.data.timestamp = gpslocation->timestamp;
	
	    
    //GPS_LOGI("zy_gpslocation.latitude is %f  longitude is %f \r\n",zy_gpslocation.data.latitude,zy_gpslocation.data.longitude);
    memcpy(sendbuf,&zy_gpslocation,sizeof(zy_gpslocation));
    sendbuflen= sizeof(zy_gpslocation);
    send_gnss(sendbuf,sendbuflen);   //send data to tcpserver
    GPS_LOGI("do_send_GpsLocation ok ");
     
	return 0;
}

/*
 *	参数		:	char* nmea    I  nmea字符
 *                  int nmea_len  I  字符总长度
 * 	返回		:	int 0  发送成功
 * 	描述		:	发送NMEA_GGA(高精度定位) 数据到tcpserver
 * 	历史		:
 * 	备注		: 	()
 */
int do_send_gnss_NMEA(char* nmea,int nmea_len){
	
	send_gnss_nmea(nmea,nmea_len); //send data to tcpserver
	GPS_LOGI("do_send_gnss_NMEA ok ");
	return 0;
}

/*
 *	参数		:	char* nmea    I  nmea字符
 *                  int nmea_len  I  字符总长度
 * 	返回		:	int 0  发送成功
 * 	描述		:	发送NMEA_GGA(芯片原始定位) 数据到tcpserver
 * 	历史		:
 * 	备注		: 	()
 */

int do_send_NMEA(char* nmea,int nmea_len){
	
	
	send_nmea(nmea,nmea_len); //send data to tcpserver
	//GPS_LOGI("do_send_NMEA ok ");
	return 0;
}

/*
 *	参数		:	double*  sol    I    惯性导航结果数据
 *                  double*  fil    I    惯性导航结果数据的方差
 * 	返回		:	int 0  发送成功
 * 	描述		:	发送惯导解算结果 数据到tcpserver
 * 	历史		:
 * 	备注		: 	()
 */
int do_send_ins(double* sol,double* fil){
	
	char sendbuf[SEND_INS_SIZE];
	int sendlen;
	memset(sendbuf,0,SEND_INS_SIZE);
	memcpy(sendbuf,sol,sizeof(sol));
	memcpy(sendbuf+sizeof(sol),fil,sizeof(fil));
	sendlen=sizeof(sol) + sizeof(fil);
	GPS_LOGI("ins_send ok %s",sendbuf);
	send_ins(sendbuf,sendlen);  //send data to tcpserver
	return 0;
}

/*
 *	参数		:	INS_RESULT* ins_result    I 惯导解算结果结构体
 * 	返回		:	int 0  发送成功
 * 	描述		:	发送NMEA_GGA(惯导组合导航结果) 数据到tcpserver
 * 	历史		:
 * 	备注		: 	()
 */

int do_send_ins_NMEA(INS_RESULT* ins_result){
	char sendbuf[SEND_M_SIZE];
	int  send_len;
	iwise_loc_nmea_gga_s_t *nmea;
	nmea =(iwise_loc_nmea_gga_s_t *)malloc(sizeof(iwise_loc_nmea_gga_s_t));
	memset(nmea,0,sizeof(iwise_loc_nmea_gga_s_t));
	memset(sendbuf,0,SEND_M_SIZE);
	sol2nmea(nmea,ins_result);
	transnmea_gga(sendbuf,nmea);
	send_len =strlen(sendbuf);
	GPS_LOGI("ins_NMEA ok %s",sendbuf);
	send_ins_nmea(sendbuf,send_len);  //send data to tcpserver
	GPS_LOGI("do do_send_ins_NMEA ok");
	return 0;
}





