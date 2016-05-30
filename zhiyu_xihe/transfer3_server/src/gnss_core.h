/*
 * gnss_core.h
 *
 *  Created on: 2013年12月20日
 *      Author: chen
 */

#ifndef GNSS_CORE_H_
#define GNSS_CORE_H_

typedef void*  gnss_any_pt;

typedef struct gnss_memnode_s		gnss_memnode_t;
typedef struct gnss_mempool_s		gnss_mempool_t;

typedef struct gnss_queue_s			gnss_queue_t;

typedef struct gnss_packet_s	gnss_packet_t;
typedef struct gnss_client_packet_s gnss_client_packet_t;

typedef struct gnss_config_s	gnss_config_t;

#define gnss_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define gnss_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

//#include <zlog/zlog.h>

#include "gnss_queue.h"
#include "gnss_mempool.h"
#include "gnss_config.h"


#endif /* GNSS_CORE_H_ */
