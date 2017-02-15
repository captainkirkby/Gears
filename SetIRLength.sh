sudo initctl stop ticktocklogger
cd ~/Gears/mcu
make clean && make flash
sudo initctl start ticktocklogger
