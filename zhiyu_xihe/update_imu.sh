adb remount


adb push output/libsensorimu.so /system/lib/
adb push output/libmlplatform.so /system/lib/
adb push output/libins.so /system/lib/
adb push output/libxihelib.so /system/lib/
adb push output/libzhiyulib.so /system/lib/
adb push output/libzy_tcp_worker.so /system/lib/
adb push output/IMU_gnss /data/zhiyu/

adb push output/udp_server /data/zhiyu/
adb push output/gnss_service_list /data/zhiyu/
adb push output/gnss_tran_pos /data/zhiyu/ 
adb push output/gnss_tran_pos_wuxi /data/zhiyu/
adb push output/gnss_tran_pos_aliyun /data/zhiyu/

adb push output/GpsTo433Client /data/zhiyu/
adb push output/tongxun2 /data/zhiyu/
adb push output/Usart /data/zhiyu/


adb push output/wpa_supplicant.conf /data/misc/wifi/
adb shell chown system:wifi /data/misc/wifi/wpa_supplicant.conf
adb shell chmod 660 /data/misc/wifi/wpa_supplicant.conf

