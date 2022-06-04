#include <iostream>
#include <stdio.h>
#include<string.h>
#include<pthread.h>
#include <cctype>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold

int server(void* a);

//////////////// QUEUE /////////////////

typedef struct Node{
    struct Node* left;
    struct Node* right;
    char val[256];
}node;

typedef struct Queue{
    node* head;
    node* tail;
    size_t size;

}queue;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

queue *createQ() {
    pthread_mutex_unlock(&mutex);
    queue* q = (queue*) malloc(sizeof(queue));
    q->tail= nullptr;
    q->head= nullptr;
    q->size=0;
    pthread_mutex_unlock(&mutex);
    return q;
}

void destoryQ(queue* q){
    pthread_mutex_unlock(&mutex);
    node* n=q->head;
    node* t;
    for (int i = 0; i < q->size; ++i) {
        t=n;
        n=n->right;
        free(t);
    }
    free(q);
    pthread_mutex_unlock(&mutex);
}

void enQ(queue* q,void* data){
    node* nn= (node*) malloc(sizeof(node));
    strcpy(nn->val,(char*)data);
    pthread_mutex_unlock(&mutex);
    if(q->head== nullptr){
        q->head=nn;
        q->tail=nn;
    }
    else{
        nn->right=q->head;
        q->head->left=nn;
        q->head=nn;
    }
    q->size++;
    pthread_mutex_unlock(&mutex);
}

std::string deQ(queue* q){
    std::string s;
    pthread_mutex_unlock(&mutex);
    if(q->tail!=NULL){
        node* t= q->tail;
        q->tail=q->tail->left;
        s=t->val;
        free(t);
    }
    q->size--;
    pthread_mutex_unlock(&mutex);
    return s;
}

void printQ(queue* q){
    if(q!= nullptr) {
        node *n = q->head;
        while (n) {
            n = n->right;
        }
    }
}

queue * getterQ=createQ();

//////////////// ACTIVE OBJECT /////////////////

typedef struct active_object{
    queue* theQ;
    pthread_t* thies;
}AO;

typedef struct Pipeline{
    AO *aos[3];
}pipeline;

AO* newAO(queue* q, void (*ptrF1)(), void (*ptrF2)() ){
    AO* ao = (AO*) malloc(sizeof(AO));
    ao->thies=new pthread_t[q->size+3];
    ao->theQ=q;
    size_t i =0;
    node* n=q->tail;

    while(n!= NULL){
        pthread_create(&ao->thies[i], NULL, reinterpret_cast<void *(*)(void *)>(ptrF1), n->val);

        n=n->left;
        i++;
    }
    ptrF2(); //done to take care of every element in the queue
    return ao;
}


void destroyAO(active_object *ao) {
    for (size_t i = 0; i < ao->theQ->size; i++) {
        pthread_cancel(ao->thies[i]);
    }
    destoryQ(ao->theQ);
    free(ao->thies);
    free(ao);
}

//////////////////// functions ///////////////

void Caesar_cipher_by_1(void* s){

    char* str=(char*) s;

    for (size_t i = 0; i < strlen(str); i++) {
        if(str[i]=='z') str[i]='a';
        else if(str[i]=='Z') str[i]='A';
        else if(str[i]==127) str[i]=0;
        else str[i]=str[i]+1;
    }
}

void flipCh(void* s){
    char* str=(char*) s;
    char ch;
    for (size_t i = 0; i < strlen(str); i++) {
        ch=str[i];
        if (ch<='z' and ch>='a'){
            str[i]=toupper(ch);
        }
        if (ch<='Z' and ch>='A'){
            str[i]=tolower(ch);
        }
    }
}

void doNothing(void* a){

}

void serverHelper(void* a){
    pipeline * p= (pipeline*) a;
    p->aos[0]= newAO(getterQ, reinterpret_cast<void (*)(void)>(doNothing), reinterpret_cast<void (*)(void)>(server));
    printQ(p->aos[0]->theQ);
}

void fliperHelper(void* a){
    pipeline * p= (pipeline*) a;
    p->aos[1]= newAO(getterQ, reinterpret_cast<void (*)(void)>(flipCh), reinterpret_cast<void (*)(void)>(doNothing));
}

void senderHelper(void* a){
    pipeline * p= (pipeline*) a;
    p->aos[2]= newAO(getterQ, reinterpret_cast<void (*)(void)>(flipCh), reinterpret_cast<void (*)(void)>(doNothing));
}

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sendMSG(void* a){
    int new_fd = *((int*)a);
    char buf[1024];

    if (recv(new_fd, buf, 1024,0) < 0)
        perror("recv");

    enQ(getterQ,buf);

    if (send(new_fd, "Hello, world!", 13, 0) == -1)
        perror("send");

    close(new_fd);
}

int server(void* a)
{
    pipeline *pip= (pipeline*) a;

    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    pthread_t thread_id[60];
    int i=0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);


        if (pthread_create(&thread_id[(i++)%60], NULL, reinterpret_cast<void *(*)(void *)>(sendMSG), &new_fd) != 0)
            printf("Failed to create thread\n");

        if (i<=60)
            pthread_join(thread_id[(i-1)],NULL);

    }
    return 0;
}


////////////////// Pipeline /////////////

pipeline* createPipeline(){
    pipeline* p = (pipeline*) malloc(sizeof(pipeline));
    pthread_t getter, fliper, sender;

    pthread_create(&getter, NULL, reinterpret_cast<void *(*)(void *)>(serverHelper), p); // will take care of aos
    pthread_create(&fliper , NULL, reinterpret_cast<void *(*)(void *)>(fliperHelper), p);
    pthread_create(&sender , NULL, reinterpret_cast<void *(*)(void *)>(senderHelper), p);

    return p;
}

void test_main1(){
    queue *q = createQ();
    char s[]="hii";
    enQ(q,s);
//    printQ(q);

//    AO * a= newAO(q, reinterpret_cast<void (*)(void)>(Caesar_cipher_by_1), reinterpret_cast<void (*)(void)>(doNothing));
//    printQ(a->theQ);
//    destroyAO(a);

    pipeline* p= createPipeline();

//    std::cout<< p;
//    printQ(p->aos[1]->theQ);


}

int main(){

    test_main1();

    return 1;
}

