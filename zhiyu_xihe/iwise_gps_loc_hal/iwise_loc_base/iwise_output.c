#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "Parserlib/rtklib.h"
#include "Parserlib/gps_common.h"
#include "iwise_output.h"
#include "iwise_util.h"
#include "iwise_loc_base.h"
#include "iwise_serial_parser.h"
#include "iwise_nmea_parser.h"
#include "iwise_loc_log.h"

static unsigned long computer_count = 0; /* 定位计算次数 */ 
static int obs_eph_count = 0;

/* -------------------------------------------------------------------------
 * fuction		:	output_one_eph
 * description	:	记录一个卫星的星历于一个文件
 * args			:	FILE *fp		I	记录日志的文件指针
 * 					eph_t *eph		I	gps卫星导航星历
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
static void output_one_ephf(FILE *fp, eph_t *eph)
{
	fprintf(fp, "++++++ satno %d\t iode %d\t iodc %d\n", eph->sat, eph->iode, eph->iodc);
    fprintf(fp, "      sva   %d\t svh  %d\t week %d\n", eph->sva, eph->svh, eph->week);
    fprintf(fp, "      code  %d\t flag %d\n", eph->code, eph->flag);
    fprintf(fp, "      toe  {%ld, %lf}\t toc {%ld, %lf}\t ttr {%ld, %lf}\n", eph->toe.time,eph->toe.sec, eph->toc.time,eph->toc.sec, eph->ttr.time,eph->ttr.sec);
    fprintf(fp, "      A  %20.17lf\t e %20.17lf\t i0 %20.17lf\t OMG0 %20.17lf omg %20.17lf\n", eph->A, eph->e, eph->i0, eph->OMG0, eph->omg);	
    fprintf(fp, "      M0 %lf\t deln  %20e\t OMGd %20e\t idot %20e\n", eph->M0, eph->deln, eph->OMGd, eph->idot);
    fprintf(fp, "      crc  %20e\t crs %20e\t cuc %20e\t cus %20e\t cic %20e\t cis %20e\n", eph->crc, eph->crs, eph->cuc, eph->cus, eph->cic, eph->cis);
    fprintf(fp, "      toes  %lf\t fit %lf\t f0 %20e\t f1 %20e\t f2 %20e\n", eph->toes, eph->fit, eph->f0, eph->f1, eph->f2);
    fprintf(fp, "      tgd   %20e\t%20e\t%20e\t%20e\n", eph->tgd[0], eph->tgd[1], eph->tgd[2], eph->tgd[3]);
    fflush(fp);	
}

/* -------------------------------------------------------------------------
 * fuction		:	output_obs2eph2ssat
 * description	:	记录所有卫星星历于同一个文件
 * args			:	FILE *fp		I	记录日志的文件指针
 * 					nav_t *nav		I	卫星导航星历
 * 					int n			I	导航星历卫星的个数
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
 extern void output_ephf(FILE *fp, nav_t *nav, int n)
{
	char tbuf[TIME_BUFFER_SIZE] = {0};
	int	len = 0;
	
	len = strftime_now(tbuf, TIME_BUFFER_SIZE);
	
	fprintf(fp, "-------------%s-------------------------------------------------\n", tbuf);
	int i;
	for(i = 0; i < n; i++)
	{
		if((nav->eph[i].sat) == 0 || (nav->eph[i].sat >= 32)) continue;
		fprintf(fp, "---------system all number %d\n", nav->n);
		output_one_ephf(fp, &(nav->eph[i]));		
	}	
}

/* -------------------------------------------------------------------------
 * fuction		:	output_obs2eph2ssat
 * description	:	同时记录卫星观测值，卫星星历于同一个文件
 * args			:	FILE *fp		I	记录日志的文件指针
 * 					obsd_t *obsd	I	单个卫星观测值
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
static void output_obsdf(FILE *fp, obsd_t *obsd)
{
	
	fprintf(fp, "++++++sat   %d\t rcv %d\n", obsd->sat, obsd->rcv);
    fprintf(fp, "      GPST   {%ld, %lf}\n", obsd->time.time, obsd->time.sec);
    fprintf(fp, "      SNR   %f\t%f\t%f\n", (obsd->SNR[0]) * 0.25, (obsd->SNR[1]) * 0.25, (obsd->SNR[2]) * 0.25);
    fprintf(fp, "      LLI   %d\t%d\t%d\n", obsd->LLI[0], obsd->LLI[1],obsd->LLI[2]);
    fprintf(fp, "      code  %d\t%d\t%d\n", obsd->code[0], obsd->code[1], obsd->code[2]);
    fprintf(fp, "      L     %lf\t%lf\t%lf\n", obsd->L[0], obsd->L[1], obsd->L[2]);
    fprintf(fp, "      P     %lf\t%lf\t%lf\n", obsd->P[0], obsd->P[1], obsd->P[2]);
    fprintf(fp, "      D     %f\t%f\t%f\n", obsd->D[0], obsd->D[1], obsd->D[2]);
    fflush(fp);
}

/* -------------------------------------------------------------------------
 * fuction		:	output_obsf
 * description	:	记录卫星所有观测值同一个文件
 * args			:	FILE *fp		I	记录日志的文件指针
 * 					obs_t *obs		I	卫星观测值
 * 					int n			I	卫星观测值的个数
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
extern void output_obsf(FILE *fp, obs_t *obs, int n)
{
	char tbuf[TIME_BUFFER_SIZE] = {0};
	int	len = 0;
	
	len = strftime_now(tbuf, TIME_BUFFER_SIZE);
	
	fprintf(fp, "-------------%s------------------------------------------------\n", tbuf);
	int i;
	for(i = 0; i < n; i++)
	{
		if((obs->data[i].sat == 0) || (obs->data[i].sat >= 32)) continue;
		fprintf(fp, "---------system all number %d\n", obs->n);
		output_obsdf(fp, &(obs->data[i]));		
	}	
}

/* -------------------------------------------------------------------------
 * fuction		:	output_error
 * description	:	记录卫星定位错误消息于同一个文件
 * args			:	FILE *fp		I	记录错误日志的文件指针
 *					char *error_buf	I	错误消息缓冲区
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
extern void output_errorf(int fd, obs_t * obs, iwise_clockorbit_t *clockorbit, char* error_buf)
{
    char buffer[1024] = {0};
    gtime_t gt1;
    char tmp[10] = {0};
    char buf[150] = {0};
    char *ptr = NULL;
    int len = 0;
    int len1 = 0;
	
	len1 = strftime_now(buffer, 1024);
    len = len1;
    
    len1 = sprintf(buffer + len," %ld", obs->data[0].time.time);//observation time
    len = len + len1;
    ptr = strcat(buffer, " ");
    len = len +1;

    weeksecond2gtime(clockorbit->GPSEpochTime, &gt1); //cloclorbit time 
    len1 = sprintf(buffer + len," %ld", gt1.time);//observation time
    len = len + len1;
    ptr = strcat(buffer, " ");
    len++;
    
    sprintf(tmp, "%2d ", obs->n);  //star number
    ptr = strcat(buffer, tmp);
    len = len + strlen(tmp);
    
    len1 = sprintf(buffer + len, " %s\n", error_buf);
    len = len + len1;
    write(fd, buffer, len);
}

/* -------------------------------------------------------------------------
 * fuction		:	output_ssatf
 * description	:	记录卫星状态消息于同一个文件
 * args			:	FILE *fp		I	记录日志的文件指针
 * 					ssat_t *ssat	I	卫星状态消息
 * 					int n			I	卫星号
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
static void output_one_ssatf(FILE *fp, ssat_t *ssat, int n)
{
    fprintf(fp, "++++++ satno %d sys   %d\tvs %d\n",n, ssat->sys, ssat->vs);
    fprintf(fp, "      az    %7.4f\tel %7.4f\n", (ssat->azel[0])*R2D, (ssat->azel[1])*R2D);
    fprintf(fp, "      resp  %lf\t%lf\t%lf\n", ssat->resp[0], ssat->resp[1], ssat->resp[2]);
    fprintf(fp, "      resc  %lf\t%lf\t%lf\n", ssat->resc[0], ssat->resc[1], ssat->resc[2]);
    fprintf(fp, "      vsat  %d\t%d\t%d\n", ssat->vsat[0], ssat->vsat[1], ssat->vsat[2]);
    fprintf(fp, "      snr   %d\t%d\t%d\n", ssat->snr[0] / 4, ssat->snr[1] / 4, ssat->snr[2] / 4);
    fprintf(fp, "      fix   %d\t%d\t%d\n", ssat->fix[0], ssat->fix[1], ssat->fix[2]);
    fprintf(fp, "      slip  %d\t%d\t%d\n", ssat->slip[0], ssat->slip[1], ssat->slip[2]);
    fprintf(fp, "      lock  %d\t%d\t%d\n", ssat->lock[0], ssat->lock[1], ssat->lock[2]);
    fprintf(fp, "      outc  %d\t%d\t%d\n", ssat->outc[0], ssat->outc[1], ssat->outc[2]);
    fprintf(fp, "      slipc %d\t%d\t%d\n", ssat->slipc[0], ssat->slipc[1], ssat->slipc[2]);
    fprintf(fp, "      rejc  %d\t%d\t%d\n", ssat->rejc[0], ssat->rejc[1], ssat->rejc[2]);
    fprintf(fp, "      gf %lf\tgf2 %lf\tphw %lf\n", ssat->gf, ssat->gf2, ssat->phw);
    //fprintf(fp, ); //pt
    //fprintf(fp, ); //ph
    fflush(fp);  
}

/* -------------------------------------------------------------------------
 * fuction		:	output_obs2eph2ssat
 * description	:	记录所有状态消息于同一个文件
 * args			:	FILE *fp		I	记录日志的文件指针
 * 					sol_t * sol		I	定位解算结果
 * 					obs_t *obs		I	卫星观测数据
 * 					ssat_t *ssat	I	卫星状态消息
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
extern void output_ssatf(FILE *fp, sol_t * sol, obs_t *obs, ssat_t *ssat)
{
	char tbuf[TIME_BUFFER_SIZE] = {0};
	int	len = 0;
	
	len = strftime_now(tbuf, TIME_BUFFER_SIZE);

    fprintf(fp, "----------------------------star informations--------------------------------\n");
    fprintf(fp, "%s\t%ld\t%d\t%d\n", tbuf, obs->data[0].time.time, sol->ns, sol->type);
    int i = 0;
    for(; i < obs->n; i++)
    {
		if((obs->data[i].sat == 0) || (obs->data[i].sat > 32)) continue;
		output_one_ssatf(fp, &(ssat[obs->data[i].sat - 1]), obs->data[i].sat);
    }
    fflush(fp);
    return;
}

/* -------------------------------------------------------------------------
 * fuction		:	output_lon_lat_altf
 * description	:	记录经纬度
 * args			:	FILE *fp		I	记录日志的文件指针
					*sol_t			I   解算结果
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
extern void output_lon_lat_altf(FILE *fp, sol_t *sol)
{	
	double lon = 0.0; //经度
	double lat = 0.0; //纬度
	double alt = 0.0; //高度
	// 根据sol计算经纬度
	cal_lon_lat_alt(sol, &lon, &lat, &alt);

	fprintf(fp, "%ld, %10lf, %10lf, %5lf\n", sol->time.time, lon, lat, alt);  //edit by zhuyong 2015.12.11 for debug sol.time
	fflush(fp);
}

/* -------------------------------------------------------------------------
 * fuction		:	output_obs2eph2ssat
 * description	:	同时记录卫星观测值，卫星星历于同一个文件
 * args			:	FILE *fp		I	记录日志的文件指针
 * 					obs_t *obs		I	卫星观测值
 * 					nav_t *nav		I	卫星导航星历
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
extern int output_resultf(int fd, iwise_clockorbit_t *clockorbit, obs_t *obs, nav_t *nav, sol_t *sol, char *error_buffer)
{
	char buffer[1024] = {0};
    gtime_t gt1;
    char tmp[10] = {0};
    char buf[150] = {0};
    char *ptr = NULL;
    int len = 0;
    int len1 = 0;
	
	len1 = strftime_hour(buffer, 1024);
    len = len1;
    
    if(obs != NULL) {
		len1 = sprintf(buffer + len,"  %ld", obs->data[0].time.time);//observation time
		len = len + len1;
		ptr = strcat(buffer, " ");
		len = len +1;
	}
	if(clockorbit != NULL) {
		weeksecond2gtime(clockorbit->GPSEpochTime, &gt1); //cloclorbit time 
		len1 = sprintf(buffer + len," %ld", gt1.time);//observation time
		len = len + len1;
		ptr = strcat(buffer, " ");
		len++;
    }

	if(obs!= NULL) {
		sprintf(tmp, "%2d ", obs->n);  //star number
		ptr = strcat(buffer, tmp);
		len = len + strlen(tmp);

	}
	if(error_buffer != NULL) {
		len1 = snprintf(buffer + len, strlen(buffer) + 3, " %s ", error_buffer);
		len = len + len1;
	}

	if(sol != NULL) {
		double lon = 0.0;
		double lat = 0.0;
		double hgt = 0.0;
		cal_lon_lat_alt(sol, &lat, &lon, &hgt);

		len1 = sprintf(buffer + len, "%lf  %lf ", lon, lat);
		len = len + len1;
		ptr = strcat(buffer, " ");

		outnmea_gga(buf, sol); //gga
		len = len + strlen(buf);
		strcat(buffer, buf);
		strcat(buffer, " "); 
		len++; 
	} else {
		strcat(buffer, "\n");
		len = len + strlen("\n");
	}

	int ret = write(fd, buffer, len);
	if(ret < 0) {
		perror("write result:");
		return -1;
	}
    return 0;
}

/* -------------------------------------------------------------------------
 * fuction		:	output_rawf
 * description	:	同时记录卫星观测值,星历
 * args			:	FILE *fp				I	记录日志的文件指针
 * 				:	const void *read_buf	I	缓冲区
 * 					size_t count			I	缓冲区数据个数
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
extern int output_rawf(int fd, const void *read_buf, size_t count)
{
	int ret = -1;
	
	ret = write(fd, read_buf, count);
	if(ret < 0) {
	
	}
	
	return ret;
}

/* -------------------------------------------------------------------------
 * fuction		:	output_clockorbitf
 * description	:	同时记录改正数
 * args			:	FILE *fp				I	记录日志的文件指针
 * 				:	const void *read_buf	I	缓冲区
 * 					size_t count			I	缓冲区数据个数
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
extern int output_clockorbitf(int fd, const void *read_buf, size_t count)
{
	int ret = -1;
	
	ret = write(fd, read_buf, count);
	if(ret < 0) {
	
	}
	
	return ret;
}

/** 比较原始观测数据和卫星的星历的卫星号，并把有对应有原始观测数据和卫星的星历的数据记录到文件里去*/
/* -------------------------------------------------------------------------
 * fuction		:	output_obs2eph
 * description	:	同时记录卫星观测值，卫星星历于同一个文件
 * args			:	FILE *fp		I	记录日志的文件指针
 * 					obs_t *obs		I	卫星观测值
 * 					nav_t *nav		I	卫星导航星历
 * return		:	
 * notes		:
 *------------------------------------------------------------------------ */
