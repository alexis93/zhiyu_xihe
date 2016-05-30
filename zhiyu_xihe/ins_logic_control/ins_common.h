/*********************************************************************************
 * Copyright(C), 2014-2015 zhiyu gnss. // 版权声明
 * File name:  ins_common.h // 文件名
 * Author:   gnss  // 作者
 * Version:   2.0 // 版本
 * Date:  2015.4.20 // 完成日期
 * Description:      头文件 存储相关数据结构以及函数声明
 * Function List:   // 主要函数的列表，每条记录应包括函数名及功能简要说明
 * History:
 * 2015.4.15 gnss add struct  CFG.
 * 修改历史，包括每次修改的日期、修改者和修改内容简述
 *********************************************************************************/

#ifndef __INS_COMMON_H__
#define __INS_COMMON_H__

#define GPS_SIZE 7


#include "iwise_loc_base.h"

typedef enum {
    OVER, NONE_THIS_TIME, OK
} GET_DATA_STATUS;

typedef struct {
    pthread_t imu_t;
    pthread_mutex_t imu_mutex;
    pthread_cond_t imu_cond;
    double ins_data[7];

    pthread_mutex_t gps_mutex;
    pthread_cond_t gps_cond;
    int gps_flag;
    double gps_data[13];

} INS_LOGIC;

typedef struct {
    pthread_t result_t;
    pthread_mutex_t result_mutex;
    pthread_cond_t result_cond;
    int result_flag;
    double sol[19];
    double fil[13];

} INS_RESULT;

typedef struct {

    char *f_imu;

    double d_rate;
    double t_start;
    double t_end;

    /* Alignment */
    char opt_ins_align;
    double ini_pos[3];
    double ini_pos_var;
    double ini_vel[3];
    double ini_vel_va;
    double ini_att[3];
    double ini_att_var[3];


    char *f_gps;
    int f_gps_col;
    double gps_pos_std_scale;
    double gps_vel_std_scale;

    /* vehicle frame aiding options */
    double var_zupt;
    double *t_zupts;
    int opt_zuptA;
    double var_zuptA;

    /* non holonmic constrains */
    char opt_holo;
    double var_holo[2];
    double C_bn[9];
    double la_holo[3];

    /* IMU performance */
    double vrw;
    double arw;
    double bg[3], ba[3], sg[3], sa[3];
    double bg_var;
    double ba_var;
    double bg_Tcor;
    double ba_Tcor;

    /* necessary parameter for 1st order */
    double bg_modle[3];
    double ba_modle[3];
    double Q[15 * 15];

    /* Output  */
    char opt_feedback_bias;
    double T_output;
    char *f_sol;
    char *f_fil;
} CFG;

extern int get_gps_data_cb(double *data);
void set_get_ins_method(GET_DATA_STATUS(*cb) (double *data));
void set_get_gps_method(GET_DATA_STATUS(*cb) (double *data, int legth));
void set_get_result_method(void (*cb) (double *sol, double *fil));
int  ins_gps_callback(iwise_sol_t* sol_gps );
static void send_location_2_server();
int config_init(CFG * CFG);
//extern void config_destory(CFG * CFG);
#ifdef __cplusplus

#endif
#endif        /* INS_COMMON_H */
