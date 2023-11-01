#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

#define READ_RECV_MAX_BUF_SIZE 8 * 1024
#define READ_FILE_MAX_BUF_SIZE 1024 * 1024
#define SEND_MAX_BUF_SIZE      2 * 1024
#define READ_FILE_PATH         "./hls/data"

void process(int client_socket)
{
    //assume get http max size READ_RECV_MAX_BUF_SIZE
    char buf_recv[READ_RECV_MAX_BUF_SIZE] = { 0 };
    int recv_size = recv(client_socket, buf_recv, READ_RECV_MAX_BUF_SIZE, 0);
    printf("recv message:\n%s\n", buf_recv);

    //get uri
    char uri[100] = { 0 };
    const char* sep = "\n";
    char* line = strtok(buf_recv, sep);
    while (line) {
        if (strstr(line, "GET")) {
            if (sscanf(line, "GET %s HTTP/1.1\r\n", &uri) != 1) {
                printf("parse uri failed");
                return;
            }
        }
        line = strtok(NULL, sep);
    }
    printf("uri=%s\n\n", uri);

    //read data
    std::string file_path = READ_FILE_PATH + std::string(uri);
    FILE* fp = fopen(file_path.c_str(), "r");
    if (!fp) {
        printf("fopen %s failed.\n", file_path.c_str());
        fclose(fp);
        return;
    }

    char send_buf[READ_FILE_MAX_BUF_SIZE] = {0};
    int send_size = fread(send_buf, 1, sizeof(send_buf), fp);
    fclose(fp);
    printf("send message:\n%s\n", send_buf);

    //send message
    char http_headers[SEND_MAX_BUF_SIZE] = {0};
    if (std::string(uri) == "/index.m3u8") {
        sprintf(http_headers, "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: * \r\n"
            "Connection: keep-alive\r\n"
            "Content-Length: %d\r\n"
            "Content-Type: application/vnd.apple.mpegurl; charset=utf-8\r\n"
            "Keep-Alive: timeout=30, max=100\r\n"
            "Server: hlsServerDemo\r\n"
            "\r\n",
            send_size);
    } else {
        sprintf(http_headers, "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: * \r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "Content-Type: video/mp2t; charset=utf-8\r\n"
            "Keep-Alive: timeout=30, max=100\r\n"
            "Server: hlsServerDemo\r\n"
            "\r\n",
            send_size);
    }

    int http_headers_len = strlen(http_headers);
    send(client_socket, http_headers, http_headers_len, 0);
    send(client_socket, send_buf, send_size, 0);
    return;
}

/*
 * ffmpeg -i input.mp4 -codec copy -f hls -hls_time 10 -hls_list_size 0 index.m3u8
 	-hls_time n: 设置每片的长度，默认值为2,单位为秒
    -hls_list_size n:设置播放列表保存的最多条目，设置为0会保存有所片信息，默认值为5
 */

int main()
{
    uint16_t port = 8080;

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error when create socket.");
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("Error when binding socket.");
        return -1;
    }

    if (listen(server_socket, 100) != 0) {
        perror("Error when listening socket.");
        return -1;
    }

    printf("Create server socket success. Listen at port %d.\n", port);

    while (true) {
        struct sockaddr_in addr = {0};
        socklen_t len = sizeof(addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&addr, &len);
        char *ip_address = inet_ntoa(addr.sin_addr);
        printf("Client from %s:%d.\n", ip_address, addr.sin_port);

        if (client_socket < 0) {
            printf("Client connect failed.\n");
            return -1;
        }
        process(client_socket);
        close(client_socket);
    }
    
    return 0;
}