/*
 * zy_config.h
 *
 *  Created on: 2014年1月7日
 *      Author: chen
 */

#ifndef ZY_CONFIG_H_
#define ZY_CONFIG_H_
#include "zy_common.h"
#include <stdint.h>
#include <unistd.h>


typedef struct gnss_config_s		gnss_config_t;
struct gnss_config_s{
	
	char * nmea_server_address;
	uint16_t nmea_server_port;
	
	char * gnss_server_address;
	uint16_t gnss_server_port;
	
	char * gnss_nmea_server_address;
	uint16_t gnss_nmea_server_port;
	
	char * ins_server_address;
	uint16_t ins_server_port;
	
	char * ins_nmea_server_address;
	uint16_t ins_nmea_server_port;
	
	char * log_tcpserver_address;
	uint16_t log_tcpserver_port;

	char * obs_log_tcpserver_address;
	uint16_t obs_log_tcpserver_port;

	char * eph_log_tcpserver_address;
	uint16_t eph_log_tcpserver_port;

	char * clockorbit_log_tcpserver_address;
	uint16_t clockorbit_log_tcpserver_port;

	char * msg_log_tcpserver_address;
	uint16_t msg_log_tcpserver_port;

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

#define CONFIG		"loc_server.conf"

extern gnss_config_t config;
extern int handle_config(void* user, const char* section, const char* name,
		const char* value);
extern int init_config(void);
#endif /* ZY_CONFIG_H_ */
