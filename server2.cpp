#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <map>

#define BUF_SIZE 1024
#define MAX_CLNT 1024

void error_handling(const char*);

std::string tempName = "游客";
int count = 0;

struct Client {
    int sock;
    std::string name;
};

int main(int argc, char* argv[]) {

    int serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1) {
        error_handling("server socket() error");
    }

    struct sockaddr_in serv_adr;
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(9190);
    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
        error_handling("server socket() error");
    }
    if(listen(serv_sock, 20) == -1) {
        error_handling("server listen() error");
    }


    int epfd = epoll_create(MAX_CLNT);
    struct epoll_event event;
    event.events = EPOLLIN;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event) == -1) {
        error_handling("server add error()");
    }

    std::map<int, Client>clients;

    while(true) {
        struct epoll_event events[MAX_CLNT];
        int n = epoll_wait(epfd, events, MAX_CLNT, -1);


        if(n < 0) {
            error_handling("epoll_wait() error");
        }

        for(int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if(fd == serv_sock) {

                struct sockaddr_in clnt_adr;
                socklen_t clnt_adr_sz = sizeof(clnt_adr);
                int clnt_sock = accept(fd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                if(clnt_sock == -1) {
                    error_handling("client accept() error");
                }

                struct epoll_event event;
                event.events = EPOLLIN;
                if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1) {
                    error_handling("client add error()");
                }

                Client client;
                client.sock = fd;
                client.name = tempName + std::to_string(++count);
                clients[fd] = client;

            }
            else {
                char buffer[BUF_SIZE];
                int n = read(fd, buffer, BUF_SIZE);

                if(n < 0) {
                    error_handling("read() error");
                }
                else if(n == 0) {
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    clients.erase(fd);
                }
                else {
                    std::string msg(buffer, n);
                    std::string name = clients[fd].name;

                    for(auto client: clients) {
                        if(client.first == fd)continue;
                        std::string message = '[' + name + ']' + ": " + msg;
                        write(client.first, message.c_str(), message.length());
                    }
                }

            }
        }
    }
    return 0;
}

void error_handling(const char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}