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

#ifndef __INCLUDE_IO_H__
#define __INCLUDE_IO_H__

// forward declare comedi_t, this is ugly because it depends on implementation
// of libComedi, but it works
struct comedi_t_struct;
typedef comedi_t_struct comedi_t;

namespace lowlevel {

struct IO {

    /**
      Initialize libComedi in "Sanntidssalen"
      @return Non-zero on success and 0 on failure
    */
    IO( const char *device = nullptr );
    ~IO();



    /**
      Sets a digital channel bit to diven value.
      @param channel Channel bit to set.
    */
    void io_set_bit(int channel, bool value);



    /**
      Writes a value to an analog channel.
      @param channel Channel to write to.
      @param value Value to write.
    */
    void io_write_analog(int channel, int value);



    /**
      Reads a bit value from a digital channel.
      @param channel Channel to read from.
      @return Value read.
    */
    bool io_read_bit(int channel);




    /**
      Reads a bit value from an analog channel.
      @param channel Channel to read from.
      @return Value read.
    */
    int io_read_analog(int channel);

  private:
    comedi_t *_comediHandle;
};

}

#endif // #ifndef __INCLUDE_IO_H__

