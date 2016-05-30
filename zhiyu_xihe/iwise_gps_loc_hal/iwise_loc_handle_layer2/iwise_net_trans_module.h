#ifndef _IWISE_NET_MODULE_H_
#define _IWISE_NET_MODULE_H_

/*
 * 初始化网络模块
 * 在该初始化函数中会启动网络线程来获取改正数
 * */
int iwise_net_trans_module_init(iwise_loc_context_t* context);
int iwise_net_trans_module_destroy();

#endif
