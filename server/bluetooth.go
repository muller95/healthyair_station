package main

import (
	"log"
)

//#cgo LDFLAGS: -L/usr/lib -lbluetooth
//#include <bluetooth/bluetooth.h>
import "C"

func bluetoothContext(config *config) {
	fd := C.bl_init(C.CString("/dev/ttyS2"))
	if fd < 0 {
		log.Println("@ERR INIT BLUETOOTH ", C.bl_errno) 		
	}
	
	need_config := true;
	for {
		rc := C.bl_ok(fd)
		if rc < 0 {
			log.Println("@ERR CHECK BLUETOOTH ", C.bl_errno) 
			continue
		} else {
			log.Println("@OK CHECK")
		}

		if need_config {
			rc = C.bl_name(fd, C.CString(config.StationName))
			if rc < 0 {
				log.Println("@ERR SETUP BLUETOOTH NAME", C.bl_errno)
				continue
			} else {
				log.Println("@OK SETUP NAME")
			}
			rc = C.bl_pin(fd, C.CString(config.StationPin))
			if rc < 0 {
				log.Println("@ERR SETUP BLUETOOTH PIN", C.bl_errno)
				continue
			} else {
				log.Println("@OK SETUP PIN")
			}
			need_config = false
		}
	}
}
