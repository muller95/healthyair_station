#!/bin/sh

set -x

cd co2-handler
make
cd ../sht-handler
make
cd ../
cd server
go build server.go


