#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <string.h>
#include <iostream>
#include <atomic>

#include <elevator/driver.h>
#include <elevator/test.h>

void handler( int sig, siginfo_t *info, void * ) {
    elevator::Driver driver;
    driver.stopElevator();
    std::cerr << "elevator stopped" << std::endl;
    if ( sig != SIGCHLD ) {
        exit( sig );
    } else {
        int pid = info->si_pid;
        waitpid( pid, nullptr, WNOHANG );
    }
}

void startElevator();

void setupChild() {
    struct sigaction act;
    memset( &act, 0, sizeof( struct sigaction ) );
    act.sa_handler = SIG_DFL;
    sigaction( SIGCHLD, &act, nullptr );
    sigaction( SIGINT, &act, nullptr );

    elevator::Driver driver;
    std::cerr << "Child started" << std::endl;
    driver.init();
    while ( true ) {
        sleep( 1 );
        driver.goToFloor( 4 );
        sleep( 1 );
        driver.goToFloor( 1 );
    }
}

void watchChild( int childPid ) {
    while ( true ) {
        pause(); // wait for signal
        startElevator();
    }
}

void setupSignals() {
    struct sigaction act;
    memset( &act, 0, sizeof( struct sigaction ) );
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO;
    sigaction( SIGCHLD, &act, nullptr ); // child died
    sigaction( SIGINT, &act, nullptr ); // ctrl+c
}

void startElevator() {
    setupSignals();
    int pid = fork();
    if ( pid == 0 ) { // child
        setupChild();
    } else if ( pid > 0 ) { // parent
        watchChild( pid );
    } else {
        std::cerr << "Fatal error: fork failed" << std::endl;
        exit( 1 );
    }
}

int main() {
    startElevator();
    return 0;
}
