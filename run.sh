#1/bin/bash

case $1 in

insert)
make sudo insmod main.ko
sudo chmod 777 /dev/test_file

;;

remove)
sudo rmmod main
make clean

;;

test)
sudo rmmod main
make clean
make
sudo insmod main.ko
sudo chmod 777 /dev/test_file
dmesg


;;

esac