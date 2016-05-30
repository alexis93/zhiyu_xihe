/*
 * transfer.c
 *
 *  Created on: 2013年12月19日
 *      Author: chen
 */
#include "gnss_core.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define LOG_CONF	"log2.conf"
#define CONFIG		"transfer_aliyun.conf"
#define BUFFER_SIZE		4096

static int data_source_sock = -1;
static int gnss_tcp_server_sock = -1;
//static zlog_category_t *log;
static gnss_mempool_t * packet_pool;
static gnss_queue_t * packet_queue;
static gnss_config_t config;
char MAC[]="000000000000";
struct gnss_packet_s {
	size_t count;
	char buffer[BUFFER_SIZE];
};

// 处理配置文件
static int handle_config(void* user, const char* section, const char* name,
		const char* value) {
	gnss_config_t * pconfig = (gnss_config_t *) user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	if (MATCH("nmea_server", "port")) {
		pconfig->nmea_server_port = (uint16_t) atoi(value);
	} else if (MATCH("aliyun_server", "port")) {
		pconfig->aliyun_server_port = (uint16_t) atoi(value);
	} else if (MATCH("nmea_server", "address")) {
		pconfig->nmea_server_address = strdup(value);
	} else if (MATCH("aliyun_server", "address")) {
		pconfig->aliyun_server_address = strdup(value);
	} if (MATCH("nmea_server", "mac")) {
		pconfig->nmea_server_mac = strdup(value);
	} else {
		return 0; /* unknown section/name, error */
	}
	return 1;
}

int find(char buf[],int len,int t)
{
	int i,num=0;
	for(i=0;i<len;i++)
	{
		if(buf[i]==',') num++;
		if(num==t) return i;
	}
	return -1;
}

	double convert_lat_lon(double latlon)
	{
		int deg;
		double min;
		deg=(int)latlon/100;
		min=latlon-(deg*100);
		return (min/60+deg);
	}

void connect_data_server() {
	//zlog_info(log, "Try to connect data source server");
	if (data_source_sock > 0) {
		//zlog_info(log, "close socket connect to data source server");
		close(data_source_sock);
	}
	struct sockaddr_in data_server;
	// 创建一个tcp 的 socket fd 连接数据服务器
	while ((data_source_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		//zlog_warn(log, "Can't create socket fd with data source server");
		sleep(1);
	}
	// 尝试连接数据服务器, 这个是不确定的
	// 如果连接不上,那就一直尝试连接, 否则也没有数据转给 gnss server
	memset(&data_server, 0, sizeof(data_server));
	data_server.sin_family = AF_INET;
	data_server.sin_addr.s_addr = inet_addr(config.nmea_server_address);
	data_server.sin_port = htons(config.nmea_server_port);
	while (connect(data_source_sock, (struct sockaddr *) &data_server,
			sizeof(data_server)) < 0) {
		//zlog_warn(log, "Can't connect to data source server");
		sleep(1);
	}

	// 设置超时时间为10秒
	int status = 0;
	struct timeval tv = { 10, 0 };
	if ((status = setsockopt(data_source_sock, SOL_SOCKET, SO_RCVTIMEO,
			(char *) &tv, sizeof(struct timeval))) == -1) {
		//zlog_error(log, "Error to set timeout to data_source_sock");
	}
	//zlog_info(log, "Connect data source server success");
}

/*
 * 建立连接,读取数据,把数据放到队列里
 */
void * connect_data_server_thread() {
	connect_data_server();

	int count = 0, times = 0;
	gnss_memnode_t * node = gnss_mempool_get_node(packet_pool);
	gnss_packet_t * packet = (gnss_packet_t *) node->data;
	while (1) {
		count = read(data_source_sock, packet->buffer, BUFFER_SIZE);
		if (count <= 0) {
			// 超时处理
			if (count == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
				//zlog_warn(log, "read from data_source_sock timeout");
				times++;
				if (times > 3) {
					//zlog_warn(log,"timeout four times, close socket, reconnect!");
					connect_data_server();
					times = 0;
				}
			} else {
				if (count == -1) {
					//zlog_warn(log, "read data error: %s", strerror(errno));
				}
				// 其它错误,不管是什么错误,重新连接
				connect_data_server();
			}
		} else {
			packet->count = count;
			gnss_queue_push(packet_queue, node);
			//zlog_info(log, "Read %d bytes from data source server", count);
			node = gnss_mempool_get_node(packet_pool);
			packet = (gnss_packet_t *) node->data;
		}
	}
	return (void *) 0;
}

void connect_gnss_tcp_server() {
	//zlog_info(log, "Try to connect gnss tcp server");
	if (gnss_tcp_server_sock > 0) {
		close(gnss_tcp_server_sock);
	}
	struct sockaddr_in gnss_server;
	// 创建一个tcp 的 socket fd 连接 gnss 服务器
	while ((gnss_tcp_server_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP))
			< 0) {
		//zlog_warn(log, "Can't create socket fd with gnss tcp server");
		sleep(1);
	}

	// 尝试连接 gnss server, 这个是内网, 应该很容易连接上
	// 如果连接失败再重试
	memset(&gnss_server, 0, sizeof(gnss_server));
	gnss_server.sin_family = AF_INET;
	gnss_server.sin_addr.s_addr = inet_addr(config.aliyun_server_address);
	gnss_server.sin_port = htons(config.aliyun_server_port);
	while (connect(gnss_tcp_server_sock, (struct sockaddr *) &gnss_server,
			sizeof(gnss_server)) < 0) {
		//zlog_warn(log, "Can't connect to gnss tcp server");
		sleep(1);
	}
	//zlog_info(log, "Connect gnss tcp server success");
}

