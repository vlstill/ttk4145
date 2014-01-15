// Go 1.2
// go run helloworld_go.go

package main

import (
    . "fmt"     // Using '.' to avoid prefixing functions with their package names
    . "runtime" //   This is probably not a good idea for large projects...
    . "time"
)

var i = 0

// version 1
//func adder(step int, iChan chan int, finished chan bool) {
// version 2
func adder(step int, semaphore chan bool, finished chan bool) {
    for x := 0; x < 1000000; x++ {
// version1
//        iLocal := <- iChan
//        iLocal += step
//        i = iLocal
//        iChan <- iLocal
// version 2
        <- semaphore
        i += step
        semaphore <- true
    }
    finished <- true
}

func main() {
    GOMAXPROCS(NumCPU())        // I guess this is a hint to what GOMAXPROCS does...

// version 1
//    iChan := make(chan int, 1) // sync for i
//    iChan <- i

// version 2
    semaphore := make(chan bool, 1)
    semaphore <- true

    finished := make(chan bool, 2) // sync for gorutines

// version 1
//    go adder(1, iChan, finished)                  // This spawns adder() as a goroutine
//    go adder(-2, iChan, finished)
// version 2
    go adder(1, semaphore, finished)
    go adder(-2, semaphore, finished)

    <- finished
    <- finished 

    Sleep(1000 * Millisecond)

    Println(i);
}
