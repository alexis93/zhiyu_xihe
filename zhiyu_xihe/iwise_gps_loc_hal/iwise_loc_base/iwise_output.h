#ifndef __IWISE_OUTPUT_LOG_H___
#define __IWISE_OUTPUT_LOG_H___

#include "iwise_loc_base.h"
#include "iwise_serial_parser.h"
#include "iwise_nmea_parser.h"
/*
 * DATA宏控制log日志的存放位置，若定义可DATA,则存放在/data/log/
 * 否则存放在/storage/sd_internal/log/
 * 如果是驱动，则日志文件只能放在/data/log/，若放在storage则会失败
 * 
 * UBLOX宏控制程序编译的是ublox驱动
 * 
 * 
 * WRITE_LOG宏控制驱动是否记录日志，若定义，则打开记录日志
 * 
 * */
//#define DATA
//#define UBLOX 
//#define WRITE_LOG
//#define GPS_DRIVER_LOG

#ifdef DATA
#define LOG_BASE_PATH "/data/log/"
#else
#define LOG_BASE_PATH "/storage/sd_internal/log/"
#endif

#if defined UBLOX
#define GPS_LOG_PATH "ublox"
#elif defined NOVATEL
#define GPS_LOG_PATH "novatel"
#elif  defined HEXIN
#define GPS_LOG_PATH "hexin"
#endif

/*--------constants----------------------------------------------------------*/
#define MAXFILENAME   		250	/* 文件名的最大长度 */
#define TIME_BUFFER_SIZE 	512	/* 时间缓冲区的长度 */
#define LOG_BUF_SIZE  1024

/* 记录日志的结构*/
typedef  struct {
#if defined WRITE_LOG
	int		raw_fd;	/* 原始观测数据的fd*/
	int		clock_orbit_fd;	/* 钟差改正数的fd*/
	FILE*	longlat_fp;		/* 经纬度 */
	int		restore_fd;		/* 记录其他有用的信息的fd */
	int		nmea_fd;		/* 记录nmea的fd */
	FILE*	eo_fp;		/* eph_obs */
	FILE*	eph_fp;		/*eph_fp */
#endif

#if defined HEXIN 
    int     hexin_fd;
#endif

#if defined GPS_DRIVER_LOG
	FILE*	gps_log_fp;
#endif

}iwise_log_file_t;

/*--fuctions--------------------------------------------------------------*/
extern int open_fd (char *fileName);
extern FILE * open_fp(char *fileName);

extern void output_obs2eph2ssat(FILE *fp, obs_t *obs, nav_t *nav, ssat_t *ssat);
extern void output_obs2eph(FILE *fp, obs_t *obs, nav_t *nav);
extern int output_resultf(int fd, iwise_clockorbit_t *clockorbit, obs_t *obs, nav_t *nav, 
							sol_t *sol, char *error_buffer);
extern void output_ssatf(FILE *fp, sol_t * sol, obs_t *obs, ssat_t *ssat);
extern void output_errorf(int fd, obs_t * obs, iwise_clockorbit_t *clockorbit, char* error_buf);
extern void output_obsf(FILE *fp, obs_t *obs, int n);
extern void output_ephf(FILE *fp, nav_t *nav, int n);
extern void output_lon_lat_altf(FILE *fp, sol_t *sol);
extern int output_restoref(int fd, obs_t *obs, iwise_clockorbit_t *clockorbit, char *error_buf);
extern int output_rmcf(int fd, iwise_nmea_parser_t *nmea_parser);
extern int output_ggaf(int fd, iwise_nmea_parser_t *nmea);
/* 日志的初始化 */
extern int iwise_log_file_init(iwise_log_file_t *log_context);
/* 日志的销毁 */
extern void iwise_log_file_destroy(iwise_log_file_t *log_context);

extern void output_log(FILE* log_file_fp, char *buffer, unsigned int count);
#endif /* iwise_output.h */