extern void output_obs2eph(FILE *fp, obs_t *obs, nav_t *nav)
{
    int i, j;
    int m = 0, len = 0;

    char tbuf[100] = {0};

	len = strftime_now(tbuf, TIME_BUFFER_SIZE); //system time
	fprintf(fp, "######### %s ##########################\n", tbuf );
    //fprintf(fp, "######### %s #########compute count = %d #################\n", buf , ++obs_eph_count);
    for(i = 0; i < obs->n; i++) 
    {
        if(obs->data[i].sat == 0) continue;
        for(j = 0; j < nav->n; j++)
        {	
            if(nav->eph[j].sat == 0) continue;
            if(obs->data[i].sat == nav->eph[j].sat) //卫星号是否相等
            {
                fprintf(fp, "--------------------sat count = %d --------------------------------\n", ++m);
		        output_obsdf(fp, &(obs->data[i]));		
		        output_one_ephf(fp, &(nav->eph[j]));		
            }
        }
    }
}

/* -------------------------------------------------------------------------
 * fuction		:	output_obs2eph2ssat
 * description	:	同时记录卫星观测值，卫星星历和卫星状态信息于同一个文件
 * args			:	FILE *fp		I	记录日志的文件指针
 * 					obs_t *obs		I	卫星观测值
 * 					nav_t *nav		I	卫星导航星历
 * 					ssat_t *ssat 	I	卫星状态信息
 * return		:
 * notes		:
 *------------------------------------------------------------------------ */
