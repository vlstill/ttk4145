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

#include <elevator/io.h>
#include <elevator/channels.h>
#include <elevator/test.h>

#ifdef O_HAVE_LIBCOMEDI
#include <comedilib.h>


lowlevel::IO::IO( const char *device ){
    if ( device == nullptr )
        device = "/dev/comedi0";

    int status = 0;

    _comediHandle = comedi_open( device );

    assert( _comediHandle != nullptr, "Failed to open device" );

    for (int i = 0; i < 8; i++) {
        status |= comedi_dio_config(_comediHandle, PORT1, i,     COMEDI_INPUT);
        status |= comedi_dio_config(_comediHandle, PORT2, i,     COMEDI_OUTPUT);
        status |= comedi_dio_config(_comediHandle, PORT3, i+8,   COMEDI_OUTPUT);
        status |= comedi_dio_config(_comediHandle, PORT4, i+16,  COMEDI_INPUT);
    }

    assert(status == 0, "Device failure");
}

lowlevel::IO::~IO() {
    comedi_close( _comediHandle );
}

void lowlevel::IO::io_set_bit( int channel, bool value ) {
    comedi_dio_write(_comediHandle, channel >> 8, channel & 0xff, int( value ) );
}



void lowlevel::IO::io_write_analog( int channel, int value ) {
    comedi_data_write(_comediHandle, channel >> 8, channel & 0xff, 0, AREF_GROUND, value);
}



bool lowlevel::IO::io_read_bit( int channel ) {
    unsigned int data=0;
    comedi_dio_read(_comediHandle, channel >> 8, channel & 0xff, &data);

    return bool( data );
}



int lowlevel::IO::io_read_analog( int channel ) {
    lsampl_t data = 0;
    comedi_data_read(_comediHandle, channel >> 8, channel & 0xff, 0, AREF_GROUND, &data);

    return int( data );
}

#else // O_HAVE_LIBCOMEDI

#warning Using fake comedi library binding

#include <map>

struct comedi_t_struct {
    std::map< int, bool > setBits;
};

lowlevel::IO::IO( const char * ) {
    _comediHandle = new comedi_t();
}

lowlevel::IO::~IO() {
    delete _comediHandle;
}

void lowlevel::IO::io_set_bit( int channel, bool value ) {
    _comediHandle->setBits[ channel ] = value;
}


void lowlevel::IO::io_write_analog( int channel, int value ) {
    std::cout << "write analog, channel " << channel << " value " << value << std::endl;
}



bool lowlevel::IO::io_read_bit( int channel ) {
    auto it = _comediHandle->setBits.find( channel );
    return it != _comediHandle->setBits.end()
        ? it->second
        : false;
}



int lowlevel::IO::io_read_analog( int /* channel */ ) {
    assert_unimplemented();
}

#endif // O_HAVE_LIBCOMEDI



