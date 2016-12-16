<?php
	while (true) {
		fscanf(STDIN, "%lf %lf %lf\n", $t, $rh, $co2);	
		$datetime = date('Y-m-d H:i:s');
	//	$url = 	sprintf("192.168.0.2/clearair.com/data_listener.php?t=%lf&rh=%lf&co2=%lf", $t, $rh, $co2);
		//$url = 	sprintf("healthyair.ru/data_listener.php?t=%lf&rh=%lf&co2=%lf", $t, $rh, $co2);
		$post_fields = 	sprintf("email=muller95@yandex.ru&passwd=Warcraft123Ab&stname=station&t=%lf&rh=%lf&co2=%d&datetime=%s", $t, $rh, $co2, $datetime);
		$curl_context = curl_init();
		curl_setopt($curl_context, CURLOPT_RETURNTRANSFER, 1);
		
		curl_setopt($curl_context, CURLOPT_URL, "http://healthyair.ru/data_listener.php");
		curl_setopt($curl_context, CURLOPT_POST, 1);
		curl_setopt($curl_context, CURLOPT_POSTFIELDS, $post_fields);
		
		printf("%lf %lf %lf\n", $t, $rh, $co2);
		$result = curl_exec($curl_context);
		echo $result . "\n";
		curl_close($curl_context);
	}
?>
