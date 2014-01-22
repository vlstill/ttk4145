package main

import (
    . "fmt"
    . "net"
    . "runtime"
)

func main() {
    GOMAXPROCS(NumCPU())

}
