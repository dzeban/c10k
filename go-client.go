package main

import (
	"bufio"
	"flag"
	"log"
	"net"
	"net/http"
	"sync"
	"time"
)

func request(addr string, delay int, wg *sync.WaitGroup) {
	conn, err := net.Dial("tcp", addr)
	if err != nil {
		log.Fatal("dial error ", err)
	}

	time.Sleep(time.Duration(delay) * time.Second)

	req, err := http.NewRequest("GET", "/index.html", nil)
	if err != nil {
		log.Fatal("failed to create http request")
	}

	req.Host = "localhost"

	err = req.Write(conn)
	if err != nil {
		log.Fatal("failed to send http request")
	}

	_, err = bufio.NewReader(conn).ReadString('\n')
	if err != nil {
		log.Fatal("read error ", err)
	}

	wg.Done()
}

func main() {
	var (
		addr  string
		n     int
		delay int
	)

	flag.StringVar(&addr, "addr", "127.0.0.1:80", "Address to connect")
	flag.IntVar(&n, "n", 1, "Number of connections")
	flag.IntVar(&delay, "d", 5, "Delay in seconds")
	flag.Parse()

	var wg sync.WaitGroup
	wg.Add(n)
	for i := 0; i < n; i++ {
		go request(addr, delay, &wg)
	}

	wg.Wait()
}
