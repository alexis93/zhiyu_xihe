/*
 * zy_mempool.c
 *
 *  Created on: 2014年6月4日
 *      Author: zhuyong
 */


#include <stdio.h>
#include <stdlib.h>
#include "zy_mempool.h"

int zy_mempool_create(zy_mempool_t *zy_mempool,size_t memnode_size, int memnode_number)
{
	zy_mempool->maxcount=memnode_number;
	zy_mempool->memnode_size=memnode_size;
	zy_mempool->freecount=memnode_number;
	zy_mempool->usecount=0;
	zy_mempool->nodelist=calloc(memnode_number,memnode_size);//分配内存池管理节点内存
	zy_mempool->freelist=zy_mempool->nodelist;
	zy_mempool->uselist=NULL;

	zy_mempoolnode_t* freenode;
	freenode=(zy_mempoolnode_t *)(zy_mempool->freelist);

	int i;
	for(i=0;i<memnode_number;i++) {
		freenode->next=(zy_mempoolnode_t*)((char *)(zy_mempool->freelist)+((i+1)%memnode_number)*memnode_size);//printf("freenode->next=%p\n",freenode->next);
		freenode->last=(zy_mempoolnode_t*)((char *)(zy_mempool->freelist)+((i==0)?(memnode_number-1):(i-1))*memnode_size);
		freenode=freenode->next;
	}
	return 0;
}


int zy_mempool_get_node(zy_mempool_t *zy_mempool,zy_mempoolnode_t **freenode){
	zy_mempoolnode_t* memnode;
	if (zy_mempool->usecount==0) {
		//先从未使用队列取出来
		memnode=zy_mempool->freelist;
		zy_mempool->freelist->last->next=zy_mempool->freelist->next;
		zy_mempool->freelist->next->last=zy_mempool->freelist->last;
		zy_mempool->freelist=zy_mempool->freelist->next;
		zy_mempool->freecount--;
		//再放入已使用队列
		zy_mempool->uselist=memnode;
		zy_mempool->uselist->last=zy_mempool->uselist;
		zy_mempool->uselist->next=zy_mempool->uselist;
		zy_mempool->usecount++;

	} else if (zy_mempool->freecount>1){
		memnode=zy_mempool->freelist;
		zy_mempool->freelist->last->next=zy_mempool->freelist->next;
		zy_mempool->freelist->next->last=zy_mempool->freelist->last;
		zy_mempool->freelist=zy_mempool->freelist->next;
		zy_mempool->freecount--;

		memnode->last=zy_mempool->uselist;
		memnode->next=zy_mempool->uselist->next;
		zy_mempool->uselist->next->last=memnode;
		zy_mempool->uselist->next=memnode;
		//if(zy_mempool->usecount==1)
		//	zy_mempool->uselist->last=memnode;

		zy_mempool->usecount++;

	}else if (zy_mempool->freecount==1){
		memnode=zy_mempool->freelist;
		//zy_mempool->freelist->last->next=zy_mempool->freelist->next;
		//zy_mempool->freelist->next->last=zy_mempool->freelist->last;
		zy_mempool->freelist=NULL;
		zy_mempool->freecount--;

		memnode->last=zy_mempool->uselist;
		memnode->next=zy_mempool->uselist->next;
		zy_mempool->uselist->next->last=memnode;
		zy_mempool->uselist->next=memnode;

		zy_mempool->usecount++;

	}	else {
		return -1;
	}
	(*freenode)=memnode;
	return 0;
}


int zy_mempool_return_node(zy_mempool_t *zy_mempool,zy_mempoolnode_t *usenode){
	if (zy_mempool->freecount==0) {
		//先从已使用队列取出来
		usenode->last->next=usenode->next;
		usenode->next->last=usenode->last;
		if(usenode==zy_mempool->uselist)
			zy_mempool->uselist=zy_mempool->uselist->next;

		zy_mempool->usecount--;
		//再放入未使用队列
		zy_mempool->freelist=usenode;
		zy_mempool->freelist->last=zy_mempool->freelist;
		zy_mempool->freelist->next=zy_mempool->freelist;
		zy_mempool->freecount++;

	}else if (zy_mempool->usecount==1){
		//usenode->last->next=usenode->next;
		//usenode->next->last=usenode->last;
		//if(usenode==zy_mempool->uselist)
		//	zy_mempool->uselist=zy_mempool->uselist->next;
		zy_mempool->uselist=NULL;
		zy_mempool->usecount--;
		//再放入未使用队列
		usenode->last=zy_mempool->freelist;
		usenode->next=zy_mempool->freelist->next;
		zy_mempool->freelist->next->last=usenode;
		zy_mempool->freelist->next=usenode;
		//zy_mempool->freelist->last=usenode;

		zy_mempool->freecount++;

	}else{
		usenode->last->next=usenode->next;
		usenode->next->last=usenode->last;
		if(usenode==zy_mempool->uselist)
			zy_mempool->uselist=zy_mempool->uselist->next;

		zy_mempool->usecount--;
		//再放入未使用队列
		usenode->last=zy_mempool->freelist;
		usenode->next=zy_mempool->freelist->next;
		zy_mempool->freelist->next->last=usenode;
		zy_mempool->freelist->next=usenode;

		zy_mempool->freecount++;

	}

	return 0;

}

int zy_mempool_destory(zy_mempool_t *zy_mempool){
	if (zy_mempool->usecount>0){
		return -1;
	} else {
		//TODO用访问链表的方式挨个把内存释放掉
		free(zy_mempool->nodelist);//此处有内存泄漏
	}
	return 0;
}

