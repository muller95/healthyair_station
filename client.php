<?php
	$port = 10000;
	$address = "192.168.0.214";
	
	if (($sock = socket_create(AF_INET, SOCK_STREAM, SOL_TCP)) == false) {
		echo "Error on creating socket: " . socket_strerror(socket_last_error()) . "\n";
		exit(1);
	}

	if (!socket_connect($sock, $address, $port)) {
		echo "Error on creating socket: " . socket_strerror(socket_last_error()) . "\n";
		exit(1);
	}

	while (true) {
		$message = trim(fgets(STDIN));
		echo $message;
		for ($i = 0; $i  < strlen($message); $i++) {
			if (!($sent = socket_write($sock, $message[$i], 1))) {
				echo "Error on sending data: " . socket_strerror(socket_last_error()) . "\n";
				exit(1);
			}
		}
		
	}
?> 
