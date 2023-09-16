#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <map>

#define CLNT_SIZE 256
#define BUF_SIZE 256

struct Client {
    int sock;
    std::string name;
};

void error_handling(const std::string &);

int main(int argc, char* argv[]) {
    int serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1) {
        error_handling("server sock() error");
    }

    struct sockaddr_in serv_adr;
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(9190);

    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
        error_handling("server bind() error");
    }
    if(listen(serv_sock, 20) == -1) {
        error_handling("server listen() error");
    }

    int epfd = epoll_create(CLNT_SIZE);
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event) == -1) {
        error_handling("epoll ctl add server error");
    }    

    std::map<int, Client> clients;

    while(true) {
        epoll_event events[CLNT_SIZE];
        int n = epoll_wait(epfd, events, CLNT_SIZE, -1);
        if(n < 0) {
            error_handling("epoll_wait() error");
        }
        
        for(int i = 0; i < n; i++) {
            int sock = events[i].data.fd;
            if(sock == serv_sock) {
                struct sockaddr_in clnt_adr;
                socklen_t clnt_adr_sz = sizeof(clnt_adr);
                int clnt_sock = accept(sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                if(clnt_sock == -1) {
                    error_handling("accept() error");
                }

                epoll_event event;
                event.events = EPOLLIN;
                event.data.fd = clnt_sock;
                if(epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event) == -1) {
                    error_handling("epoll ctl add client error");
                }

                Client client;
                client.sock = clnt_sock;
                client.name = "";
                clients[clnt_sock] = client;
                std::cout << "Connect fd: " << clnt_sock << " success" << std::endl;
            }
            else {
                std::cout << sock << std::endl;
                char buf[BUF_SIZE];
                int len = read(sock, buf, BUF_SIZE - 1);
                if(len == -1) {
                    error_handling("read() error");
                }
                else if(len == 0) {
                    close(sock);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, sock, nullptr);
                    clients.erase(sock);
                    std::cout << "Removed" << sock << std::endl;
                }
                else {
                    buf[len] = '\0';
                    if(clients[sock].name == "") {
                        clients[sock].name = buf;
                    }
                    else {
                        std::string message = '[' + clients[sock].name + "]: " + buf;
                        for(auto temp: clients) {
                            if(temp.first == sock) continue;
                            Client & client = temp.second;
                            write(client.sock, message.c_str(), message.length());
                        }
                    }
                }

            }
        }
    } 
    close(serv_sock);
    close(epfd);
    return 0;
}

void error_handling(const std::string & message) {
    std::cerr << message << std::endl;
    exit(1);
}