package main

import (
    "fmt"
    "net"
    "runtime"
    "bufio"
)

func conTCP() {
    conn, err := net.Dial("tcp", "google.com:80")
    if err != nil {
      // handle error
    }
    fmt.Fprintf(conn, "GET / HTTP/1.0\r\n\r\n")
    status, err := bufio.NewReader(conn).ReadString('\000')
    fmt.PrintLn( status )
}

func main() {
    runtime.GOMAXPROCS( runtime.NumCPU() )
    conTCP()
}
