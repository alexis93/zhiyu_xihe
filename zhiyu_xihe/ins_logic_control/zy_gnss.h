/*
 * zy_gnss.h
 *
 *  Created on: Jul 9, 2015
 *      Author: root
 */

#ifndef ZY_GNSS_H_
#define ZY_GNSS_H_

#define MAX_NMEA_SENDBUF 200

#define PACKET_SYNC 0x5A
#define GPSLOCATION_TYPE 0x00
#pragma pack(1)

//知域服务数据包包头,总长2字节
typedef struct zy_packet_head_s{
	int8_t sync;		//统一为0x5A,即 'Z'
	int8_t type;    	//GPS服务为0x0X (X:[0~9])
						//车间通讯服务为0x1X
}zy_packet_head_t;

//知域GpsLocation数据结构体，总长50字节
typedef struct zy_gpslocation_data_s{
	int32_t size;   	//GpsLocation结构体大小
	int16_t flags;      //GpsLocationFlags
	double latitude;    //纬度
    double longitude;   //经度
    double altitude;    //高度
    float speed;        //速度
    float bearing;      //方向
    float accuracy;     //精确度
    int64_t timestamp;  //时间戳
}zy_gpslocation_data_t;
//知域GpsLocation数据包结构体
typedef struct zy_gpslocation_packet_s{
	zy_packet_head_t head;
	zy_gpslocation_data_t data;
}zy_gpslocation_packet_t;

extern int gnss_init(void);
extern int gnss_destory(void);
#endif /* ZY_GNSS_H_ */
