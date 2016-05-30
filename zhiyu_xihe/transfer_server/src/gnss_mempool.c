/*
 * gnss_mempool_t.c
 *
 *  Created on: 2013年10月13日
 *      Author: chen
 */
#include <gnss_core.h>

/*
 * 初始化为内存内一大片连续的数组，每个内存块大小固定
 * 仿照 nginx 的 ngx_connection_t 设计
 * 参考网页：http://blog.csdn.net/marcky/article/details/6041303
 * */
gnss_mempool_t * gnss_mempool_create(size_t memnode_size,
		uint32_t memnode_number) {
	gnss_mempool_t *pool = calloc(1, sizeof(gnss_mempool_t));
	if (pool == NULL) {
		fprintf(stderr, "gnss_mempool_create:NO MEM\n");
		return NULL;
	}
	pthread_mutex_init(&(pool->mutex), NULL);
	pthread_cond_init(&(pool->empty_pool_cond), NULL);
	pool->memnode_size = memnode_size;
	/* 分配固定大小的数组空间 */
	pool->memnode_pool = calloc(memnode_number,
			memnode_size + sizeof(gnss_memnode_t));
	if (pool->memnode_pool == NULL) {
		fprintf(stderr, "gnss_mempool_create:NO MEM\n");
		return NULL;
	}
	int i;
	gnss_memnode_t *p = pool->memnode_pool;
	for (i = 0; i < memnode_number - 1; i++) {
		// 标记不要从0开始，防止数据包默认为0
		p->in_use = false;
		p->index = i + 1;
		p->count = 0;
		p->owner = pool;
		/*
		 * 找到下一个gnss_memnode_t在内存中的地址
		 * 这个地方要把ｐ转换为(void *)或者(char *)
		 * 因为指针与数字相加地址偏移等于指针地址+数字*sizeof(指针所指类型)
		 * */
		p->next = (gnss_memnode_t *) ((char *) p
				+ (sizeof(gnss_memnode_t) + pool->memnode_size));
		p = p->next;
	}
	/* 最后一个节点的next为NULL */
	p->in_use = false;
	p->index = i + 1;
	p->count = 0;
	p->owner = pool;
	p->next = NULL;
	/* 第一个节点为空闲节点 */
	pool->current_free_memnode = pool->memnode_pool;
	pool->total_memnode_number = memnode_number;
	pool->free_memnode_number = memnode_number;
	pool->max_used = 0;
	return pool;
}

gnss_memnode_t * gnss_mempool_get_node(gnss_mempool_t *pool) {
	assert(pool != NULL);
	pthread_mutex_lock(&pool->mutex);

	//TODO 内存池的自动扩容
	gnss_memnode_t * memnode = pool->current_free_memnode;
	/* 已经没有空余的块了 */
	if (memnode == NULL) {
		pthread_mutex_unlock(&pool->mutex);
		return NULL;
	}
	// 取内存块的in_use标为true
	memnode->in_use = true;
	memnode->count++;
	pool->current_free_memnode = memnode->next;
	pool->free_memnode_number--;
	/* 统计最大使用内存 */
	if ((pool->total_memnode_number - pool->free_memnode_number)
			> pool->max_used) {
		pool->max_used = pool->total_memnode_number - pool->free_memnode_number;
	}
	pthread_mutex_unlock(&pool->mutex);
	return memnode;
}

/* 这个函数不需要加锁，直接访问地址 */
gnss_memnode_t * gnss_mempool_get_node_by_index(gnss_mempool_t *pool,
		uint32_t index) {
	if (index > pool->total_memnode_number || index < 1) {
		return NULL;
	}
	return (gnss_memnode_t *) ((char*) pool->memnode_pool
			+ (index - 1) * (pool->memnode_size + sizeof(gnss_memnode_t)));
}

void gnss_mempool_return_node(gnss_memnode_t *node) {
	assert(node != NULL);
	assert(node->owner != NULL);
	gnss_mempool_t *pool = node->owner;
	// 归还的时候要把该内存块的in_use标为false
	node->in_use = false;
	memset(node->data, 0, pool->memnode_size);
	pthread_mutex_lock(&pool->mutex);
	node->next = pool->current_free_memnode;
	pool->current_free_memnode = node;
	pool->free_memnode_number++;
	pthread_mutex_unlock(&pool->mutex);
	/* 把 memnode　的数据部分设置为0 */

}

void gnss_mempool_destory(gnss_mempool_t *pool) {
	if (pool != NULL) {
		pthread_cond_destroy(&pool->empty_pool_cond);
		pthread_mutex_destroy(&pool->mutex);
		if (pool->memnode_pool != NULL) {
			free(pool->memnode_pool);
		}
		free(pool);
	}
}

