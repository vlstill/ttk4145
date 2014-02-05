// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

/* C++ inteface for libComedi and elevator, based on wrapping
 * original C interface by Martin Korsgaard (see below)
 *
 * Almost the only change is that everything is wrapped in struct
 * to avoid global variable and hardcoded device name
 */

// Wrapper for libComedi I/O.
// These functions provide and interface to libComedi limited to use in
// the real time lab.
//
// 2006, Martin Korsgaard

#include "io.h"
#include "channels.h"
#include "test.h"

#include <comedilib.h>


lowlevel::IO::IO( const char *device ){
    if ( device == nullptr )
        device = "/dev/comedi0";

    int status = 0;

    it_g = comedi_open( device );

    assert( it_g != nullptr, "Failed to open device" );

    for (int i = 0; i < 8; i++) {
        status |= comedi_dio_config(it_g, PORT1, i,     COMEDI_INPUT);
        status |= comedi_dio_config(it_g, PORT2, i,     COMEDI_OUTPUT);
        status |= comedi_dio_config(it_g, PORT3, i+8,   COMEDI_OUTPUT);
        status |= comedi_dio_config(it_g, PORT4, i+16,  COMEDI_INPUT);
    }

    assert(status == 0, "Device failure");
}



void lowlevel::IO::io_set_bit( int channel ) {
    comedi_dio_write(it_g, channel >> 8, channel & 0xff, 1);
}



void lowlevel::IO::io_clear_bit( int channel ) {
    comedi_dio_write(it_g, channel >> 8, channel & 0xff, 0);
}



void lowlevel::IO::io_write_analog( int channel, int value ) {
    comedi_data_write(it_g, channel >> 8, channel & 0xff, 0, AREF_GROUND, value);
}



int lowlevel::IO::io_read_bit( int channel ) {
    unsigned int data=0;
    comedi_dio_read(it_g, channel >> 8, channel & 0xff, &data);

    return (int)data;
}



int lowlevel::IO::io_read_analog( int channel ) {
    lsampl_t data = 0;
    comedi_data_read(it_g, channel >> 8, channel & 0xff, 0, AREF_GROUND, &data);

    return (int)data;
}




