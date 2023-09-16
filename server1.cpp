#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <map>

#define BUF_SIZE 1024
#define MAX_CLNT 1024

void error_handling(const char* message) {
    printf("%s\n", message);
    exit(1);
}

struct Client {
    int sock;
    char* name;
};

int main(int argc, char* argv[]) {

    int serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1)
        error_handling("socket() error");

    sockaddr_in serv_adr;
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(9190);

    if(bind(serv_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if(listen(serv_sock, 30) == -1)
        error_handling("listen() error");

    int epfd = epoll_create(0);
    std::map<int, Client>clients;
    
    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    if((epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event)) == -1)
        error_handling("epoll_ctl() server error");
    
    while(true) {
        epoll_event events[MAX_CLNT];
        int n = epoll_wait(epfd, events, MAX_CLNT, -1);
        if(n < 0)
            error_handling("epoll_wait() error");
        for(int i = 0; i < n; i++) {

            int fd = events[i].data.fd;

            if(fd == serv_sock) {
                sockaddr_in clnt_adr;
                socklen_t clnt_adr_sz = sizeof(clnt_adr);
                int clnt_sock = accept(fd, (sockaddr*)&clnt_adr, &clnt_adr_sz);
                if(clnt_sock == -1)
                    error_handling("accept() error");
                
                epoll_event event;
                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                if(epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event) == -1) 
                    error_handling("epoll_ctl() client error");

                printf("%s Connected...\n", clnt_adr.sin_addr.s_addr);

                Client client;
                client.sock = clnt_sock;
                client.name = "";

                clients[clnt_sock] = client;

            }
            else {
                char buffer[BUF_SIZE];
                int n = read(fd, buffer, 1024);
                if(n < 0) {
                    close(fd);
                }
            }
        }

    }



    return 0;
}