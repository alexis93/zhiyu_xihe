/*
 * gnss_config.h
 *
 *  Created on: 2014年1月7日
 *      Author: chen
 */

#ifndef GNSS_CONFIG_H_
#define GNSS_CONFIG_H_

#include <gnss_core.h>

/* 这个版本专门为转发程序配置，不可通用 */

struct gnss_config_s{
	// 连接数据源的端口
	uint16_t data_server_port;
	// 连接客户端的端口
	uint16_t old_server_port;
	// 绑定的ip
	char * data_server_address;

	char * old_server_address;
	
	char * old_server_mac;

	char *mysqlserver_host;
	uint16_t mysqlserver_port;
	char *mysqlserver_usr;
	char *mysqlserver_pass;
	char *mysqlserver_db;
};


int ini_parse(const char* filename,
              int (*handler)(void* user, const char* section,
                             const char* name, const char* value),
              void* user);

/* Same as ini_parse(), but takes a FILE* instead of filename. This doesn't
   close the file when it's finished -- the caller must do that. */
int ini_parse_file(FILE* file,
                   int (*handler)(void* user, const char* section,
                                  const char* name, const char* value),
                   void* user);

/* Nonzero to allow multi-line value parsing, in the style of Python's
   ConfigParser. If allowed, ini_parse() will call the handler with the same
   name for each subsequent line parsed. */
#ifndef INI_ALLOW_MULTILINE
#define INI_ALLOW_MULTILINE 1
#endif

/* Nonzero to allow a UTF-8 BOM sequence (0xEF 0xBB 0xBF) at the start of
   the file. See http://code.google.com/p/inih/issues/detail?id=21 */
#ifndef INI_ALLOW_BOM
#define INI_ALLOW_BOM 1
#endif

/* Nonzero to use stack, zero to use heap (malloc/free). */
#ifndef INI_USE_STACK
#define INI_USE_STACK 0
#endif

/* Stop parsing on first error (default is to keep parsing). */
#ifndef INI_STOP_ON_FIRST_ERROR
#define INI_STOP_ON_FIRST_ERROR 0
#endif

/* Maximum line length for any line in INI file. */
#ifndef INI_MAX_LINE
#define INI_MAX_LINE 200
#endif



#endif /* GNSS_CONFIG_H_ */
