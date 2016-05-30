/*
 * gnss_log.h
 *
 *  Created on: 2014年3月23日
 *      Author: zhuyong
 */

#ifndef ZY_LOG_H_
#define ZY_LOG_H_
/*
 * zlog_info(SERVERLOG, "listen on %s:%d error!!!", server_address,server_port);
 * zlog_debug(SERVERLOG,"");
 * zlog_error(SERVERLOG,"");
 * zlog_warn(SERVERLOG,"");
 *
 *
 *
 *
 *
 *
 *
 * */
#include "zlog/zlog.h"

#define LOG_CONF	"log.conf"
extern zlog_category_t *serverlog;

#define SERVERLOG serverlog
#define CLIENTLOG clientlog
#define REGISTERLOG registerlog

extern int zy_log_init(void);
#endif /* GNSS_LOG_H_ */
