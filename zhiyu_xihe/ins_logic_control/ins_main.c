/*********************************************************************************
 * Copyright(C), 2014-2015 zhiyu gnss. // 版权声明
 * File name:  ins_main.c// 文件名
 * Author:   gnss  // 作者
 * Version:   2.0 // 版本
 * Date:  2015.04.17 // 完成日期
 * Description:    控制层主函数
 * Function List:
 * get_ins_data()  //获取ins数据
 * get_gps_data()  //获取gps数据
 * get_result_data() // 获取结果
 * gps_callback()  //gps回调函数
 * init_gps()   //gps数据初始化
 * init_ins()   //系统初始化
 * init_imu()  //惯导数据初始化
 * imu_data_thread() //获取惯导数据线程
 * main()  //主函数
 * History:
 * 2015.4.15 gnss add function  ins_cb()
 * 2015.3.13  gnss add the function get_result_data()
 * 2015.3.13  gnss add the function cb()
 * 2015.3.11  gnss add the function get_gps_data_cb()
 *********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "ins_common.h"
#include "zy_gnss.h"
#include "data_parse.h"

#include "zy_tcp_worker.h"
#include "iwise_net_log.h"

/* the path need to change if necessary */
//#include "../gps/iwise_position.h"

#include "iwise_loc_base.h"
//#include "iwise_gps.h"
#include "../imu/imu_sensor.h"
#define pi 3.141592653


static const GpsInterface* mGpsInterface = NULL;
static const PGpsInterface* mPGpsInterface = NULL;

INS_LOGIC mLogic;
INS_RESULT mResult;

CFG cfg;
#define LOG_BASE_PATH "/data/log/"
//#define LOG_BASE_PATH "/storage/sd_internal/log/"
#define LOG_BUF_SIZE  1024
#define MAXFILENAME   1300


static int  gps_send=-1;
static int  sol_fd = -1;
static int  fil_fd = -1;
static int  gps_result=-1;
static int  compare_result=-1;
static int  imu_result=-1;


int open_fd (char *fileName)
{
    int fd;
    char fileSt[MAXFILENAME]= LOG_BASE_PATH;
    char * fileStr = fileSt;
    char tbuf[LOG_BUF_SIZE] = "";
	struct tm * now = 0;
	time_t t;
	time(&t);
    t+=8*60*60;
	now = localtime(&t);
	int c = strftime(tbuf, LOG_BUF_SIZE, "%F-%H-%M-%S", now);
	sprintf(fileStr, "%s%s/%s_%s%s.txt",
			LOG_BASE_PATH, "imu","t", fileName, tbuf);
			
    fd = open(fileStr, O_WRONLY|O_CREAT|O_NONBLOCK,06666);
    if(fd < 0)
    {
       
        return -1;
    }
   

    return fd;
}


/****************************************
 * Function:     imu_data_thread
 * Description:    //获取惯导数据线程
 * Calls:      // 被本函数调用的函数清单
 * ins_read_data()  //调用驱动中获取数据函数
 * Input: arg
 * Output: ins_data
 * Return: none
 * * Others:        // 其他说明
 ********************************************/

