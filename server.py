#!/usr/bin/env python3

import hashlib
import _thread
import time

from validate_email import validate_email

class station_config:
	def set_name(self, name):
		if len(name) > 20:
			raise ValueError("Maximum name length is 20 characters")
		self.name = name

	def get_name(self):
		return self.name
	
	def set_pin(self, pin):
		if len(pin) != 4:
			raise ValueError("Pin must be 4 symbols len")
		if not pin.isdigit():
			raise ValueError("Pin must contain only digits")
		self.pin = pin

	def get_pin(self):
		return self.pin

	def set_user_email(self, email):
		if not validate_email.validate_email(email):
			raise ValueError("Email: " + email + " is invalid")		
		self.email = email

	def get_user_email(self):
		return self.email
	
	def set_user_password_hash(self, password_hash):
		if len(password_hash) == 0: 
			raise ValueError("Password hash can't have zero length")
		self.password_hash = password_hash
	
	def get_user_password_hash(self):
		return self.user_password_hash
		

def bluetooth_routine():
	need_config = True
	
	while True:
		if need_config:
			try:
				config_file = open("/etc/healthyair.conf")
			except FileNotFoundError:
				print('no config found')
			need_config = False
		
		print('b')
		time.sleep(1)
	

_thread.start_new_thread(bluetooth_routine, ())

while True:
	print('a')
	time.sleep(5)