void * connect_gnss_tcp_server_thread() {
	connect_gnss_tcp_server();
	int count, retval;
	bool failed = false;
	while (1) {
		gnss_memnode_t * node = (gnss_memnode_t *) gnss_queue_pop(packet_queue);
		gnss_packet_t * packet = (gnss_packet_t *) node->data;
		count = 0;
		//packet->count=40;//重新修改字节数
		char sendBuf[120];
		memset(sendBuf,0,120);
		int code=100,i;
		char *end;//strlen(packet->buffer)
		memcpy(sendBuf,packet->buffer,strlen(packet->buffer));
		retval = write(gnss_tcp_server_sock,sendBuf,strlen(sendBuf));//retval保存真正发送的字节数


		gnss_mempool_return_node(node);
		// 失败重连
		if (failed) {
			connect_gnss_tcp_server();
		} else {
			//zlog_info(log, "Write %d bytes to gnss tcp server", count);
		}
		failed = false;
	}
	return (void *) 0;
}

int getWlan0Mac(char Save[]) //取Wlan0 Mac地址 save用来保存取到的MAC地址  成功返回1  失败返回 0
{
   char *device="wlan0"; //wlan0是网卡设备名
   unsigned char macaddr[ETH_ALEN]={0x00,0x00,0x00,0x00,0x00,0x00}; //ETH_ALEN（6）是MAC地址长度
   struct ifreq req;
   int err,i;

   int s=socket(AF_INET,SOCK_DGRAM,0); //internet协议族的数据报类型套接口
   strcpy(req.ifr_name,device); //将设备名作为输入参数传入
   err=ioctl(s,SIOCGIFHWADDR,&req); //执行取MAC地址操作
   close(s);
   if(err != -1)
   { memcpy(macaddr,req.ifr_hwaddr.sa_data,ETH_ALEN); //取输出的MAC地址
      for(i=0;i<ETH_ALEN;i++)
      {
			Save[2*i]=macaddr[i]/16<10?macaddr[i]/16+'0':macaddr[i]/16+'a'-10;
			Save[2*i+1]=macaddr[i]%16<10?macaddr[i]%16+'0':macaddr[i]%16+'a'-10;
	  }
	  return 1;
  }
  return 0;
}

int main() {
	//delete by zhuyong 2015.10.12
	//getWlan0Mac(MAC);//失败的MAC地址为"000000000000";

	printf("====MAC %s ====\n",MAC);
	int rc;
	//rc = zlog_init(LOG_CONF);
	if (rc) {
		fprintf(stderr, "初始化日志出错,请检查文件[%s]的格式\n", LOG_CONF);
		abort();
	}
	//log = zlog_get_category("gnss_log");
	/*
	if (!log) {
		fprintf(stderr, "配置文件[%s]中找不到gnss_log日志类别\n", LOG_CONF);
		//zlog_fini();
		exit(-1);
	}
	*/
	if (ini_parse(CONFIG, handle_config, &config) < 0) {
		//zlog_error(log, "Can't load '%s'", CONFIG);
		return 1;
	}

	//add by zhuyong 2015.10.12
	//strncpy(MAC,config.nmea_server_mac,12);

	signal(SIGPIPE, SIG_IGN);

	pthread_t data_source_thread, gnss_server_thread;

	packet_pool = gnss_mempool_create(sizeof(gnss_packet_t), 20);
	packet_queue = gnss_queue_create(10);
	int status;
	status = pthread_create(&data_source_thread, NULL,
			connect_data_server_thread, NULL);
	if (status != 0) {
		//zlog_error(log, "Error while create connect_data_server_thread");
		exit(1);
	}
	status = pthread_create(&gnss_server_thread, NULL,
			connect_gnss_tcp_server_thread, NULL);
	if (status != 0) {
		//zlog_error(log, "Error while create connect_gnss_tcp_server_thread");
		exit(1);
	}

	pause();
	//zlog_error(log, "Transfer client exit");
	return 0;
}
