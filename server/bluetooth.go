package main

import (
	"log"
)

//#cgo CFLAGS: -I../bluetooth/include
//#cgo LDFLAGS: -L../bluetooth/ -lbluetooth
//#include <bluetooth.h>
import "C"

func bluetoothContext(config *config) {
	fd := int(C.bl_init(C.CString("/dev/ttyS2")))
	if fd < 0 {
		log.Println("@ERR INIT BLUETOOTH ", C.bl_errno) 		
	}
}
