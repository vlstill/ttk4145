// package udp
package main

import (
    "fmt"
    "net"
    "os"
    "strconv"
    "sync"
    "strings"
) 

const bcast = "129.241.187.255"
const udpPort = "20002" // workplace 2


// #### UDP CONNECTOR

func ConUDP( port string ) {
    // open sending side
    raddr, err := net.ResolveUDPAddr( "udp", net.JoinHostPort( bcast, port ) )
    if err != nil {
        fmt.Fprintln( os.Stderr, "Failed to resolve addr for " + bcast + ":" + port );
    }
    send, err := net.DialUDP( "udp", nil, raddr )
    if err != nil {
        fmt.Fprintln(os.Stderr, "UDP send connection error on " + raddr.String() )
        fmt.Fprintln(os.Stderr, err)
        return
    }
    defer send.Close()
    sendingFrom := send.LocalAddr()

    // open listening side
    laddr, err := net.ResolveUDPAddr( "udp", net.JoinHostPort( "", port ) )
    if err != nil {
        fmt.Fprintln( os.Stderr, "Failed to resolve addr for :" + port );
    }
    rcv, err := net.ListenUDP( "udp", laddr )
    if err != nil {
        fmt.Fprintln(os.Stderr, "UDP recv connection error on " + laddr.String() )
        fmt.Fprintln(os.Stderr, err)
        return
    }
    defer rcv.Close()

    // synchronization
    var start sync.WaitGroup
    var end sync.WaitGroup
    start.Add( 1 )
    end.Add( 1 )

    go udpListener( rcv, sendingFrom, &start, &end );

    fmt.Fprintln( os.Stderr, "Waiting for listener to start..." )
    start.Wait()
    fmt.Fprintln( os.Stderr, " OK" )

    for i := 0; i < 5; i++ {
        fmt.Fprintln( os.Stderr, "Sending message " + strconv.Itoa( i ) )
        _, err := fmt.Fprintf( send, "Hello " + strconv.Itoa( i ) )
        if err != nil {
            fmt.Fprintln( os.Stderr, "Error sending: " + err.Error() )
        }
    }

    fmt.Fprintln( os.Stderr, "Waiting for listener to terminate..." )
    fmt.Fprintf( send, "<<terminate>>" )
    end.Wait()
    fmt.Fprintln( os.Stderr, " terminated" )
}

func udpListener( conn *net.UDPConn, sendingFrom net.Addr, start *sync.WaitGroup, end *sync.WaitGroup ) {
    fmt.Fprintln( os.Stderr, "Started listener..." )
    start.Done()

    buff := make( []byte, 1600 ) // standard MTU size -- no packet should be bigger
    for i := 0; true; i++ {
        fmt.Fprintln( os.Stderr, "Waiting for packet #" + strconv.Itoa( i ) )
        len, from, err := conn.ReadFromUDP( buff )
        if err != nil {
            fmt.Fprintln( os.Stderr, "Error receiving UDP packet: " + err.Error() )
        }
        if from.String() == sendingFrom.String() {
            i--
            continue
        }

        str := string( buff[ :len ] )
        fmt.Println( "Received message from " + from.String() + "\n\t" + str )
        if strings.Contains( str, "<<terminate>>" ) {
            conn.Close()
            fmt.Fprintln( os.Stderr, "Terminated UDP listener" )
            end.Done()
            return
        }
    }
}

func main() {
    ConUDP( udpPort )
}
