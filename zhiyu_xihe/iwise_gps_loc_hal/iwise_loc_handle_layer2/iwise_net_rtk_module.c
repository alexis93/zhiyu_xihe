#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "iwise_loc_base/iwise_loc_log.h"
#include "iwise_loc_base/Parserlib/rtklib.h"
#include "iwise_loc_base/Parserlib/gps_common.h"
//#include "iwise_serial_parser.h"
//#include "iwise_nmea_parser.h"
#include "iwise_loc_hardware_layer3/iwise_loc_hardware.h"
#include "iwise_net_rtk_module.h"
//#include "iwise_novatel.h"
//#include "genGGA.h"
static iwise_net_rtk_context_t rtk_context;
pthread_t t_net_rtk;

stream_t stream_ntrip;
#define MAX_BUFFER 4096
unsigned char ntrip_buf[MAX_BUFFER] = {0};
int ntrip_len;
//ntrip_t *ntrip = NULL;
#define NMEABUF_LEN 1024
unsigned char nmea_buf[NMEABUF_LEN];
int nmea_len=0;

//shanghai1
//char *path = "2fy425jp:jpzou@202.136.213.10:9901/RTCM3";
//shanghai2
//char *path = "b:b@202.136.208.106:2102/RTCM3MSM";
//char *path = "b:b@211.144.118.5:2102/CMR";
//char *path = "b:b@211.144.118.5:2102/RTCM3MSM";

//wuhan
//char *path = "test:123@121.40.164.151:2101/RTCM3";  /* user:passwd@address:port/mountpoint*/ /* pc ip*//* 运行了POWERNET软件的pc ip*/

//wuxi
//char *path = "test:1234@61.177.142.154:2104/RTCM-COM";

//deqing
char *path = "test:123@115.231.245.222:2101/RTCM3";  /* user:passwd@address:port/mountpoint*/ /* pc ip*//* 运行了POWERNET软件的pc ip*/

//char *path = "test:123@115.231.245.222:2101/RTCM3";  /* user:passwd@address:port/mountpoint*/ /* pc ip*//* 运行了POWERNET软件的pc ip*/
//char *path = "test:123@192.168.31.153:2101/RTCM3";  /* user:passwd@address:port/mountpoint*/ /* pc ip*//* 运行了POWERNET软件的pc ip*/


char msg[1024];
int compos_fd;

void set_pos_netrtk(unsigned char* nmea,int len){
	//pthread_mutex_lock(&rtk_context.data_mutex);
	memset(nmea_buf,0,NMEABUF_LEN);
	//sprintf((char*)nmea_buf,(char*)"$GPGGA,061408.00,3021.1260000,N,11415.7620000,E,2,08,1.0,90.000,M,0.000,M,0.0,*4C\n\r",84);
	//nmea_len=84;
	sprintf((char*)nmea_buf,(char*)nmea,len);
	nmea_len=len;

	rtk_context.nmea_flag=1;
	//pthread_mutex_unlock(&rtk_context.data_mutex);
}

void set_compos_fd(int fd){
	compos_fd=fd;
	rtk_context.com_flag=1;
}

static void * netrtk_handle(void * arg){
	int ntrip_status=0;
	int status;
	int temp_status;
	int len;
	int cnt=0;
	strinit(&stream_ntrip); //初始化ntrip流
	while(1){
		if ((rtk_context.com_flag==1)&&(rtk_context.nmea_flag==1)) break;
		sleep(1);
		/*if (rtk_context.com_flag==1) {
			write(compos_fd,"hello_world!",12);
		}*/
	}
	status = write(compos_fd, "interfacemode com1 auto auto on\n\r",33);
	while(1){
		if (ntrip_status<=0) {
			temp_status=stropen(&stream_ntrip, STR_NTRIPCLI, STR_MODE_RW, path);//创建ntrip_t
			if (temp_status==1) {
				ntrip_status=1;
				len=strread(&stream_ntrip,ntrip_buf,MAX_BUFFER);

				//pthread_mutex_lock(&rtk_context.data_mutex);
				strwrite(&stream_ntrip,nmea_buf,nmea_len);
				//pthread_mutex_unlock(&rtk_context.data_mutex);
				printf("open ntrip succuss!\n");
			} else {
				ntrip_status=0;
				printf("open ntrip error!\n");
			}
			sleep(1);
		} else {
			ntrip_len=strread(&stream_ntrip,ntrip_buf,MAX_BUFFER);
			//printf("rep[%d]:%s\n\r",ntrip_len,&ntrip_buf);
			status=write(compos_fd,ntrip_buf,ntrip_len);
			cnt++;
			//printf("send com1 %d,status=%d \n\r",ntrip_len,status);
			if (cnt>=5) {
				//pthread_mutex_lock(&rtk_context.data_mutex);
				strwrite(&stream_ntrip,nmea_buf,nmea_len);
				//status=write(compos_fd,"hello_world!",12);

				//pthread_mutex_unlock(&rtk_context.data_mutex);
				cnt=0;
			}
			if (strstat(&stream_ntrip,msg)<=0) {
				ntrip_status=0;
				strclose(&stream_ntrip);
			}
			sleep(1);
		}
	}

	return 0;
}

int iwise_net_module_init(){

	pthread_mutex_init(&rtk_context.data_mutex, NULL);
	pthread_cond_init(&rtk_context.net_cond, NULL);

	if(pthread_create(&t_net_rtk, NULL, netrtk_handle, (void *)&compos_fd) == 0){
		GPS_LOGD("create netrtk thread success !");
		//printf("create pthread success\n");
		return 1;
	}else {
		GPS_LOGE("create netrtk thread failed !");
		//perror("create thread failed");
		return 0;
	}
};

int iwise_net_module_destroy(){
	return 1;
};
