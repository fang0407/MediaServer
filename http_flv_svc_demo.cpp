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
#define SEND_MAX_BUF_SIZE      10 * 1024
#define READ_FILE_PATH         "./flv/test.flv"

void process(int client_socket, FILE* fp)
{
    //assume get http max size READ_RECV_MAX_BUF_SIZE
    char buf_recv[READ_RECV_MAX_BUF_SIZE] = { 0 };
    int recv_size = recv(client_socket, buf_recv, READ_RECV_MAX_BUF_SIZE, 0);
    printf("recv message:\n%s\n", buf_recv);

    char send_buf[SEND_MAX_BUF_SIZE] = {0};

    //send msg
    int count = 0;
    while (true) {
        if (count == 0) {
            char http_headers[] = \
                "HTTP/1.1 200 OK\r\n" \
                "Access-Control-Allow-Origin: * \r\n" \
                "Content-Type: video/x-flv\r\n" \
        	    "Connection: Keep-Alive\r\n" \
        	    "Expires: -1\r\n" \
        	    "Pragma: no-cache\r\n" \
        	    "\r\n";

            int http_headers_len = strlen(http_headers);
            send(client_socket, http_headers, http_headers_len, 0);
            printf("send message:\n%s\n", http_headers);
        } else {
            usleep(1000 * 100);
            memset(send_buf, 0, sizeof(send_buf));
            int send_size = fread(send_buf, 1, sizeof(send_buf), fp);
            if (send_size <= 0) {
                printf("flv file read finish\n");
                break;
            }
            send(client_socket, send_buf, send_size, 0);
        }
        count++;
    }

    return;
}

int main()
{
    signal(SIGPIPE, SIG_IGN);

    //read data
    FILE* fp = fopen(READ_FILE_PATH, "r");
    if (!fp) {
        printf("fopen %s failed.\n", READ_FILE_PATH);
        fclose(fp);
        return -1;
    }

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
        process(client_socket, fp);
        close(client_socket);
    }

    fclose(fp);
    
    return 0;
}