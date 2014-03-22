all : build

build : _build
	cd _build; make elevator
	echo "binary is _build/elevator"
	echo "you can run in with ./_build/elevator -n <number-of-elevator>"

_build :
	./configure -DCMAKE_BUILD_TYPE=Release

unit : _build
	cd _build; make unit

tarball : GRUPPE-W-21.tar.gz unit
	git archive -o GRUPPE-W-21.tar.gz --prefix=GRUPPE-W-21/ HEAD .

manual : manual.pdf

manual.pdf : README.md
	 pandoc README.md -o manual.pdf
