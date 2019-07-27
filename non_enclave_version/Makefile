
#all:
#	$(MAKE) -f Makefile_sender
#	$(MAKE) -f Makefile_receiver
#
#%:
#	$(MAKE) -f Makefile_sender $@
#	$(MAKE) -f Makefile_receiver $@

CFLAGS= -g -std=c++11 -I /usr/local
CC=g++


TARGETS=sender receiver test rd_rx rd_tx
UTILS=util.o 

all: $(TARGETS)

$(UTILS): %.o: %.cpp %.hpp
	$(CC) $(CFLAGS) -c $<

%.o: %.cpp util.hpp
	$(CC) $(CFLAGS)  -c $< 


$(TARGETS): %:%.o util.o
	$(CC) $(CFLAGS) $^ -o $@


.PHONY:	clean

clean:
	rm *.o $(HELPERS) $(TARGETS) 

