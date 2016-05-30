/*
 * gps_mutex_queue.c
 *
 *  Created on: 2013年10月13日
 *      Author: chen
 */
#include <gnss_core.h>

/* 队列的初始化是不需要加锁的，在主线程中初始化 */
gnss_queue_t * gnss_queue_create(uint32_t total_queue_number) {
	gnss_queue_t * queue;
	queue = calloc(1, sizeof(gnss_queue_t));
	if (queue == NULL) {
		fprintf(stderr, "gnss_queue_create NO MEM");
		return NULL;
	}
	pthread_mutex_init(&(queue->mutex), NULL);
	pthread_cond_init(&(queue->empty), NULL);
	queue->items = (gnss_any_pt *) calloc(total_queue_number, sizeof(gnss_any_pt));
	queue->head = 0;
	queue->tail = 0;
	queue->total_queue_number = total_queue_number;
	return queue;
}

gnss_any_pt gnss_queue_pop(gnss_queue_t *queue) {
	gnss_any_pt type;
	pthread_mutex_lock(&(queue->mutex));
	// 这里要用while， 不能用if
	while (queue->tail == queue->head) {
		//队列为空
		pthread_cond_wait(&(queue->empty), &(queue->mutex));
	}
	type = queue->items[queue->head];
	//下标后移
	queue->head = (queue->head + 1) % queue->total_queue_number;
	pthread_mutex_unlock(&(queue->mutex));
	return type;
}

/*
 * 专门为中转程序改写了
 * 如果满了,直接把最开始的给删掉
 */
void gnss_queue_push(gnss_queue_t *queue, gnss_any_pt type){
	pthread_mutex_lock(&(queue->mutex));
	// 满了,删掉head处的点
	if((queue->tail+1-queue->head)%(queue->total_queue_number) == 0){
		gnss_memnode_t * node = (gnss_memnode_t *) queue->items[queue->head];
		gnss_mempool_return_node(node);
		queue->head = (queue->head + 1) % queue->total_queue_number;
	}
	queue->items[queue->tail] = type;
	queue->tail = (queue->tail + 1) % queue->total_queue_number;
	pthread_mutex_unlock(&(queue->mutex));
	pthread_cond_signal(&(queue->empty));
}

void gnss_queue_destory(gnss_queue_t *queue) {
	pthread_cond_destroy(&queue->empty);
	pthread_mutex_destroy(&queue->mutex);
	free(queue->items);
	free(queue);
}
