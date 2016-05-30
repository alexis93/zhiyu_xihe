/*
 * zy_mempool.c
 *
 *  Created on: 2014年6月4日
 *      Author: zhuyong
 */

#include "zy_common.h"
#include "zy_mempool.h"
//#include "zy_tcp_worker.h"

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
		freenode->next=(zy_mempoolnode_t *)((uint8_t *)(zy_mempool->freelist)+((i+1)%memnode_number)*memnode_size);
		freenode->last=(zy_mempoolnode_t *)((uint8_t *)(zy_mempool->freelist)+((i==0)?(memnode_number-1):(i-1))*memnode_size);
		freenode->nodeID=i;
		freenode=freenode->next;
	}
	return 0;
}


int zy_mempool_get_node(zy_mempool_t *zy_mempool,zy_mempoolnode_t **freenode){
	zy_mempoolnode_t* memnode;

	if (zy_mempool->freecount>=1){
		memnode=zy_mempool->freelist;

		//从空闲链表中取出节点
		zy_mempool->freecount--;
		if (zy_mempool->freecount==0) {
			zy_mempool->freelist=NULL;
		} else {
			zy_mempool->freelist->last->next=zy_mempool->freelist->next;
			zy_mempool->freelist->next->last=zy_mempool->freelist->last;
			zy_mempool->freelist=zy_mempool->freelist->next;
		}

		//将节点放入使用链表
		if (zy_mempool->usecount==0) {
			zy_mempool->uselist=memnode;
			zy_mempool->uselist->last=memnode;
			zy_mempool->uselist->next=memnode;
		} else {
			memnode->last=zy_mempool->uselist->last;
			memnode->next=zy_mempool->uselist;
			zy_mempool->uselist->last->next=memnode;
			zy_mempool->uselist->last=memnode;
			zy_mempool->uselist=memnode;
		}
		zy_mempool->usecount++;

		(*freenode)=memnode;
		//print_all_usernode(zy_mempool);
		return 0;

	} else {
		//没有空闲的节点
		return -1;
	}

}


int zy_mempool_return_node(zy_mempool_t *zy_mempool,zy_mempoolnode_t *usenode){

			//从链表中取出节点
			zy_mempool->usecount--;
			if (zy_mempool->usecount==0) {
				zy_mempool->uselist=NULL;
			} else {
				usenode->last->next=usenode->next;
				usenode->next->last=usenode->last;
				if (zy_mempool->uselist==usenode)
				zy_mempool->uselist=zy_mempool->uselist->next;
			}

			//将节点放入使用链表
			if (zy_mempool->freecount==0) {
				zy_mempool->freelist=usenode;
				zy_mempool->freelist->last=usenode;
				zy_mempool->freelist->next=usenode;
			} else {
				usenode->last=zy_mempool->freelist->last;
				usenode->next=zy_mempool->freelist;
				zy_mempool->freelist->last->next=usenode;
				zy_mempool->freelist->last=usenode;
				zy_mempool->freelist=usenode;
			}
			zy_mempool->freecount++;
			//print_all_usernode(zy_mempool);
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

void print_all_usernode(zy_mempool_t *zy_mempool) {
	int i;
	char logbuf[1024];
	int buf_p=0;
	zy_mempoolnode_t *node;
	buf_p=sprintf(&logbuf,"usenode:%d=",zy_mempool->usecount);
	node=zy_mempool->uselist;
	for (i=0;i<zy_mempool->usecount;i++) {
		buf_p=buf_p+sprintf(&logbuf[buf_p],"node%d,",node->nodeID);
		node=node->next;
	}
	buf_p=buf_p+sprintf(&logbuf[buf_p],"\r\n");
	send_log(logbuf,buf_p);

}
