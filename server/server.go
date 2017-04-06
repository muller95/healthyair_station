package main

import (
	"bytes"
	"os/exec"
	"fmt"
	"log"
	"strings"
)

var serverErrors map[string]string

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
}
