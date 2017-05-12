#!/bin/sh

set -x

git add docs/*
git add configs/*
git add co2-handler/*.cpp co2-handler/*.h co2-handler/Makefile
git add sht-handler/*.cpp sht-handler/*.h sht-handler/Makefile
git add server/*.go
git add bluetooth/*.c bluetooth/*.h bluetooth/Makefile 
git add *.sh
