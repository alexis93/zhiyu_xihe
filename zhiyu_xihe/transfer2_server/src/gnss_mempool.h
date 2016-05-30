/*
 *gnss_mempool_t.h
 *
 * Created on: 2013年10月13日
 *     Author: chen
 */

#ifndef GNSS_MEMPOOL_H_
#define GNSS_MEMPOOL_H_

/*加锁版内存池 */
#include <gnss_core.h>

/*
 *每个节点到底需要哪些标识
 *owner 需不需要？
 *in_use 需要吗？
 **/
struct gnss_memnode_s{
	bool in_use;
	uint32_t count;
	uint32_t index;
	gnss_memnode_t *next;
	gnss_mempool_t *owner;
	char data[0];
};

struct gnss_mempool_s{
	pthread_mutex_t mutex;
	pthread_cond_t empty_pool_cond;
	size_t memnode_size;	/*memnode_size是指gnss_memnode_t中data字段的长度 */
	uint32_t max_used;
	uint32_t total_memnode_number;
	uint32_t free_memnode_number;
	gnss_memnode_t *current_free_memnode;
	gnss_memnode_t *memnode_pool;
};

gnss_mempool_t * gnss_mempool_create(size_t memnode_size, uint32_t memnode_number);
gnss_memnode_t * gnss_mempool_get_node(gnss_mempool_t *pool);
gnss_memnode_t * gnss_mempool_get_node_by_index(gnss_mempool_t *pool, uint32_t index);
void gnss_mempool_return_node(gnss_memnode_t *node);
void gnss_mempool_destory(gnss_mempool_t *pool);

#endif /*gnss_mempool_t_H_ */
