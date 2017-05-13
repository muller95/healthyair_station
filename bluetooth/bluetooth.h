#ifndef BLUETOOTH_H
#define BLUETOOTH_H

enum BL_ERRNO {
	BL_ESUCCESS,
	BL_EWPINIT,
	BL_ESEROPEN,
	BL_EAT,
	BL_ETIMEOUT,
	BL_EINVAL
};


extern enum BL_ERRNO bl_errno;

int bl_init(char *serial);
int bl_ok(int fd);
int bl_name(int fd, char *name);

#endif 
