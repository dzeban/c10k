package main

import (
	"bufio"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"sync"
	"time"
)

func request(addr string, delay, requests int, wg *sync.WaitGroup) {
	conn, err := net.Dial("tcp", addr)
	if err != nil {
		log.Fatal("dial error ", err)
	}

	for i := 0; i < requests; i++ {
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

		respReader := bufio.NewReader(conn)
		resp, err := http.ReadResponse(respReader, req)
		if err != nil {
			log.Fatal("read reponse error ", err)
		}

		body, err := ioutil.ReadAll(resp.Body)
		if err != nil {
			log.Fatal("body read error ", err)
		}
		fmt.Println(resp.Status, string(body))

		resp.Body.Close()
	}
	conn.Close()

	wg.Done()
}

func main() {
	var (
		addr     string
		n        int
		requests int
		delay    int
	)

	flag.StringVar(&addr, "addr", "127.0.0.1:80", "Address to connect")
	flag.IntVar(&n, "n", 1, "Number of connections")
	flag.IntVar(&requests, "r", 1, "Number of requests per connection")
	flag.IntVar(&delay, "d", 5, "Delay in seconds")
	flag.Parse()

	var wg sync.WaitGroup
	wg.Add(n)
	for i := 0; i < n; i++ {
		go request(addr, delay, requests, &wg)
	}

	wg.Wait()
}
