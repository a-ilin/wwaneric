

ifeq ($(OS),Windows_NT)
    DEST=pdu.dll
else
    OS:=$(strip $(shell uname))
    ifeq ($(OS), Darwin)
        DEST=libpdu.dylib
        LINK=-dynamiclib -current_version 1.0.0
    else
        DEST=libpdu.so
    endif
endif


LINK=-shared


OBJ = pdu_base.o pdu_submit.o pdu_deliver.o pdu_packed.o pdu_address.o Pdu.o pdu_log.o pdu_userdata.o\
      pdu_datacoding.o pdu_7bit_packing.o pdu_8bit_packing.o pdu_userdataheader.o
	  
$(DEST):	$(OBJ)
	g++ $(LINK) -o $@ $(OBJ)

pdu_base.o: pdu_base.cpp pdu_base.h pdu_packed.h pdu_address.h pdu_log.h
	g++ -g -c -o  $@ $<
	
pdu_submit.o: pdu_submit.cpp pdu_submit.h
	g++ -g -c -o  $@ $<
	
pdu_deliver.o: pdu_deliver.cpp pdu_deliver.h pdu_userdata.h pdu_datacoding.h pdu_log.h pdu_packed.h pdu_base.h
	g++ -g -c -o  $@ $<

pdu_packed.o: pdu_packed.cpp pdu_packed.h pdu_address.h pdu_7bit_packing.h pdu_8bit_packing.h
	g++ -g -c -o  $@ $<
		
pdu_address.o: pdu_address.cpp pdu_address.h caststring.h
	g++ -g -c -o  $@ $<		
		
Pdu.o: Pdu.cpp Pdu.h pdu_deliver.h
	g++ -g -c -o  $@ $<		

pdu_log.o: pdu_log.cpp pdu_log.h 
	g++ -g -c -o  $@ $<		
		
pdu_userdata.o: pdu_userdata.cpp pdu_userdata.h pdu_log.h pdu_datacoding.h pdu_packed.h\
				pdu_7bit_packing.h pdu_8bit_packing.h pdu_userdataheader.h
	g++ -g -c -o  $@ $<	
	
pdu_datacoding.o: pdu_datacoding.cpp pdu_datacoding.h pdu_log.h
	g++ -g -c -o  $@ $<	
	
pdu_8bit_packing.o: pdu_8bit_packing.cpp pdu_8bit_packing.h pdu_log.h
	g++ -g -c -o  $@ $<	

pdu_7bit_packing.o: pdu_7bit_packing.cpp pdu_7bit_packing.h pdu_log.h
	g++ -g -c -o  $@ $<	

pdu_userdataheader.o: pdu_userdataheader.cpp pdu_userdataheader.h pdu_log.h
	g++ -g -c -o  $@ $<	

clean:
	rm -f *.o $(DEST)
