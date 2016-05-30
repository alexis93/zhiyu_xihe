

rm -R ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libzhiyulib_intermediates
rm -R ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libmlplatform_intermediates
rm -R ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libsensorimu_intermediates
rm -R ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libins_intermediates
rm -R ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libxihelib_intermediates
rm -R ../../out/target/product/tiny4412/obj/SHARED_LIBRARIES/libzy_tcp_worker_intermediates
rm -R ../../out/target/product/tiny4412/obj/EXECUTABLES/IMU_gnss_intermediates

cd ../..
. setenv
mmm vendor/gnss_service_ins_ex/imu
sleep 5
mmm vendor/gnss_service_ins_ex/imulib
sleep 5
#mmm vendor/gnss_service_ins_ex/xihe
mmm vendor/gnss_service_ins_ex/iwise_gps_loc_hal/iwise_loc_xihelib
sleep 5
mmm vendor/gnss_service_ins_ex/iwise_gps_loc_hal
sleep 5
mmm vendor/gnss_service_ins_ex/tcpserver
sleep 5
mmm vendor/gnss_service_ins_ex/ins_logic_control
sleep 5

cp out/target/product/tiny4412/system/lib/libmlplatform.so vendor/gnss_service_ins_ex/output/libmlplatform.so
cp out/target/product/tiny4412/system/lib/libsensorimu.so vendor/gnss_service_ins_ex/output/libsensorimu.so
cp out/target/product/tiny4412/system/lib/libins.so vendor/gnss_service_ins_ex/output/libins.so
cp out/target/product/tiny4412/system/lib/libzhiyulib.so vendor/gnss_service_ins_ex/output/libzhiyulib.so
cp out/target/product/tiny4412/system/lib/libzy_tcp_worker.so vendor/gnss_service_ins_ex/output/libzy_tcp_worker.so
cp out/target/product/tiny4412/system/bin/IMU_gnss vendor/gnss_service_ins_ex/output/IMU_gnss






