/*-----------------------------------------------------------------
 *	iwise_pos_module.h :
 *
 *	Copyright (C) 2014-2015 by iwise. All rights reserved.
 *
 * 	version	:
 * 	history	: 	2015-05-18	created by chenxi
 * 				2015-06-11	add notes and comments by lanzhigang
 *-------------------------------------------------------------------*/
#pragma once

//#include <pthread.h>
//#include "iwise_loc_base/iwise_loc_base.h"
#include "rtklib.h"
//#include "iwise_loc_base.h"
//rtk_t *rtk_temp;
/*
 * 解算模块的上下文结构体
 * */

#ifdef __cplusplus
extern "C" {
#endif

extern rtk_t rtk_temp;   //

//extern obs_t obs_ppp;
//extern nav_t nav_ppp;
//extern clockorbit_t clockorbit_ppp;
//extern sol_t sol_ppp;
//extern ssat_t ssat_ppp;

typedef struct{
	//pthread_t	pos_thread;
	char		ctrl_file_path[100];
}iwise_pos_context_t;


/*
 * 解算定位函数
 */
extern int pnt_pos_init(const char* param);
extern int pnt_pos_destroy();
//extern int pnt_pos_sol(const iwise_gnss_obs_t *obs, const iwise_gnss_nav_t *nav,
//	const prcopt_t *opt, const iwise_clockorbit_t * clockorbit,
//	iwise_sol_t *sol, iwise_ssat_t *ssat, char *msg);
extern int pnt_pos_sol(const obs_tMEP *obs, const nav_t *nav, const prcopt_t *opt, const clockorbit_t * clockorbit,
	sol_t *sol, ssat_t *ssat, char *msg);
/*
 * 初始化解算模块
 * 在该初始化函数中会启动解算线程
 * */
//int iwise_pos_module_init();
//int iwise_pos_module_destroy();


#ifdef __cplusplus
}
#endif
