#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pwd.h>

#define BUFFER_SIZE 2048
#define FILE_NAME "server.log"

void log_message(const char *packet, const char *client_ip, int client_port) {
    FILE *file = fopen(FILE_NAME, "a");
    if (file == NULL) {
        perror("Couldn't open the log file!");
        return;
    }
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[100];
    strftime(time_str, sizeof(time_str) - 1, "%Y-%m-%d %H:%M:%S", t);

    fprintf(file, "[%s] %s:%d - %s\n", time_str, client_ip, client_port, packet);
    fclose(file);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <PORT_NUMBER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number. Please use a port between 1 and 65535.\n");
        exit(EXIT_FAILURE);
    }

    if (port < 1024) {
        uid_t uid = getuid();
        struct passwd *pw = getpwuid(uid);
        
        if (uid != 0) {
            fprintf(stderr, "You are running this program with user permissions of %s! Try running with sudo or as root to use ports below 1024 on Linux.\n", pw->pw_name);
            exit(EXIT_FAILURE);
        }
    }

    int server_fd;
    struct sockaddr_in address;
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Failed to listen");
        exit(EXIT_FAILURE);
    }

    printf("Logging server is running on port %d\n", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        char buffer[BUFFER_SIZE] = {0};
        ssize_t bytes_read = read(new_socket, buffer, BUFFER_SIZE - 1);
        
        if (bytes_read > 0) {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            int client_port = ntohs(client_addr.sin_port);
            
            printf("Received from %s:%d - %s\n", client_ip, client_port, buffer);
            log_message(buffer, client_ip, client_port);
        }

        close(new_socket);
    }

    close(server_fd);
    return EXIT_SUCCESS;
}