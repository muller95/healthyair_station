#!/bin/sh

set -x

cd co2-handler
make
sudo cp co2-handler /bin

cd ../sht-handler
make
sudo cp sht-handler /bin

cd ../
cd bluetooth
make
sudo cp libbluetooth.so /usr/lib/
sudo mkdir /usr/include/bluetooth
sudo cp bluetooth.h /usr/include/bluetooth/

cd ..
cd server
go build server.go config.go bluetooth.go
sudo cp server /bin


