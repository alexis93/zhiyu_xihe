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



int main(){
	
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
	
	inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
			 MPUREG_ACCEL_CONFIG, 0x10);
	
	unsigned char d[4];
	inv_serial_read(g_mlsl_handle, mldl_cfg.addr,
			 0x0D, 4, d);
	printf("[test]%x %x %x %x\n", d[0], d[1], d[2], d[3]);
			 
	status = 0;
	
	inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
			 MPUREG_FIFO_EN, status);
	
	//设置采样率为 10hz = 1000hz/(1+99)
	result = inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
				MPUREG_SMPLRT_DIV, 99);
			 
	// 第一次的时候需要重置，打开fifo
	inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
			 MPUREG_USER_CTRL, BIT_FIFO_EN|BIT_FIFO_RST);
			 
	status = BIT_GYRO_XOUT | BIT_GYRO_YOUT | BIT_GYRO_ZOUT | BIT_ACCEL;
	inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
			 MPUREG_FIFO_EN, status);
	//open int		 
	inv_serial_single_write(g_mlsl_handle, mldl_cfg.addr,
			 MPUREG_INT_ENABLE, 0x01);
	
		
	unsigned short length;
	unsigned char l[2];	 
	int nevents=-1;
	int nread=-1;
	while(1){
		
		nevents = poll(mPollFds,1,1000);               
		printf("nevents [%d]\n", nevents);
		if(nevents<=0){
			continue;
		}
		if(mPollFds[0].revents == POLL_IN){
			nread = read(mPollFds[0].fd, &irqdata, sizeof(irqdata));
			mPollFds[0].revents = 0;
		}
		//read the count 
		inv_serial_read(g_mlsl_handle, mldl_cfg.addr,
                             MPUREG_FIFO_COUNTH, 2, l);
        length = (l[0]<<8)+l[1];
        printf("Before length [%d]\n", length);
        if(length < 60){
			continue;
		}
		
		unsigned char data[60];
		inv_serial_read_fifo(g_mlsl_handle, mldl_cfg.addr, 60, data); 
		int i = 0;
		for(; i<60/12; i++){
			short x, y, z;
			short x1, y1, z1; 
			x = (short)(((data[0+12*i] << 8) | data[1+12*i]));/*CAL_DIV;*/
			y = (short)(((data[2+12*i] << 8) | data[3+12*i]));/*CAL_DIV;*/
			z = (short)(((data[4+12*i] << 8) | data[5+12*i]));/*CAL_DIV;*/
			
			x1 = (short)(((data[6+12*i] << 8) | data[7+12*i]));/*CAL_DIV;*/
			y1 = (short)(((data[8+12*i] << 8) | data[9+12*i]));/*CAL_DIV;*/
			z1 = (short)(((data[10+12*i] << 8) | data[11+12*i]));/*CAL_DIV;*/  
			printf("mpu6050_read_all[Acc] x: %d y: %d Z: %d\n", x, y, z);
			printf("mpu6050_read_all[Gyro] x: %d y: %d Z: %d\n", x1, y1, z1); 
			
		}
		inv_serial_read(g_mlsl_handle, mldl_cfg.addr,
                             MPUREG_FIFO_COUNTH, 2, l);
        length = (l[0]<<8)+l[1];
        printf("After length [%d]\n", length);             
	}
	
	return 0;
}