extern void output_obs2eph2ssat(FILE *fp, obs_t *obs, nav_t *nav, ssat_t *ssat)
{
    int i, j;
    int m = 0, len =  0;
    char tbuf[TIME_BUFFER_SIZE] = {0};
    
	len = strftime_now(tbuf, TIME_BUFFER_SIZE); //system time
	
    fprintf(fp, "######### %s #########compute count = %d #################\n", tbuf , ++obs_eph_count);
    for(i = 0; i < obs->n; i++) 
    {
        if(obs->data[i].sat == 0) continue;
        for(j = 0; j < nav->n; j++)
        {	
            if(nav->eph[j].sat == 0) continue;
            if(obs->data[i].sat == nav->eph[j].sat) //卫星号是否相等
            {
                fprintf(fp, "--------------------sat count = %d --------------------------------\n", ++m);
		        output_obsdf(fp, &(obs->data[i]));		
		        output_one_ephf(fp, &(nav->eph[j]));
		        output_one_ssatf(fp, &(ssat[obs->data[i].sat - 1]), obs->data[i].sat);		
            }
        }
    }
}

extern int output_restoref(int fd, obs_t *obs, iwise_clockorbit_t *clockorbit, char *error_buf)
{
	char buffer[1024] = {0};
    gtime_t gt1;
    char tmp[10] = {0};
    char buf[150] = {0};
    char *ptr = NULL;
    int len = 0;
    int len1 = 0;
	int i = 0;
	
	len1 = strftime_hour(buffer, 1024);
    len = len1;
    
    len1 = sprintf(buffer + len," %ld", obs->data[0].time.time);//observation time
    len = len + len1;
    ptr = strcat(buffer, " ");
    len = len +1;

    weeksecond2gtime(clockorbit->GPSEpochTime, &gt1); //cloclorbit time 
    len1 = sprintf(buffer + len," %ld", gt1.time);//cloclorbit time
    len = len + len1;
    ptr = strcat(buffer, " ");
    len++;
    
    sprintf(tmp, "%2d ", obs->n);  //star number
    ptr = strcat(buffer, tmp);
    len = len + strlen(tmp);
    
        
    for(i = 0; i < obs->n; i++)
	{
		//if((obs->data[i].sat == 0) || (obs->data[i].sat > 32)) continue;
		len1 = sprintf(buffer + len , "%d, ", obs->data[i].sat);	
		len = len + len1;
	}	

    len1 = sprintf(buffer + len, " %s", error_buf);
    len = len + len1;

    write(fd, buffer, len);	
    
    return 0;
}

