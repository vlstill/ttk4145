// Go 1.2
// go run helloworld_go.go

package main

import (
    . "fmt"     // Using '.' to avoid prefixing functions with their package names
    . "runtime" //   This is probably not a good idea for large projects...
)

func adder( step int, fin chan bool, comm chan int ) {
    for x := 0; x < 1000000; x++ {
        comm <- step
    }
    fin <- true
}

func server( fin chan bool, comm chan int, read chan int ) {
    var i int = 0
    for {
        select {
            case <- fin:
                return
            case step := <- comm:
                i += step
            case read <- i:
        }
    }
}

func main() {
    GOMAXPROCS( NumCPU() )        // I guess this is a hint to what GOMAXPROCS does...
 
    addFin := make( chan bool, 2 )
    servFin := make( chan bool )
    comm := make( chan int )
    read := make( chan int )

    go server( servFin, comm, read )
    go adder( 1, addFin, comm )
    go adder( -1, addFin, comm )

    <- addFin // wait for adders
    <- addFin
    Println( <- read )
    servFin <- true // termitate server and wait for it
}
