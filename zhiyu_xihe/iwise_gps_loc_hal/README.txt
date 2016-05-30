该文件记录了iwise_gps_driver驱动的修改记录

********************************************************************************************
2015-06-10	  chenxi, lanzhigang
驱动目录架构 
iwise_loc_hal 	── Android.mk						makefile
				├── iwise_loc_base
				│   ├── clock_orbit_rtcm.c			钟差改正数解析
				│   ├── clock_orbit_rtcm.h
				│   ├── iwise_loc_base.h			驱动的基本的数据类型，统一驱动数据类型
				│   ├── iwise_loc_log.h				驱动的日志消息
				│   ├── iwise_loc_type.h			驱动的数据类型
				│   ├── iwise_nmea_parser.c			驱动的nmea解析
				│   ├── iwise_nmea_parser.h
				│   ├── iwise_pool.c				驱动的改正数解析的内存池
				│   ├── iwise_pool.h
				│   ├── iwise_serial_parser.c		驱动的gps数据解析模块
				│   ├── iwise_serial_parser.h
				│   └── Parserlib					解析库（参考rtklib）
				│       ├── Android.mk
				│       ├── convkml.c
				│       ├── datum.c
				│       ├── download.c
				│       ├── ephemeris.c
				│       ├── geoid.c
				│       ├── gps_common.h
				│       ├── ionex.c
				│       ├── lambda.c
				│       ├── options.c
				│       ├── preceph.c
				│       ├── qzslex.c
				│       ├── rcv
				│       │   ├── binex.c
				│       │   ├── crescent.c
				│       │   ├── gw10.c
				│       │   ├── javad.c
				│       │   ├── novatel.c
				│       │   ├── nvs.c
				│       │   ├── rcvlex.c
				│       │   ├── skytraq.c
				│       │   ├── ss2.c
				│       │   └── ublox.c
				│       ├── rcvraw.c
				│       ├── README
				│       ├── rinex.c
				│       ├── rinex_s.c
				│       ├── rtcm2.c
				│       ├── rtcm3.c
				│       ├── rtcm3e.c
				│       ├── rtcm.c
				│       ├── rtkcmn.c
				│       ├── rtklib.h
				│       ├── sbas.c
				│       ├── solution.c
				│       ├── stream.c
				│       ├── streamsvr.c
				│       ├── tags
				│       └── tle.c
				├── iwise_loc_hal.c						gps虚拟层抽象，对上层提供接口
				├── iwise_loc_handle_layer2				gps数据处理层
				│   ├── iwise_hardware_data_module.c	gps数据分离模块(nmea和原始数据)
				│   ├── iwise_hardware_data_module.h
				│   ├── iwise_loc_handle.c
				│   ├── iwise_loc_handle.h
				│   ├── iwise_loc_report_module.c		gps数据上报模块
				│   ├── iwise_loc_report_module.h
				│   ├── iwise_net_rtk_module.c
				│   ├── iwise_net_rtk_module.h
				│   ├── iwise_net_trans_module.c		gps驱动网络透传模块
				│   ├── iwise_net_trans_module.h
				│   ├── iwise_net_xihe_module.c			gps网络改正数模块
				│   ├── iwise_net_xihe_module.h
				│   ├── iwise_pos_module.c				gps定位模块
				│   └── iwise_pos_module.h
				├── iwise_loc_hardware_layer3			gps硬件抽象层，对上面调用隐藏gps芯片
				│   ├── iwise_loc_hardware.c
				│   ├── iwise_loc_hardware.h
				│   └── rcv
				│       ├── iwise_novatel.c
				│       └── iwise_ublox.c
				├── iwise_loc_interface_layer1			gps虚拟层抽象的具体实现
				│   ├── iwise_loc_interface.c
				│   └── iwise_loc_interface.h
				├── iwise_loc_xihelib					定位解算库
				│   ├── Android.mk
				│   ├── ctrl.txt
				│   └── libxihelib.so					（libLjrLib.so 重命名）
				└── README.txt

该版本的gps驱动采用了分层架构和模块化的重要思想。参考下面的图形（从下往上）
-------------------------------------------------------------------------------------
APP层(JVAV)
-------------------------------------------------------------------------------------
framework层 (JAVA)
-------------------------------------------------------------------------------------
JNI层 (C++)
-------------------------------------------------------------------------------------
HAL层    			iwise_loc_hal

					iwise_loc_interface
-------------------------------------------------------------------------------------
定位具体实现层			iwise_loc_handle

					iwise_loc_report_module
					
					iwise_loc_pos_module			

iwise_hardware_data_module  			iwise_net_xihe_module (iwise_net_tran_module) 
-------------------------------------------------------------------------------------
硬件抽象层			iwise_loc_hardware_layer
--------------------------------------------------------------------------------------
					LINUX Kernel
------------------------------------------------------------------
当前驱动结果
		1：该版本驱动可以运行
		2：该版本驱动可能存在小许逻辑问题
		3：该版本驱动的注释还没有添加完

***********************************************************************************************************
