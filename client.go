package main

import (
	"bufio"
	"fmt"
	"log"
	"net"
	"sync"
	"time"
)

func request(wg *sync.WaitGroup) {
	conn, err := net.Dial("tcp", "localhost:80")
	if err != nil {
		log.Fatal("dial error ", err)
	}

	time.Sleep(10 * time.Second)

	fmt.Fprintf(conn, "GET /index.html HTTP/1.1\r\n\r\n")

	_, err = bufio.NewReader(conn).ReadString('\n')
	if err != nil {
		log.Fatal("read error ", err)
	}

	wg.Done()
}

func main() {
	var wg sync.WaitGroup
	wg.Add(100)
	for i := 0; i < 100; i++ {
		go request(&wg)
	}

	wg.Wait()
}
