adb remount
adb shell mkdir /data/zhiyu
adb shell mkdir /data/log
adb shell mkdir /data/log/imu
adb shell mkdir /data/log/ublox
adb shell mkdir /data/log/novatel


adb shell mkdir /data/log/xihe
adb shell mkdir /data/log/xihe/zdpos
adb shell mkdir /data/log/xihe/zdpos/midoutput
adb shell mkdir /data/log/xihe/zdpos/endoutput
adb shell mkdir /data/log/xihe/zdpos/iono
adb shell mkdir /data/log/xihe/zdpos/ambarc
adb shell mkdir /data/log/xihe/brdc
adb shell mkdir /data/log/xihe/igs
adb shell mkdir /data/log/xihe/eph
adb shell mkdir /data/log/xihe/eph/mass                                       
adb shell mkdir /data/log/xihe/meteorology                              
adb shell mkdir /data/log/xihe/obs
adb shell mkdir /data/zhiyu/xihe
adb shell mkdir /data/zhiyu/xihe/dcb
adb shell mkdir /data/zhiyu/xihe/table

adb push output/libsensorimu.so /system/lib/
adb push output/libmlplatform.so /system/lib/
adb push output/libins.so /system/lib/
adb push output/libxihelib.so /system/lib/
adb push output/libzhiyulib.so /system/lib/
adb push output/libzy_tcp_worker.so /system/lib/


adb push output/IMU_gnss /data/zhiyu/
adb push output/P1C1.DCB /data/zhiyu/xihe/dcb/
adb push output/P1P2.DCB /data/zhiyu/xihe/dcb/
adb push output/P2C2.DCB /data/zhiyu/xihe/dcb/
adb push output/antenna.pcv /data/zhiyu/xihe/table/
adb push output/leap.sec /data/zhiyu/xihe/table/

adb push output/udp_server /data/zhiyu/
adb push output/gnss_service_list /data/zhiyu/
adb push output/tcp_channel_server /data/zhiyu/
adb push output/tcp_channel_server.conf /data/zhiyu/


#adb push output/libLjrLib.so /system/lib/

adb push output/gnss_tran_pos /data/zhiyu/ 
adb push output/transfer.conf /data/zhiyu/
adb push output/gnss_tran_pos_wuxi /data/zhiyu/
adb push output/transfer_wuxi.conf /data/zhiyu/
adb push output/gnss_tran_pos_aliyun /data/zhiyu/
adb push output/transfer_aliyun.conf /data/zhiyu/


adb push output/GpsTo433Client /data/zhiyu/
adb push output/tongxun2 /data/zhiyu/
adb push output/Usart /data/zhiyu/
adb push output/clock_server.conf /data/zhiyu/
adb push output/log.conf /data/zhiyu/


#adb push output/mpu6050b1.ko /system/lib/
#adb push output/timerirq.ko /system/lib/


adb push output/loc_server.conf /data/zhiyu/
adb push output/ctrl.txt /data/zhiyu/
#adb push output/ctrl.txt /data/ljrublox/

adb push output/check_imu.sh /data/zhiyu/
adb push output/init_imu.sh /data/zhiyu/
adb push output/run_imu.sh /data/zhiyu/

adb push output/wpa_supplicant.conf /data/misc/wifi/
adb shell chown system:wifi /data/misc/wifi/wpa_supplicant.conf
adb shell chmod 660 /data/misc/wifi/wpa_supplicant.conf