/*转换经纬度---------------------------------------*/
static double __convert_lat_lon(double latlon) 
{
	int deg;
	double min;

	deg = (int) latlon / 100;
	min = latlon - (deg * 100);
	return (min / 60 + deg);
}

extern int output_nmea_gga(int fd, iwise_nmea_parser_t *nmea)
{
	char buf[150]= {0};
	int ret = 0,len = 0;
	len = sprintf(buf, "%lf, %lf, %f\n", __convert_lat_lon(nmea->nmea_gga_info.latitude), 
				__convert_lat_lon(nmea->nmea_gga_info.longitude), nmea->nmea_gga_info.altitude);
				
	ret = write(fd, buf, len);
	if(ret < 0) {
		GPS_LOGE("output gga to file failed");
		return -1;
	}
	return 0;
}

extern int output_rmcf(int fd, iwise_nmea_parser_t *nmea)
{
	char buf[150]= {0};
	int ret = 0,len = 0;
	len = sprintf(buf, "%lf, %lf, %f\n", nmea->nmea_rmc_info.latitude, 
				nmea->nmea_rmc_info.longitude, 0.0);
				
	ret = write(fd, buf, len);
	if(ret < 0) {
		GPS_LOGE("output gga to file failed");
		return -1;
	}
	return 0;
}

