
ifeq ($(OS),Windows_NT)
    PDULIB=pdu.dll
    DEST=pdutest.exe
else
    OS:=$(strip $(shell uname))
    ifeq ($(OS), Darwin)
        PDULIB=libpdu.dylib
        LINK=-dynamiclib -current_version 1.0.0
    else
        PDULIB=libpdu.so
    endif
    
    DEST=pdutest
endif


$(DEST):	pdutest.o ../src/$(PDULIB)
	g++  -g -L../src -lpdu -o $@ $<

pdutest.o: pdutest.cpp pdutest.h ../src/Pdu.h ../src/pdu_deliver.h ../src/pdu_userdata.h
	g++ -g -c -I../src -o  $@ $<
	
clean:
	rm -f *.o $(DEST)
