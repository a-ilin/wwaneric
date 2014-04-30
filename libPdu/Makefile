
ifeq ($(OS),Windows_NT)
MAKE=mingw32-make
else
MAKE=make
endif

all: libpdu tests

libpdu:
	cd src && $(MAKE) -f Makefile	

tests:
	cd test && $(MAKE) -f Makefile
	
clean:
	cd src && $(MAKE) -f Makefile	clean
	cd test && $(MAKE) -f Makefile clean
	
ifneq ($(OS),Windows_NT)
install: libpdu
	rm -rf /usr/local/include/Pdu
	mkdir /usr/local/include/Pdu
	cp src/*.h /usr/local/include/Pdu
	cp src/libpdu.so /usr/local/lib
endif