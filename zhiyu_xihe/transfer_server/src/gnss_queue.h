/*
 * gps_mutex_queue.h
 *
 *  Created on: 2013年10月13日
 *      Author: chen
 */

#ifndef GNSS_QUEUE_H_
#define GNSS_QUEUE_H_

#include "gnss_core.h"

/*
 * 环形队列
 * 限定队列中的元素为指针
 * 指针所指向的内容在内存池中
 * */
struct gnss_queue_s {
	pthread_mutex_t mutex;
	pthread_cond_t empty;
	uint32_t total_queue_number;
	uint32_t head;
	uint32_t tail; /* 头尾都存储下标 */
	gnss_any_pt * items;
};

gnss_queue_t * gnss_queue_create(uint32_t total_queue_number);

gnss_any_pt gnss_queue_pop(gnss_queue_t *queue);

void gnss_queue_push(gnss_queue_t *queue, gnss_any_pt type);

void gnss_queue_destory(gnss_queue_t *queue);


#endif /* GPS_MUTEX_QUEUE_H_ */
