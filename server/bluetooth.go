package main

import (
	"log"
)

// #cgo LDFLAGS: -lwiringPi
// #include <wiringPi.h>
import "C"


func bluetoothContext(config *config) {
	if (C.wiringPiSetup() != 0) {
		log.Println("@ERR SETUPING WIRING PI")
		return
	}

	fd := C.serialOpen("/dev/ttyS1", 9600)
	if (fd < 0) {
		log.Println("@ERR OPENING SERIAL ")
	}
}
