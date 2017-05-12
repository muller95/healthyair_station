#ifndef BLUETOOTH_H
#define BLUETOOTH_H

enum BL_ERRNO {
	BL_ESUCCESS,
	BL_EWPINIT,
	BL_ESEROPEN,
};


extern enum BL_ERRNO bl_errno;

int bl_init(char *serial);

#endif 
