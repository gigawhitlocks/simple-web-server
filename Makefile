CXX = g++ -fPIC
NETLIBS= -lnsl -lsocket

all: myhttpd daytime-client use-dlopen hello.so

daytime-client : daytime-client.o
	$(CXX) -o $@ $@.o $(NETLIBS)

myhttpd : myhttpd.o
	$(CXX) -g -o $@ $@.o $(NETLIBS)

use-dlopen: use-dlopen.o
	$(CXX) -o $@ $@.o $(NETLIBS) -ldl

hello.so: hello.o
	ld -G -o hello.so hello.o

myhttpd.o: myhttpd.cpp
	@echo 'Building $@ from myhttpd.cpp'
	$(CXX) -o $@ -c -I. myhttpd.cpp 

%.o: %.cc
	@echo 'Building $@ from $<'
	$(CXX) -g -o $@ -c -I. $<

clean:
	rm -f *.o use-dlopen hello.so
	rm -f *.o daytime-server daytime-client

