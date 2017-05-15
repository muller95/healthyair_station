package main

import (
	"encoding/json"
	"io/ioutil"
	"os"
)

type config struct {
	UserEmail string
	UserPasswd string
	StationName string
	StationPin string
}

func readConfig() config {
	var config config

	marshaled, err := ioutil.ReadFile("/etc/station.conf")
	if err != nil {
		config.StationName = "Healthy Station"
		config.StationPin = "1234"
		marshaled, _ = json.Marshal(config)
		ioutil.WriteFile("/etc/station.conf", marshaled, os.ModePerm)
	} else {
		json.Unmarshal(marshaled, &config)
	}

	/*DELETE AFTER BLUETOOTH CONFIG DONE*/
	config.StationName = "station"
	config.UserEmail = "muller95@yandex.ru"
	config.UserPasswd = "Warcraft123Ab"
	/***********************************/

	return config
}
