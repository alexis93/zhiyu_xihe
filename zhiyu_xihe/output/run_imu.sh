./udp_server &
./gnss_service_list &
./tcp_channel_server &
sleep 20
./IMU_gnss &
#sleep 5
#./gnss_tran_pos &
#sleep 5
#./gnss_tran_pos_wuxi &
sleep 5
./gnss_tran_pos_aliyun &
#./tongxun2 &
#./GpsTo433Client &