void *imu_data_thread(void *arg)
{
    double ins_data[7];
    double rel_data[7] = { 0 };
    memset(rel_data, 0, sizeof(double) * 7);
    char r[256];
    int l = 0;
    double m_time;
    double GT;
    unsigned int week;
    int i = 0;

    double ax = 0, ay = 0, az = 0;
    double axa = 0, aya = 0, aza = 0;

    double accel[110] = { 0 };
    double alcount = 0;
    double alavg = 0;
    int jj = 0;

/* here we keep  100 seconds  imu data ,and get the average of them and the average bias .
 * after we use the raw  ins data to subtract the average bias, to get the useful raw  ins data */
    for (; jj < 100; jj++)
    {
        if (ins_read_data(ins_data) != 7)
        {

        }
        accel[jj] = ins_data[3] / 16384 * 9.8 * 0.05;

        alcount = alcount + accel[jj];

    }
    alavg = alcount / 100 - 0.49;
    while (1)
    {
        if (ins_read_data(ins_data) != 7)
        {
            continue;

        }
        i += i < 70 ? 1 : 0;

        if (i < 20)
        {
            continue;
        }
        if (i < 70)
        {
            ax = (ax * (i - 20) + ins_data[5]) / (i - 19);
            ay = (ay * (i - 20) + ins_data[4]) / (i - 19);
            az = (az * (i - 20) + ins_data[6]) / (i - 19);

            axa = (axa * (i - 20) + ins_data[2]) / (i - 19);
            aya = (aya * (i - 20) + ins_data[1]) / (i - 19);
            aza = (aza * (i - 20) + ins_data[3]) / (i - 19);
            continue;
        }

        rel_data[0] = ins_data[0];      /* time */
        rel_data[1] = (ins_data[5] - ax) / 131 * pi / 180 * 0.05;
        rel_data[2] = (ins_data[4] - ay) / 131 * pi / 180 * 0.05;
        rel_data[3] = -(ins_data[6] - az) / 131 * pi / 180 * 0.05;      /* gyro */
        rel_data[4] = (ins_data[2]) / 16384 * 9.8 * 0.05;
        rel_data[5] = (ins_data[1]) / 16384 * 9.8 * 0.05;
        rel_data[6] = -((ins_data[3]) / 16384 * 9.8 * 0.05 - alavg);    /* accel */
        //printf(" imu data %lf %lf %lf %lf %lf %lf %lf\n",ins_data[0],rel_data[1],rel_data[2],rel_data[3],rel_data[4],rel_data[5],rel_data[6]);
        l = sprintf(r,"%lf %lf %lf %lf %lf %lf %lf\n",ins_data[0],
													rel_data[1],
													rel_data[2],
													rel_data[3],
													rel_data[4],
													rel_data[5],
													rel_data[6]
													);
		write(imu_result, r, l);
        
        
        pthread_mutex_lock(&mLogic.imu_mutex);
        memcpy(mLogic.ins_data, rel_data, sizeof(double) * 7);
        pthread_mutex_unlock(&mLogic.imu_mutex);
        pthread_cond_signal(&mLogic.imu_cond);
    }

}

/****************************************
 * Function:     ins_gps_callback
 * Description:    GPS 回调函数调用GPS数据
 * Calls:      // 被本函数调用的函数清单
 * get_gps_data_cb()
 * Input: event_mask
 *        event_payload_u_type * payload
 * Output: 
 * Return: 1
 * * Others:        // 其他说明
 *****************************************/

