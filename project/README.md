# TTK4145 project -- elevator control software

© 2014 Vladimír Štill & Sameh Khalil

Redistributable under BSD3 license.

This project uses the wibble library, see
[http://repos.mornfall.net/wibble/embeddable/](http://repos.mornfall.net/wibble/embeddable/).
Wibble library is licensed under BSD and GPL licenses.

### Compilation

There is top-level makefile, so you can do just make in project directory,
it will take care of configuring cmake and building.

If you need more control you can run configure manually or (if you already
have the build directory) run `cmake .. [ options ]` in `_build` directory.
Then you can run `make elevator` or `make` (to run tests too) in
`_build` directory.

Both top-level make and configure will create `_build` directory in which
all building takes place.

In order to make it work with hardware you need comedi library.

### Usage

Simply run from commandline.

    ./elevator [ --avoid-recovery ] [ -N # | --nodes=# ]
    ./elevator { -v | --version }
    ./elevator { -h | -? | --help }

*   `-N # | --nodes=#` Specifies number of computers/elevators to use.
    All of them must be in same LAN. Program will wait for all nodes at
    the beginning.
*   `--avoid-recovery` Do not use the auto recovery after program crash.
    This is particularly usefull for debugging as recovery procedure
    requres fork.
