package main

import (
    . "fmt"
    . "runtime"
)

func server(in chan bool, out chan int) {
    i := 0
    for {
        select {
        case action, ok := <-in:
            if !ok { return }
            if action {
                i++
            } else  {
                i -= 2
            }
        case out <- i:
        }
    }
    Println(i)
}

func adder(signal bool, in chan bool, finished chan bool) {
    for x := 0; x < 100000; x++ {
        in <- signal
    }
    finished <- true
}

func main() {
    GOMAXPROCS(NumCPU())

    in := make(chan bool)
    out := make(chan int)
    finished := make(chan bool, 2)

    go server(in, out)
    go adder(true, in, finished)
    go adder(false, in, finished)

    <- finished
    <- finished

    Println(<-out)
    close(in)
}