/* -------------------------------------------------------------------------
 * fuction		:	open_fd
 * description	:	open the file path and return this file descriptor
 * args			:	char *fileName	I	file path or file name
 * return		:	file descriptor (0=<:ok; 0>:error)
 * notes		:
 *------------------------------------------------------------------------ */
extern int open_fd (char *fileName)
{
    int fd;
    int len = 0;
    
    char fileSt[MAXFILENAME]= LOG_BASE_PATH;
    char * fileStr = fileSt;
    char tbuf[TIME_BUFFER_SIZE] = "";

	len = strftime_now(tbuf, TIME_BUFFER_SIZE);
	sprintf(fileStr, "%s%s/%s_%s%s.log",
			LOG_BASE_PATH, GPS_LOG_PATH, GPS_LOG_PATH, fileName, tbuf);
			
    fd = open(fileStr, O_WRONLY|O_CREAT|O_NONBLOCK,06666);
    if(fd < 0) {
        GPS_LOGE("open %s failed", fileStr);
    } else {
		GPS_LOGD("open %s success", fileStr);
	}

    return fd;
}

/* -------------------------------------------------------------------------
 * fuction		:	open_fp
 * description	:	open the file path and return this FILE pointer
 * args			:	char *fileName		I	file path or file name
 * return		:	FILE pointor(not NULL:ok; NULL:error)
 * notes		:
 *------------------------------------------------------------------------ */
