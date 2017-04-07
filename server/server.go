package main

import (
	"bytes"
	"os/exec"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"net/url"
	"strings"
)

var serverErrors map[string]string
var domainName = "http://healthyair.ru/data_listener.php"
var userEmail = "muller95@yandex.ru"
var userPassword = "Warcraft123Ab"
var stationName = "station"

func main() {
	var out bytes.Buffer

	cmd := exec.Command("co2-handler")
	cmd.Stdout = &out
	err := cmd.Run()
	if err != nil {
		log.Fatal(err)
	}

	outputCO2 := strings.Split(out.String(), "\n");
	if outputCO2[0] == "ERR" {
		serverErrors["co2-handler"] = outputCO2[1]
	}
	out.Reset()

	cmd = exec.Command("sht-handler")
	cmd.Stdout = &out
	err = cmd.Run()
	if err != nil {
		log.Fatal(err)
	}

	outputSHT := strings.Split(out.String(), "\n");
	if outputSHT[0] == "ERR" {
		serverErrors["sht-handler"] = outputSHT[1]
	}

	fmt.Println(outputCO2, outputSHT)

	if outputCO2[0] == "OK" && outputSHT[0] == "OK" {
		shtMeasures := strings.Split(outputSHT[1], " ")
		fmt.Println(shtMeasures)
		reqData := url.Values{"email": {userEmail}, "passwd": {userPassword},
			"stname": {stationName}, "t": {shtMeasures[0]}, "rh": {shtMeasures[1]},
			"co2": {outputCO2[1]}}
		reqBody := bytes.NewBufferString(reqData.Encode())
		fmt.Println(reqData.Encode())
		resp, err := http.Post(domainName, "application/x-www-form-urlencoded", reqBody)
		if err != nil {
			log.Println(err)
		}

		if err == nil {
			defer resp.Body.Close()
			respData, err := ioutil.ReadAll(resp.Body)
			if err != nil {
				log.Println(err)
			}
			fmt.Println(string(respData[:]))
		}
	}
}
