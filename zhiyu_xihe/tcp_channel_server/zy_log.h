/*
 * gnss_log.h
 *
 *  Created on: 2014年3月23日
 *      Author: zhuyong
 */

#ifndef ZY_LOG_H_
#define ZY_LOG_H_

#define WRITE_TCP_LOG 1

//#include "zlog/zlog.h"

#define LOG_CONF	"log.conf"
extern int tcp_server_log;

#define SERVERLOG tcp_server_log

extern int zy_log_init(void);
#endif /* GNSS_LOG_H_ */