extern FILE * open_fp(char *fileName)
{
    FILE *fp;
    int fd = open_fd(fileName);
    fp = fdopen(fd, "w"); 
    return fp;
}

extern void output_log(FILE* log_file_fp, char *buffer, unsigned int count)
{
	int ret = 0;
	char msg[1024];
	memset(msg, 0, 1024);
	ret = sprintf(msg, "%s:%d\t:", __FILE__, __LINE__);
	
	strcat(msg, buffer);
	fprintf(log_file_fp, "%s\n", msg);
	fflush(log_file_fp);
}

/* 日志的初始化 */
extern int iwise_log_file_init(iwise_log_file_t *log_context_file)
{
	int ret = -1;
#if defined GPS_DRIVER_LOG
	log_context_file->gps_log_fp = open_fp("gps_log");
	if(log_context_file->gps_log_fp == NULL) {
		GPS_LOGE("open gps_log success");
	}
#endif

#if defined HEXIN
	log_context_file->hexin_fd = open_fd("hexin");
	if(log_context_file->raw_fd < 0) {
		GPS_LOGE("open hexin_fd failed!");
		return -1;
	}
#endif

#if defined WRITE_LOG
	log_context_file->raw_fd = open_fd("raw");
	if(log_context_file->raw_fd < 0) {
		GPS_LOGE("open raw_fd failed!");
		return -1;
	}
	
	log_context_file->clock_orbit_fd = open_fd("clockorbit");
	if(log_context_file->clock_orbit_fd < 0) {
		GPS_LOGE("open clockorbit_fd failed!");
		return -1;
	}
	
	log_context_file->longlat_fp = open_fp("longlat");
	if(log_context_file->longlat_fp == NULL) {
		GPS_LOGE("open longlat_fp failed!");
		return -1;
	}
	
	log_context_file->restore_fd = open_fd("restore");
	if(log_context_file->restore_fd < 0) {
		GPS_LOGE("open clockorbit_fd failed!");
		return -1;
	}
	
	log_context_file->nmea_fd = open_fd("nmea");
	if(log_context_file->nmea_fd < 0) {
		GPS_LOGE("open clockorbit_fd failed!");
		return -1;
	}
	
	log_context_file->eo_fp = open_fp("eph_obs");
	if(log_context_file->eo_fp == NULL) {
		GPS_LOGE("open longlat_fp failed!");
		return -1;
	}
	
	log_context_file->eph_fp = open_fp("eph");
	if(log_context_file->eph_fp == NULL) {
		GPS_LOGE("open eph_fp failed!");
		return -1;
	}
#endif
	GPS_LOGD("open log file success!");
	return 0;
}

/* 日志的销毁*/
extern void iwise_log_file_destroy(iwise_log_file_t *log_context_file)
{
#if defined WRITE_LOG
	close(log_context_file->raw_fd);
	close(log_context_file->clock_orbit_fd);
	fclose(log_context_file->longlat_fp);
	close(log_context_file->restore_fd);
	close(log_context_file->nmea_fd);
	fclose(log_context_file->eo_fp);
	fclose(log_context_file->eph_fp);
#endif

#if defined HEXIN
    close(log_context_file->hexin_fd);
#endif

#if defined GPS_DRIVER_LOG
	fclose(log_context_file->gps_log_fp);
#endif
}

