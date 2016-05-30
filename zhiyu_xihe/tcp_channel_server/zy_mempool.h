/*
 * zy_mempool.h
 *
 *  Created on: 2014年6月4日
 *      Author: zhuyong
 */

#ifndef ZY_MEMPOOL_H_
#define ZY_MEMPOOL_H_
#include <stdio.h>
#include <stdint.h>

typedef struct zy_mempoolnode_s zy_mempoolnode_t;
struct zy_mempoolnode_s {
	zy_mempoolnode_t *last;
	zy_mempoolnode_t *next;
	int nodeID;
}__attribute__ ((aligned (4)));

typedef struct zy_mempool_s zy_mempool_t;
struct zy_mempool_s {
	int maxcount;
	int memnode_size;
	int usecount;
	int freecount;
	zy_mempoolnode_t *nodelist;
	zy_mempoolnode_t *freelist;
	zy_mempoolnode_t *uselist;
}__attribute__ ((aligned (4)));
int zy_mempool_new_node(zy_mempool_t *zy_mempool,size_t memnode_size);
int zy_mempool_free_node(zy_mempool_t *zy_mempool,zy_mempoolnode_t *node,size_t memnode_size);
int zy_mempool_create(zy_mempool_t *zy_mempool,size_t memnode_size, int memnode_number);
int zy_mempool_get_node(zy_mempool_t *zy_mempool,zy_mempoolnode_t **freenode);
int zy_mempool_return_node(zy_mempool_t *zy_mempool,zy_mempoolnode_t *usenode);
int zy_mempool_destory(zy_mempool_t *zy_mempool);

#endif /* ZY_MEMPOOL_H_ */
