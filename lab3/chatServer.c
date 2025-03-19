#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int client_sockets[2] = {-1, -1}; // 存储两个客户端的 socket
int server_socket;

void handle_sigint(int sig) {
    printf("\n服务器正在正常关闭...\n");
    close(client_sockets[0]);
    close(client_sockets[1]);
    close(server_socket);
    exit(0);
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];

    signal(SIGINT, handle_sigint); // 捕获 Ctrl+C 以正常关闭服务器

    // 创建服务器 socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket 创建失败");
        exit(1);
    }

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定 socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("绑定失败");
        exit(1);
    }

    // 监听客户端连接
    if (listen(server_socket, 2) < 0) {
        perror("监听失败");
        exit(1);
    }

    printf("服务器已启动，等待客户端连接...\n");

    for (int i = 0; i < 2; i++) {
        addr_size = sizeof(client_addr);
        client_sockets[i] = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sockets[i] < 0) {
            perror("接受连接失败");
            exit(1);
        }
        printf("客户端 %d 已连接\n", i + 1);
    }

    printf("两个客户端已连接，可以开始聊天。\n");

    while (1) {
    for (int i = 0; i < 2; i++) {
        if (client_sockets[i] == -1) continue; // 如果客户端已断开，则跳过

        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sockets[i], buffer, BUFFER_SIZE, 0);
        
        if (bytes_received <= 0) {
            printf("客户端 %d 断开连接。\n", i + 1);
            close(client_sockets[i]);
            client_sockets[i] = -1; // 标记为无效
            continue;
        }

        printf("客户端 %d: %s\n", i + 1, buffer);

        // 如果客户端发送 "BYE"，服务器关闭
        if (strncmp(buffer, "BYE", 3) == 0) {
            printf("客户端 %d 结束聊天，服务器关闭。\n", i + 1);
            close(client_sockets[0]);
            close(client_sockets[1]);
            close(server_socket);
            exit(0);
        }

        // 确保目标客户端仍然连接
        if (client_sockets[1 - i] != -1) {
            int total_sent = 0;
            int len = strlen(buffer);
            while (total_sent < len) {
                int sent = send(client_sockets[1 - i], buffer + total_sent, len - total_sent, 0);
                if (sent == -1) {
                    perror("send 失败");
                    break;
                }
                total_sent += sent;
            }
        } else {
            printf("消息无法转发，因为客户端 %d 已断开。\n", 2 - i);
        }
    }
}
}
