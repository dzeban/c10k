package main

import (
	"bufio"
	"fmt"
	"log"
	"net"
	"time"
)

func main() {
	conn, err := net.Dial("tcp", "localhost:80")
	if err != nil {
		log.Fatal("dial error ", err)
	}

	fmt.Fprintf(conn, "GET /index.html HTTP/1.0\r\n\r\n")

	time.Sleep(5 * time.Second)
	body, err := bufio.NewReader(conn).ReadString('\n')
	if err != nil {
		log.Fatal("read error ", err)
	}

	fmt.Printf(body)
}
