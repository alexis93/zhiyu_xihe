#include <stdio.h> 
#include <cutils/log.h> 
#include <unistd.h> 
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include <poll.h>

#include "mldl_cfg.h"


#define ALL_MPL_SENSORS_NP (INV_THREE_AXIS_ACCEL | INV_THREE_AXIS_GYRO)

void * g_mlsl_handle;
int mpu_int_fd = -1;
struct pollfd mPollFds[1];
struct mldl_cfg mldl_cfg;
struct ext_slave_descr gAccel;
struct ext_slave_descr gCompass;
struct ext_slave_descr gPressure;
struct mpu_platform_data gPdata;
	
	
struct mpuirq_data irqdata;
	
int status;
int result;
	
unsigned short length;
unsigned char l[2];	 
int nevents=-1;
int nread=-1;

int sensor_init(){
	mpu_int_fd = open("/dev/mpuirq", O_RDWR);
	
	mPollFds[0].fd=mpu_int_fd;
	mPollFds[0].events = POLL_IN;
	
	if (mpu_int_fd == -1) {
        ALOGE("could not open the mpu irq device node %s", strerror(errno));
    } else {
        fcntl(mpu_int_fd, F_SETFL, O_NONBLOCK);
        //ioctl(mpu_int_fd, MPUIRQ_SET_TIMEOUT, 0);
        mPollFds[0].fd = mpu_int_fd;
        mPollFds[0].events = POLLIN;
    }
    
    inv_serial_open(NULL, &g_mlsl_handle);

	mldl_cfg.addr  = 0x68; /* default incase the driver doesn't set it */
    mldl_cfg.accel = &gAccel;
    mldl_cfg.compass = &gCompass;
    mldl_cfg.pressure = &gPressure;
    mldl_cfg.pdata = &gPdata;
	inv_mpu_open(&mldl_cfg, g_mlsl_handle, NULL, NULL, NULL);
	inv_mpu_resume(&mldl_cfg, g_mlsl_handle, NULL, NULL, NULL, ALL_MPL_SENSORS_NP);
	
	
/*	inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
			 MPUREG_ACCEL_CONFIG, 0x10);
			 	unsigned char d[4];
	inv_serial_read(g_mlsl_handle, mldl_cfg.addr,
			 0x0D, 4, d);*/
//	printf("[test]%x %x %x %x\n", d[0], d[1], d[2], d[3]);
			 
	status = 0;
	
	inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
			 MPUREG_FIFO_EN, status);
	
	//设置采样率为 100hz = 1000hz/(1+9)
	result = inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
				MPUREG_SMPLRT_DIV, 9);
	//add by li to config digtal filter	
	result=0;
	inv_serial_read(g_mlsl_handle, mldl_cfg.addr,MPUREG_CONFIG,1,&result);
//	printf("-----------------%x---------------\n",result);
	inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
				MPUREG_CONFIG,0x03);//44HZ 42HZ
//	inv_serial_read(g_mlsl_handle, mldl_cfg.addr,MPUREG_CONFIG,1,&result);
//	printf("----------------new -%x---------------\n",result);
	//config the gyro 0x00
	inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
				MPUREG_GYRO_CONFIG, 0x00);		 
	// 第一次的时候需要重置，打开fifo
	inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
			 MPUREG_USER_CTRL, BIT_FIFO_EN|BIT_FIFO_RST);
			 
	status = BIT_GYRO_XOUT | BIT_GYRO_YOUT | BIT_GYRO_ZOUT | BIT_ACCEL;
	inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
			 MPUREG_FIFO_EN, status);
	//open int		 
	inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
			 MPUREG_INT_ENABLE, 0x01);
	
	return 1;
	
}
int totalax=0;
int totalay=0;
int totalaz=0;

int totalgx=0;
int totalgy=0;
int totalgz=0;

static short value_array[6][5];
static void insert_sort(short a[],int n){
	int i;
	for(i=1;i<n;i++){
		if(a[i]<a[i-1]){
			int j=i-1;
			short x=a[i];
			while(j>=0&&x<a[j]){
				a[j+1]=a[j];
				j--;
			}
			a[j+1]=x;
		}
	}
}
static void value_sort(){
		int i=0;
		for(;i<6;i++)//sort
			insert_sort(value_array[i],5);
}
int ins_read_data(double* getdata){
	while(1){
		nevents = poll(mPollFds,1,1000);               
		if(nevents<=0){
			continue;
		}
		if(mPollFds[0].revents == POLL_IN){
			nread = read(mPollFds[0].fd, &irqdata, sizeof(irqdata));
			mPollFds[0].revents = 0;
		}
		inv_serial_read(g_mlsl_handle, mldl_cfg.addr,
                             MPUREG_FIFO_COUNTH,2 , l);
        length = (l[0]<<8)+l[1];
        if(length < 60){
			continue;
		}
		
		struct timeval now;
		gettimeofday(&now,NULL);
		
		unsigned char data[60];
		inv_serial_read_fifo(g_mlsl_handle, mldl_cfg.addr, 60, data); 
		int i = 0;
		for(; i<60/12; i++){
//			short x, y, z;
//			short x1, y1, z1; 
			value_array[0][i] = (short)(((data[0+12*i] << 8) | data[1+12*i]));/*CAL_DIV;*/
			value_array[1][i] = (short)(((data[2+12*i] << 8) | data[3+12*i]));/*CAL_DIV;*/
			value_array[2][i] = (short)(((data[4+12*i] << 8) | data[5+12*i]));/*CAL_DIV;*/
/*			totalax+=x;
			totalay+=y;
			totalaz+=z;*/
			value_array[3][i] = (short)(((data[6+12*i] << 8) | data[7+12*i]));/*CAL_DIV;*/
			value_array[4][i] = (short)(((data[8+12*i] << 8) | data[9+12*i]));/*CAL_DIV;*/
			value_array[5][i] = (short)(((data[10+12*i] << 8) | data[11+12*i]));/*CAL_DIV;*/  
/*			totalgx+=x1;
			totalgy+=y1;
			totalgz+=z1;*/
		}
		value_sort();
		getdata[0]=now.tv_sec+(double)now.tv_usec/1000000;
/*		getdata[1]=((double)totalax)/5;
		getdata[2]=((double)totalay)/5;
		getdata[3]=((double)totalaz)/5;
		getdata[4]=((double)totalgx)/5;
		getdata[5]=((double)totalgy)/5;
		getdata[6]=((double)totalgz)/5;*/
		int k=1;
		for(;k<7;k++){
			getdata[k]=value_array[k-1][2];
		}
		//printf("hello imu sensor\n\n\n\n\n\n\n\n\n\n");
 //     	totalax=totalay=totalaz=0;
//		totalgx=totalgy=totalgz=0;
		
		break;
	}
	return 7;
}
