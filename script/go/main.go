package main

import (
	"fmt"
	"context"
	"time"
)

const (
	CONTEXT_TIMEOUT = 10
)

var (
	ctx_key string = "name"
)

func ContextDemo(ctx context.Context) {
	for {
		// wait for finish goroutine
		select {
			case <- ctx.Done():
				return
			default:				
				if deadline, ok := ctx.Deadline(); ok {
					fmt.Println(ctx.Value(ctx_key), "now:", time.Now(), "deadline:", deadline);
				}
				time.Sleep(time.Second)
		}
	}
}

func ContextTest() {
	fmt.Println("context test start!")

	// assign context with value
	timeout := CONTEXT_TIMEOUT * time.Second
	timeout_ctx, cancel:= context.WithTimeout(context.Background(), timeout)
	timeout_ctx = context.WithValue(timeout_ctx, ctx_key, "Timeout")

	go ContextDemo(timeout_ctx)

	i := 0
	for {
		// cancel context ahead
		if (i == CONTEXT_TIMEOUT/2) {
			cancel()
			break
		}
		//fmt.Println("main loop wait...")
		time.Sleep(time.Second)
		i++
	}	

	time.Sleep(5 * time.Second)
	fmt.Println("context test finish!")
}

func main() {
	ContextTest()
}
