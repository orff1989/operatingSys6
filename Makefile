CC=g++

all: shaO

shaO: main1.o guard.o singleton.o reactor.o
	$(CC) -o exeFile.so -shared main1.o guard.o singleton.o reactor.o -fPIC -lpthread

main1.o: main1.cpp
	$(CC) -c main1.cpp -fPIC -lpthread

guard.o: guard.cpp
	$(CC) -c guard.cpp -fPIC -lpthread

singleton.o: singleton.cpp
	$(CC) -c singleton.cpp -fPIC -lpthread

reactor.o: reactor.cpp
	$(CC) -c reactor.cpp -fPIC -lpthread

.PHONY: clean all

clean:
	rm -f *.o *.a *.out exeFile.so