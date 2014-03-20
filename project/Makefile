all : build

build : _build
	cd _build; make elevator
	echo "binary is _build/elevator"
	echo "you can run in with ./_build/elevator -n <number-of-elevator>"

_build :
	./configure -DCMAKE_BUILD_TYPE=Release

unit : _build
	cd _build; make unit