int  ins_gps_callback(iwise_sol_t* sol_gps )
{
  
        double data[7] = { 0 };
        struct timeval now;
        gettimeofday(&now, NULL);
        data[0] = now.tv_sec + (double) now.tv_usec / 1000000;
        //printf("sol.rr 0 %lf, rr1 %lf rr 2 %lf \n",sol_gps->rr[0],sol_gps->rr[1],sol_gps->rr[2]);
        memcpy(data + 1, sol_gps->rr, sizeof(double) * 6);
        //memcpy(data + 1, &(sol_gps->rr[0]), sizeof(double) * 6);
        printf("gps data 0 %lf, data 1 %lf data 2 %lf  %lf %lf %lf %lf \n",data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
        if (*(data + 1) < 1)
            return 0;
        get_gps_data_cb(data);
        gps_send=1;

    
    return 1;
}

/****************************************
 * Function:     get_gps_data_cb
 * Description:    GPS数据
 * Calls:      // 被本函数调用的函数清单
 * Input: double *data  gps数据
 * Output: 
 * Return: 7
 * * Others:        // 其他说明
 *****************************************/
extern int get_gps_data_cb(double *data)
{
    pthread_mutex_lock(&mLogic.gps_mutex);
    memcpy(mLogic.gps_data, data, sizeof(double) * 7);
    mLogic.gps_flag = 1;
    pthread_mutex_unlock(&mLogic.gps_mutex);
    return 7;
}

/****************************************
 * Function:     get_ins_data
 * Description:   
 * Calls:      // 被本函数调用的函数清单
 * Input: double *data ins数据
 * Output: 
 * Return: ok
 * * Others:        // 其他说明
 *****************************************/
static GET_DATA_STATUS get_ins_data(double *data)
{
    pthread_mutex_lock(&mLogic.imu_mutex);
    pthread_cond_wait(&mLogic.imu_cond, &mLogic.imu_mutex);
    memcpy(data, mLogic.ins_data, sizeof(double) * 7);
    pthread_mutex_unlock(&mLogic.imu_mutex);

    return OK;
}

/****************************************
 * Function:     get_result_data
 * Description:    
 * Calls:      // 被本函数调用的函数清单
 * Input: double *sol  sol数据结构体
 *        double *fil  fil数据结构体
 * Output: 
 * Return: 
 * * Others:        // 其他说明
 *****************************************/
static void get_result_data(double *sol, double *fil)
{  


    int lock_status;

    lock_status = pthread_mutex_trylock(&mResult.result_mutex);
    if (lock_status == 0)
    {
        
            memcpy(mResult.sol, sol, sizeof(double) * 19);
            memcpy(mResult.fil, fil, sizeof(double) * 13);
            
            do_send_ins(&(mResult.sol),&(mResult.fil));  // send ins  result data  to tcpserver
            do_send_ins_NMEA(&mResult);     //send ins_NMEA data to tcpserver
            
            int l;
	        char buf[512];
	        printf(" result %lf %10.9f %10.9f %10.9f \n",mResult.sol[0],						
													mResult.sol[1]*180/pi,	mResult.sol[2]*180/pi,	mResult.sol[3]);
            l = sprintf(buf,"%lf %10.9f %10.9f %10.9f %10.9f %10.9f %10.9f %10.9f %10.9f %10.8f %10.8f %10.8f %10.8f %10.8f %10.8f %10.8f  \n",
													mResult.sol[0],	  /* time*/												
													mResult.sol[1]*180/pi,	mResult.sol[2]*180/pi,	mResult.sol[3], /* position */	
													mResult.sol[4],	mResult.sol[5],	mResult.sol[6], /* velocity */	
													mResult.sol[7],	mResult.sol[8],	mResult.sol[9], /* attitude */
													mResult.fil[1],	 mResult.fil[2],	 mResult.fil[3], /* gyro bias */
													mResult.fil[4],	 mResult.fil[5],	 mResult.fil[6] /* accel bias */);
	        write(compare_result,buf,l);//write to file for format
	        send_location_2_server();

        
        pthread_mutex_unlock(&mResult.result_mutex);
    }

}

/****************************************
 * Function:     get_gps_data
 * Description:    GPS 回调函数调用GPS数据
 * Calls:      // 被本函数调用的函数清单
 * Input: double *data  gps数据
 *        int length     数据长度
 * Output: 
 * Return: result
 * * Others:        // 其他说明
 *****************************************/
static GET_DATA_STATUS get_gps_data(double *data, int length)
{

    int lock_status;
    GET_DATA_STATUS result = NONE_THIS_TIME;
    lock_status = pthread_mutex_trylock(&mLogic.gps_mutex);
    if (lock_status == 0)
    {
        if (mLogic.gps_flag == 1)
        {
            memcpy(data, mLogic.gps_data, sizeof(double) * 7);
            mLogic.gps_flag = 0;
            result = OK;
            int l;
	        char buf[256];
	        l = sprintf(buf,"%lf %10.9f %10.9f %10.9f %10.9f %10.9f %10.9f\n",mLogic.gps_data[0],mLogic.gps_data[1],mLogic.gps_data[2],mLogic.gps_data[3],mLogic.gps_data[4],mLogic.gps_data[5],mLogic.gps_data[6]);
	        write(gps_result,buf,l);//write to file for format
        }
        pthread_mutex_unlock(&mLogic.gps_mutex);
    }

    return result;
}



/****************************************
 * Function:     init_gps
 * Description:    
 * Calls:      // 被本函数调用的函数清单
 * get_interface()
 * Input:
 * Output: 
 * Return: 1
 * * Others:        // 其他说明
 *****************************************/
static int init_gps()
{
    int ret= -1;
    ret = gnss_init();   //init the iwise_gps_loc_hal 
    if(ret == 0){
	printf("gnss init success\n");
    return 1;
    }
    else{
    return 0;}
    
}


/****************************************
 * Function:     init_imu
 * Description:    获取imu数据
 * Calls:      // 被本函数调用的函数清单
 * sensor_init()  Imu驱动调用数据函数
 * Input:
 * Output: 
 * Return: sensor_init()
 * * Others:        // 其他说明
 *****************************************/
static int init_imu()
{
    return sensor_init();
}

/****************************************
 * Function:    init_ins
 * Description:    系统配置初始
 * Calls:      // 被本函数调用的函数清单
 * config_init()
 * Input: 
 * Output: 
 * Return: 1
 * * Others:        // 其他说明
 *****************************************/
static int init_ins()
{
    memset(&cfg, 0, sizeof(CFG));
    config_init(&cfg);
    return 1;
}

/****************************************
 * Function:     cb
 * Description:    for test
 * Calls:      // 被本函数调用的函数清单
 
 * Input: double *sol
 *        double *fil
 * Output: 
 * Return: 
 * * Others:        // 其他说明
 *****************************************/
void cb(double *sol, double *fil)
{
    printf("for test\n");
}

/****************************************
 * Function:     ins_cb
 * Description:    函数调用
 * Calls:      // 被本函数调用的函数清单
 * get_ins_data（）
 * Input: double *data
 * Output: 
 * Return: get_ins_data(data)
 * * Others:        // 其他说明
 *****************************************/
GET_DATA_STATUS ins_cb(double *data)
{
    return get_ins_data(data);
}

/****************************************
 * Function:     gps_cb
 * Description:    函数调用
 * Calls:      // 被本函数调用的函数清单
 * get_gps_data()
 * Input: double *data
 *       int length
 * Output: 
 * Return: get_gps_data(data, length)
 * * Others:        // 其他说明
 *****************************************/
GET_DATA_STATUS gps_cb(double *data, int length)
{
    return get_gps_data(data, length);
}


struct message_s{
	int message_type;
	int message_id;
	double lat;
	double lon;
	double alt;
	int type; // 标志是哪一路数据
	int other;
};

typedef struct message_s message_t;
typedef struct{
	int message_type;
	int message_id;
	double lat;
	double lon;
	double alt;
	
	double speed;
	double fangxiang;
	int64_t time;
	int type;
	int other;
}message808_t;
message808_t message808={
	0x5A5A,
	0x04,
	0,
	0,
	0,
	
	0,
	0,
	0,
	1,
	2
};
message_t send_message ={
	0x5A5A,
	0x03,
	0,
	0,
	0,
	7,
	2
};
message_t send_message1 ={
	0x5A5A,
	0x03,
	0,
	0,
	0,
	8,
	2
};
static pthread_t location_point_pthread_t;
static pthread_mutex_t location_point_mutex;
static pthread_cond_t  location_point_cond;
//extern int location_socket;

static void send_location_2_server(){
		static uint64_t send_limit=0;//horiable implementent
		pthread_mutex_lock(&location_point_mutex);
		/*if(send_limit++<120){//call 120s
			send_message.lon = gps[2]*180/pi;
			send_message.lat = gps[1]*180/pi;
			send_message.alt = gps[3];
		}else{
			send_message.lon  = nav.r[1]*180/pi;
			send_message.lat  = nav.r[0]*180/pi;
			send_message.alt  = nav.r[2];
		}*/
		send_message1.lon = mLogic.gps_data[2]*180/pi;
		send_message1.lat = mLogic.gps_data[1]*180/pi;
		send_message1.alt = mLogic.gps_data[3];
		send_message.lon  = mResult.sol[2]*180/pi;
	    send_message.lat  = mResult.sol[1]*180/pi;
		send_message.alt  = mResult.sol[3];
		
/*		message808.lon    = gps[2]*180/pi;
		message808.lat    = gps[1]*180/pi;
		if(nav.r[2]<0)
			message808.alt    =0;
		else
			message808.alt = gps[3];*/
		message808.lon    =send_message.lon;
		message808.lat    = send_message.lat;
		if(mLogic.gps_data[2]<0)
			message808.alt    =0;
		else
			message808.alt = send_message.alt;
			
		message808.speed  = sqrt(pow2(mResult.sol[4])+pow2(mResult.sol[5])+pow2(mResult.sol[6]));
		message808.fangxiang =0;
		
		message808.time   =((int64_t)mLogic.gps_data[0])*1000;
		pthread_cond_signal(&location_point_cond);
		pthread_mutex_unlock(&location_point_mutex);
		
}

int location_socket=-1; 
int message808_socket=-1;//add

static int connect_server(int port, char *addr){
	struct sockaddr_in server_addr;
	int msocket = socket(AF_INET, SOCK_STREAM, 0);
	if(msocket<0){
			printf("get socket error\n");
			return -1;
	}
	
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(addr);
	server_addr.sin_port = htons(port);
	int try_times=0;
	printf("try to connect\n");
	while(connect(msocket,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
			perror("connect error:");
			
			if(++try_times>6)
				return -1;
			sleep(1);
	}
	
	printf("connect success\n");
	return msocket;//return socket
}
static void send_location_boot(){
//we just send it anywhere
		send_message1.lon = mLogic.gps_data[2]*180/pi;
		send_message1.lat = mLogic.gps_data[1]*180/pi;
		send_message1.alt = mLogic.gps_data[3];
		
		
		message808.lon    = mLogic.gps_data[2]*180/pi;
		message808.lat    = mLogic.gps_data[1]*180/pi;
		send_message1.alt = mLogic.gps_data[3];
		message808.time   =((int64_t)mLogic.gps_data[0])*1000;//we do not wait the network
		
		write(location_socket,&send_message1,sizeof(message_t));
		//write(location_socket,&send_message,sizeof(message_t));
		static int send_count=0;
		if((++send_count)>5){
			send_count=0;
			//write(message808_socket,&message808,sizeof(message808_t));
		}
	
}
static void* send_location_context(void* arg){
	while((location_socket=connect_server(9001,"119.97.194.188"))<0);
	while((message808_socket=connect_server(7202,"121.41.90.92"))<0);//we wait 
	static int message808_count=0;
	int lenth;
	message_t nav_message,gps_message;
	message808_t msg_808;
	while(1){
		pthread_mutex_lock(&location_point_mutex);
		pthread_cond_wait(&location_point_cond,&location_point_mutex);
		
		memcpy(&nav_message,&send_message,sizeof(message_t));
		memcpy(&gps_message,&send_message1,sizeof(message_t));
		memcpy(&msg_808,&message808,sizeof(message808_t));
		
		pthread_mutex_unlock(&location_point_mutex);
		
		lenth=write(location_socket,&nav_message,sizeof(message_t));
		
		
		if(lenth<0){
			printf("send nav.r failed\n");
			close(location_socket);
			location_socket=connect_server(9001,"119.97.194.188");
		}else
			printf("send nav.r success\n");
		if((++message808_count)>5){//5s everytime
			message808_count=0;
			lenth=write(message808_socket,&msg_808,sizeof(message808_t));
			//printf("send 808\n");
			if(lenth<0){
				close(message808_socket);
				message808_socket=connect_server(7202,"121.41.90.92");
			}
		}
		
		if(gps_send==1){
			gps_send=-1;
			lenth=write(location_socket,&gps_message,sizeof(message_t));
			printf("send gps\n");
			if(lenth<0){
				close(location_socket);
				location_socket=connect_server(9001,"119.97.194.188");
			}
		}
//		printf("%10.10f %10.10f %10.5f\n",nav_message.lon,nav_message.lat,nav_message.alt);
	}
	close(location_socket);
	close(message808_socket);
	
	return NULL;	
}



/*****************************************
 * Function:     main
 * Description:   主函数
 * Calls:      // 被本函数调用的函数清单
 * init_gps() //gps数据初始化
 * init_imu（）//惯导数据初始化
 * init_ins（）//系统初始化
 * set_get_result_method（）获取结果数据
 * set_get_ins_method（） 获取ins数据
 * set_get_gps_method（）获取gps数据
 * ins_ekf（）导航函数
 * Input: 
 * Output: 
 * Return: 1
 * * Others:        // 其他说明
 *****************************************/
int main()
{  

	//sleep(10);
	 /* open files. */
	compare_result=open_fd("compare");
	if (compare_result<=0) {
	    printf("open compare failed:%s \n",strerror(errno));
	} else 
	{
	   printf("open compare success:%d \n",compare_result);
	}

	gps_result=open_fd("gps_result");
	if (gps_result<=0) {
	    printf("open compare failed:%s \n",strerror(errno));
	}

	imu_result=open_fd("imu_result");
	if (imu_result<=0) {
	    printf("open compare failed:%s \n",strerror(errno));
	}
	
	//init the tcpclient
	 if (tcp_client_init() != 0)
    {
         printf("tcp client init failed\n");
        return 1;
    }
    else
        printf("tcp client init success\n");
	
	
    /*  data init  */
    if (init_gps() != 1)
    {

        return 1;
    }
    else
        printf("init gps success\n");



    if (init_imu() != 1)
    {

        return 1;
    } else
        printf("init imu success\n");

    if (init_ins() != 1)
    {

        return 1;
    } else
        printf("init logic success\n");
    /* create manager  */
    pthread_mutex_init(&mLogic.imu_mutex, NULL);
    pthread_cond_init(&mLogic.imu_cond, NULL);
    pthread_mutex_init(&mLogic.gps_mutex, NULL);
    pthread_create(&mLogic.imu_t, NULL, imu_data_thread, NULL);
    
    //pthread_mutex_init(&location_point_mutex,NULL);
	//pthread_cond_init(&location_point_cond,NULL);
	//pthread_create(&location_point_pthread_t,NULL,send_location_context,NULL);
	

    /* register the cb, ins_cb,gps_cb  to libins.so */
    set_get_result_method(get_result_data);
    set_get_ins_method(ins_cb);
    set_get_gps_method(gps_cb);
     
    /*Navigation start  */
    ins_ekf(&cfg);
  
    
    close(gps_result);
    close(imu_result);
	close(compare_result);
    /* close the gps data  */
    //interface->iwise_stop_fix();
     //gnss_destory();
    

    return 1;

}
