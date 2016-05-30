/***********************************************************************************
 * Copyright(C), 2014-2015 zhiyu gnss. // 版权声明
 * File name:  config.c// 文件名
 * Author:   gnss  // 作者
 * Version:   2.0 // 版本
 * Date:  2014.12.20 // 完成日期
 * Description:     算法初始配置函数
 * Function List:   // 主要函数的列表，每条记录应包括函数名及功能简要说明
 * config_init（） //初始配置函数
 * History:
 * 2015.4.15 gnss update the config parameter ba,bg,sa,sg,CFG->gps_pos_std_scale,CFG->bg_Tcor,CFG->ba_Tcor,CFG->ba_var,CFG->bg_var.
 * 修改历史，包括每次修改的日期、修改者和修改内容简述
 ***********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>

#include "../imulib/function.h"
#include "ins_common.h"




static const double ini_pos[3] = { 30.5230 * pi / 180, 114.3620 * pi / 180,
    80.05
};                              /* Given initial position [rad; rad; m] */


static const double ini_vel[3] = { 0, 0, 0 };   /* Given initial velocity */

static const double ini_att[3] = { 0 * pi / 180, 0 * pi / 180, 68.1 * pi / 180 };  /* Given initial attitude [rad] */

static const double ini_att_var[3] = { 2.0 * pi / 180, 2.0 * pi / 180, 5.0 * pi / 180
};                              /* variance of the initial attitude [rad^2] */
static const double var_holo[2] = { 0.1, 0.1 };
static const double la_holo[3] = { 0.0, 0.0, 0.0 };


static const double sa[3] = { 0, 0, 0 };        /* known accel SF */
static const double sg[3] = { 0, 0, 0 };        /* known gyro SF error [] */
static const double ba[3] = { 0.2, 0, -0.25 };  /* known accelerometer biases [m/s^2]  */
static const double bg[3] = { -0.0017, -0.0157, 0 };    /* known gyro biases [rad/s] */


static void var_init(double *p, const double *src, int row)
{
    int i = 0;
    for (; i < row; i++)
        *p++ = pow2(src[i]);
}

static void matrix_init(double *p, const double *src)
{
    int i = 0;
    for (; i < 3; i++)
        *p++ = *src++;
}

static void model_init(const double beta, double *model)
{
    int i = 0;
    for (; i < 3; i++)
        *model++ = beta * 1.0;
}


static void init_pos(CFG * cfg)
{
    matrix_init(cfg->ini_pos, ini_pos);
}

extern int config_init(CFG * CFG)
{

    CFG->d_rate = 20;


    CFG->opt_ins_align = 1;

    init_pos(CFG);
    CFG->ini_pos_var = pow2(10);
    matrix_init(CFG->ini_vel, ini_vel);
    CFG->ini_vel_va = pow2(0.1);
    matrix_init(CFG->ini_att, ini_att);
    var_init(CFG->ini_att_var, ini_att_var, 3);


    CFG->f_gps_col = 7;

    CFG->gps_pos_std_scale = 0.8;/* scale to adjust the GPS position STD */
    CFG->gps_vel_std_scale = 1; /* scale to adjust the GPS velocity STD  */

    /* Vehicleframe aiding options */
    CFG->var_zupt = pow2(0.01); /* variance of ZUPT [(m/s)^2] */
    CFG->opt_zuptA = 1;
    CFG->var_zuptA = pow2((0.1 * pi / 180));

    /* non-holonomic constraints */
    CFG->opt_holo = 1;
    var_init(CFG->var_holo, var_holo, 2);

    euler2dcm(0 * pi / 180, 0 * pi / 180, 0 * pi / 180, CFG->C_bn);

    matrix_init(CFG->la_holo, la_holo);

    /* IMU Performance */
    CFG->vrw = 0.03;            /* Velocity random walk coefficient, i.e. white noise of accel [m/s^2/sqrt(Hz)] */
    CFG->arw = pi / 180 / 60;   /* Angular random walk coefficient, i.e. white noise of gyro [rad/s/sqrt(Hz)] */
    matrix_init(CFG->bg, bg);
    matrix_init(CFG->ba, ba);
    matrix_init(CFG->sg, sg);
    matrix_init(CFG->sa, sa);   /* lookup biases of sensors. */

    CFG->bg_var = pow2((210 * pi / 180 / 3600)); /* variance of gyro biases [(rad/s)^2] */
    CFG->ba_var = pow2((0.02)); /* variance of accel biases [(m/s^2)^2] */

    CFG->bg_Tcor = 1 * 3600; /* gyro bias correlation time [sec] */
    CFG->ba_Tcor = 1 * 3600; /* acc bias correlation time [sec]  */

    /* calculate necessary parameters for 1st order GM process, based on given parameters */
    model_init(exp((-1.0) / CFG->bg_Tcor / CFG->d_rate), CFG->bg_modle);
    model_init(exp((-1.0) / CFG->ba_Tcor / CFG->d_rate), CFG->ba_modle);

    double q_bg = 2 * CFG->bg_var / CFG->bg_Tcor;
    double q_ba = 2 * CFG->ba_var / CFG->ba_Tcor;
    double vector[15] = { 0, 0, 0,
        pow2(CFG->vrw), pow2(CFG->vrw), pow2(CFG->vrw), /* velocity random walk */
        pow2(CFG->arw), pow2(CFG->arw), pow2(CFG->arw), /* angular random walk */
        q_bg, q_bg, q_bg,       /* gyro bias variation  */
        q_ba, q_ba, q_ba        /* accelerometer bias variation */
    };
    mat_diag(vector, 15, CFG->Q);


    CFG->opt_feedback_bias = 1;
    CFG->T_output = 0.999999;   /* Output interval of the navigation solution [sec] */

    return 1;
}

extern void config_destory(CFG * CFG)
{
    free(CFG->t_zupts);
}
