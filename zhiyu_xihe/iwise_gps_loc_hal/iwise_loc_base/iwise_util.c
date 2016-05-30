#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "iwise_util.h"
#include "iwise_loc_log.h"
#include "Parserlib/rtklib.h"
static obs_t* copy_obs(obs_t* obs){
	obs_t* o = malloc(sizeof(obs_t));
	o->n = obs->n;
	o->data = malloc(sizeof(obsd_t)*MAXOBS);
	memcpy(o->data, obs->data, sizeof(obsd_t)*(MAXOBS));
	return o;
}

static obs_t* copy_obs_fix(obs_t* obs){
	static int i=0;
	static obs_t * fix;
	if(i==0){
		
		fix = malloc(sizeof(obs_t));
		fix->data = malloc(sizeof(obsd_t)*MAXOBS);
		fix->n = obs->n;
		memcpy(fix->data, obs->data, sizeof(obsd_t)*MAXOBS);
		i = 1;
		
		int j =0;
		for(; j<fix->n; j++){
			fix->data[j].time.time = obs->data[j].time.time+1;
			fix->data[j].time.sec = obs->data[j].time.sec;
		}
		
		GPS_LOGD("return fix %d", fix->n);
		return fix;
	}else{
		
		obs_t* o = malloc(sizeof(obs_t));
		o->data = malloc(sizeof(obsd_t)*MAXOBS);
		o->n = fix->n;
		memcpy(o->data, fix->data, sizeof(obsd_t)*(MAXOBS));
		
		int j =0;
		for(; j<fix->n; j++){
			o->data[j].time.time = obs->data[j].time.time+1;
			o->data[j].time.sec = obs->data[j].time.sec;
		}
		GPS_LOGD("return copy %d\t%ld\t%ld", o->n, o->data[0].time.time, obs->data[0].time.time);
		return o;
	}
}


static obs_t* copy_obs_5_fix(obs_t* obs){
	int i=0;
	obs_t * fix;
	fix = malloc(sizeof(obs_t));
	fix->data = malloc(sizeof(obsd_t)*MAXOBS);
	memset((char*)(fix->data), 0, sizeof(obsd_t)*(MAXOBS));
	if(obs->n > 5){
		memcpy((char*)(fix->data), (char*)(obs->data)+sizeof(obsd_t)*(0), sizeof(obsd_t)*5);
		fix->n = 5;
	}else{
		memcpy((char*)(fix->data), (char*)(obs->data), sizeof(obsd_t)*obs->n);
		fix->n = obs->n;
	}
	return fix;
	
}

extern obs_t* checkobs(obs_t* obs)
{
	assert(obs != NULL);
	int n = obs->n;
	assert(n>=0);
	GPS_LOGD("[OBS] %d", n);
	
	memset((char*)(obs->data)+sizeof(obsd_t)*n, 0, sizeof(obsd_t)*(MAXOBS-n));
	/*test*/
	// change_sat_num(obs, 4);
	// change_sat_num(obs, 5);
	// change_sat_no(obs);
	// change_sat_time(obs);
	return obs;
}

/*--------------------------------------------------------------
 *函数名：updatenav
 *功能  ：更新卫星星历
 *参数  ：nav_t   *nav   I/O   广播卫星的星历
 *返回值：none 
 *--------------------------------------------------------------*/
extern void updatenav(nav_t *nav)
{
    int i,j;
    for (i=0;i<MAXSAT;i++) for (j=0;j<NFREQ;j++) {
        nav->lam[i][j]=satwavelen(i+1,j,nav);
    }
}

/*-----------------------------------------------------------------------------------
 *函数名 	:  weeksecond2gtime
 *功能  		:	将改正数中的周秒转换为gtime_t
 *参数    	: 	int       weeksecond2gtime    I  周秒
 *          	gtime_t   time                O  satposs函数所需要的时间格式
 *返回值  	: none 
 *注意    	: weeksecond 代指从某周星期天00：00 开始经过的秒数
 *           改正数中只有周秒没有周，默认可以认为GPS周是接受数据包时所在的周
 *           计算方法：得到当地时间，找到本周星期天00：00，往后推weeksecond秒
 *           weeksecond 和 本地时间可能还存在时区差
 *           如果计算得到的结果与当前时间差别超过1分钟可能需要加上8小时的时区差
 *------------------------------------------------------------------------------*/
extern void weeksecond2gtime(const int weeksecond, gtime_t * time_temp)
{ 
	#define SECOND_OF_WEEK	604800
    time_t timep;
    struct tm *tblock;
    int weeksecond1;
    
    time(&timep);
    tblock = gmtime(&timep);
    weeksecond1 = tblock->tm_wday*60*60*24 + tblock->tm_hour*60*60 + tblock->tm_min*60 + tblock->tm_sec;
	
    time_temp->time = timep-weeksecond1 + weeksecond;
    if(weeksecond - weeksecond1 > SECOND_OF_WEEK/2){
		time_temp->time -= SECOND_OF_WEEK;
	}else if(weeksecond - weeksecond1 < SECOND_OF_WEEK/2){
		time_temp->time += SECOND_OF_WEEK;
	}
    time_temp->sec =0;
}



/* -------------------------------------------------------------------------
 * fuction		:	cal_lon_lat_alt
 * description	:	计算卫星的经纬高
 * args			:	sol_t *sol		I	定位解算结果
 * 					double *lat		O	经度
 * 					double *lon		O	纬度
 * 					double *alt		O	高度
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
extern void cal_lon_lat_alt(sol_t *sol, double *lat, double *lon, double *alt)
{
	double pos[3] = {0}, Qe[9] = {0};
    ecef2pos(sol->rr, pos);
    covenu(pos, sol->qr, Qe);
    *lon = pos[1] * R2D;
    *lat = pos[0] * R2D;
    *alt = pos[2];
}

/* -------------------------------------------------------------------------
 * fuction		:	strftime_now
 * description	:	把时间转换成一定字符串格式
 * args			:	char *tbuf		O	储存一定格式的时间字符串
 * 					int length		I	缓冲区的长度
 * return		:	int (缓冲区数据的长度)
 * notes		:	该函数使用条件是系统时间设置为格林威治时间，使用的测试地点是东八区，
 * 					若是把系统时间设置为当地时间，则不需要进行加8个小时的运算
 *------------------------------------------------------------------------ */
extern int strftime_now(char *tbuf, int length)
{
	int len = 0;
	time_t t;
	struct tm * now = 0;
	
	time(&t);
    t+=8*60*60; //把格林威治时间加8个小时变成北京时间（东八区）
	now = localtime(&t);
	
	len = strftime(tbuf, length, "%F-%H-%M-%S", now);	
	
	return len;
}


/* -------------------------------------------------------------------------
 * fuction		:	strftime_now
 * description	:	把时间转换成一定字符串格式
 * args			:	char *tbuf		O	储存一定格式的时间字符串
 * 					int length		I	缓冲区的长度
 * return		:	int (缓冲区数据的长度)
 * notes		:	该函数使用条件是系统时间设置为格林威治时间，使用的测试地点是东八区，
 * 					若是把系统时间设置为当地时间，则不需要进行加8个小时的运算
 *------------------------------------------------------------------------ */
extern int strftime_hour(char *tbuf, int length)
{
	int len = 0;
	time_t t;
	struct tm * now = 0;
	
	time(&t);
    t+=8*60*60; //把格林威治时间加8个小时变成北京时间（东八区）
	now = localtime(&t);
	
	len = strftime(tbuf, length, "%H-%M-%S", now);	
	
	return len;
}


