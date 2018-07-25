package main

import (
		"os"
		"net"
		//"net/http"
		"time"
		"sync"
		"fmt"
)

var (	
	mu		sync.Mutex
	count	int
	wg		sync.WaitGroup
)

const (
	MAX_CYCLE = 100
)

func Request(addr string) {
	mu.Lock()
	count += 1
	mu.Unlock()

	//var new_runner int
	//runner := <- baton	
	//fmt.Println("runner = ", runner)

	time.Sleep(time.Duration(1) * time.Millisecond)

	_, err := net.DialTimeout("tcp", addr, 900 * time.Duration(1) * time.Second)
	if err != nil {
		fmt.Println("dial error lidi ", err)
		os.Exit(-1)
	}
	println("connection count = ", count)

	/*
	strEcho := "Hello"
	_, err = conn.Write([]byte(strEcho))
	if err != nil {
		fmt.Println("Write to server failed:", err.Error())
	}		
	println("write to server = ", strEcho)
	*/

	/*
	new_runner = runner + 1

	if runner > MAX_CYCLE {
		fmt.Println("finish connection...")
		wg.Done()
		return
	}

	baton <- new_runner
	*/
}

func main() {
	fmt.Println("hello world")

	addr := "192.168.201.75:8888"
	//baton := make(chan int)
	//baton <- 1

	// wait coroutine finish
	wg.Add(MAX_CYCLE)
	for i:= 0; i < MAX_CYCLE; i++ {
		go Request(addr)	
	}	
	wg.Wait()

	fmt.Println("process over...")
}

/*		
	req, err := http.NewRequest("GET", "/index.html", nil)
	if err != nil {
		fmt.Println("failed to create http request")
	}

	req.Host = "localhost"

	err = req.Write(conn)
	if err != nil {
		fmt.Println("failed to send http request")
	}
*/
