sudo initctl stop ticktocklogger
cd mcu && make clean && make flash
sudo initctl start ticktocklogger