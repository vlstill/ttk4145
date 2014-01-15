// Go 1.2
// go run helloworld_go.go

package main

import (
    . "fmt"     // Using '.' to avoid prefixing functions with their package names
    . "runtime" //   This is probably not a good idea for large projects...
    . "sync"
)

func adder( step int, fin chan bool, i *int, lock *Mutex ) {
    for x := 0; x < 1000000; x++ {
        lock.Lock()
        *i += step
        lock.Unlock()
    }
    fin <- true
}

func main() {
    GOMAXPROCS(NumCPU())        // I guess this is a hint to what GOMAXPROCS does...
    
    fin := make( chan bool, 2 ) // buffer size 2
    var lock Mutex
    var i int = 0

    go adder( 1, fin, &i, &lock )                  // This spawns adder() as a goroutine
    go adder( -1, fin, &i, &lock )

    <- fin
    <- fin

    Println(i);
}
