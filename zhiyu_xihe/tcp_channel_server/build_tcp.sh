cd ../..
. setenv
mmm vendor/tcp_channel_server/
cp out/target/product/tiny4412/system/bin/tcp_channel_server vendor/tcp_channel_server/
adb push vendor/tcp_channel_server/tcp_channel_server /data/zhiyu/
cd vendor/tcp_channel_server/

