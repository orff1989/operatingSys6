#include <stdlib.h>
#include <unistd.h>

using namespace std;

class reactor{
public:
    int _fd;
    reactor(){}
};

void* newReactor(){
    return new reactor();
}

void InstallHandler(reactor* r, void (*func)(char*), int fd){
    char txt[1024];
    r->_fd=fd;
    read(r->_fd,txt,1024);
    func(txt);
}

void RemoveHandler(reactor* r){
    free(r);
}