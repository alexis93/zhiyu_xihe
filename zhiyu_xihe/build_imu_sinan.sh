cd ../..
. setenv
mmm vendor/zhiyu_xihe/imu
sleep 5
mmm vendor/zhiyu_xihe/imulib
sleep 5
#mmm vendor/gnss_service_ins_ex/xihe
mmm vendor/zhiyu_xihe/iwise_gps_loc_hal/iwise_loc_xihelib
sleep 5
cp vendor/zhiyu_xihe/iwise_gps_loc_hal/Android.mk.sinan vendor/zhiyu_xihe/iwise_gps_loc_hal/Android.mk
mmm vendor/zhiyu_xihe/iwise_gps_loc_hal
sleep 5
mmm vendor/zhiyu_xihe/tcpclient
sleep 5
cp vendor/zhiyu_xihe/ins_logic_control/Android.mk.sinan vendor/zhiyu_xihe/ins_logic_control/Android.mk
mmm vendor/zhiyu_xihe/ins_logic_control
sleep 5
mmm vendor/zhiyu_xihe/gnss_udp_server
sleep 5
mmm vendor/zhiyu_xihe/gnss_service_list
sleep 5
mmm vendor/zhiyu_xihe/tcp_channel_server
sleep 5
mmm vendor/zhiyu_xihe/transfer_server
sleep 5
mmm vendor/zhiyu_xihe/transfer2_server
sleep 5
mmm vendor/zhiyu_xihe/transfer3_server
sleep 5





cp out/target/product/tiny4412/system/lib/libmlplatform.so vendor/zhiyu_xihe/output/libmlplatform.so
cp out/target/product/tiny4412/system/lib/libsensorimu.so vendor/zhiyu_xihe/output/libsensorimu.so
cp out/target/product/tiny4412/system/lib/libins.so vendor/zhiyu_xihe/output/libins.so
cp out/target/product/tiny4412/system/lib/libzhiyulib.so vendor/zhiyu_xihe/output/libzhiyulib.so
#cp out/target/product/tiny4412/system/lib/libxihelib.so vendor/zhiyu_xihe/output/libxihelib.so
cp out/target/product/tiny4412/system/lib/libzy_tcp_worker.so vendor/zhiyu_xihe/output/libzy_tcp_worker.so
cp out/target/product/tiny4412/system/bin/IMU_gnss vendor/zhiyu_xihe/output/IMU_gnss
cp out/target/product/tiny4412/system/bin/tcp_channel_server vendor/zhiyu_xihe/output/tcp_channel_server
cp out/target/product/tiny4412/system/bin/udp_server vendor/zhiyu_xihe/output/udp_server
cp out/target/product/tiny4412/system/bin/gnss_service_list vendor/zhiyu_xihe/output/gnss_service_list
cp out/target/product/tiny4412/system/bin/gnss_tran_pos vendor/zhiyu_xihe/output/gnss_tran_pos

#cp vendor/gnss_service_ins/tcpserver/loc_server.conf vendor/zhiyu_xihe/loc_server.conf
cp out/target/product/tiny4412/system/bin/gnss_tran_pos_wuxi vendor/zhiyu_xihe/output/gnss_tran_pos_wuxi
cp out/target/product/tiny4412/system/bin/gnss_tran_pos_aliyun vendor/zhiyu_xihe/output/gnss_tran_pos_aliyun

#cp vendor/gnss_service_ins/iwise_gps_loc_hal/iwise_loc_xihelib/ctrl.txt vendor/zhiyu_xihe/output/ctrl.txt

cd ..
#ls ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libzhiyulib_intermediates
#ls ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libmlplatform_intermediates
#ls ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libsensorimu_intermediates
#ls ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libins_intermediates
#ls ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libzhiyulib_intermediates
#ls ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libxihelib_intermediates
#ls ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libzy_tcp_worker_intermediates
#ls ../../out/target/product/tiny4412/obj/EXECUTABLES/IMU_gnss_intermediates


