// Go 1.2
// go run helloworld_go.go

package main

import (
    . "fmt"     // Using '.' to avoid prefixing functions with their package names
    . "runtime" //   This is probably not a good idea for large projects...
    . "sync"
)

func adder( step int, fin chan bool, sync chan bool, i *int, lock *Mutex ) {
    for x := 0; x < 1000000; x++ {
        <- sync // acquire
        *i += step
        sync <- true // release
    }
    fin <- true
}

func main() {
    GOMAXPROCS( NumCPU() )        // I guess this is a hint to what GOMAXPROCS does...
    
    fin := make( chan bool, 2 ) // buffer size 2
    sync := make( chan bool, 1 ) 
    var lock Mutex
    var i int = 0

    sync <- true // init channel

    go adder( 1, fin, sync, &i, &lock )                  // This spawns adder() as a goroutine
    go adder( -1, fin, sync, &i, &lock )

    <- fin
    <- fin

    Println(i);
}
