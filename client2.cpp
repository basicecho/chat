#include <iostream>
#include <thread>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

int serv_sock;
std::mutex mtx;

void error_handling(const std::string &);
void readThread();
void writeThread();

int main(int argc, char* argv[]) {
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1) {
        error_handling("socker() error");
    }

    struct sockaddr_in serv_adr;
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr("43.136.123.166");
    serv_adr.sin_port = htons(9190);

    if(connect(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
        error_handling("cononect() error");
    }
    else {
        std::cout << "Connect()........................" << std::endl;
    }

    std::cout << "请输入你的名字：";
    std::string name;
    std::cin >> name;
    write(serv_sock, name.c_str(), name.length());

    std::thread rThread(readThread);
    std::thread wThread(writeThread);

    rThread.join();
    wThread.join();

    close(serv_sock);

    return 0;
}

void error_handling(const std::string & message) {
    std::cerr << message << std::endl;
    exit(1);
}

void readThread() {
    while(true) {
        std::string message;
        std::cin >> message;
        write(serv_sock, message.c_str(), message.length());
    }
}

void writeThread() {
    char buf[BUF_SIZE];
    while(true) {
        int len;
        while((len = read(serv_sock, buf, BUF_SIZE - 1))) {
            buf[len] = 0;
            std::cout << buf << std::endl;
        }
    }
}