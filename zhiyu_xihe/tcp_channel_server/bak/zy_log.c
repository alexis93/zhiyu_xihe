/*
 * gnss_log.c
 *
 *  Created on: 2014年3月23日
 *      Author: zhuyong
 */
#include "zlog/zlog.h"
#include "zy_log.h"
zlog_category_t *serverlog;

int zy_log_init(void)
{
	int rc;
	rc = zlog_init(LOG_CONF);
	if (rc) {
		fprintf(stderr, "初始化日志出错,请检查文件[%s]的格式\n", LOG_CONF);
		puts("初始化日志出错,请检查文件的格式");
		return -1;
	}
	serverlog = zlog_get_category("server_log");
	if (!serverlog) {
		fprintf(stderr, "配置文件[%s]中找不到server_log日志类别\n", LOG_CONF);
		zlog_fini();
		return -1;
	}

	return 0;
}

