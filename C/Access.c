#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <pcap.h>
#include <pthread.h>
#include <signal.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>

#define BUFFER_SIZE 2048
#define FILE_NAME "server.log"

int running = 1;
pcap_t *handle;

void log_message(const char *packet, const char *source, int source_port, const char *dest, int dest_port) {
    FILE *file = fopen(FILE_NAME, "a");
    if (file == NULL) {
        perror("Couldn't open the log file!");
        return;
    }
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[100];
    strftime(time_str, sizeof(time_str) - 1, "%Y-%m-%d %H:%M:%S", t);

    fprintf(file, "[%s] %s:%d -> %s:%d - %s\n", time_str, source, source_port, dest, dest_port, packet);
    fclose(file);
}

void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
    struct ether_header *eth_header = (struct ether_header *) packet;
    if (ntohs(eth_header->ether_type) != ETHERTYPE_IP) {
        return; // Not an IP packet
    }

    const struct ip *ip_header = (struct ip*)(packet + sizeof(struct ether_header));
    if (ip_header->ip_p != IPPROTO_TCP) {
        return; // Not a TCP packet
    }

    int ip_header_length = ip_header->ip_hl * 4;
    struct tcphdr *tcp_header = (struct tcphdr*)(packet + sizeof(struct ether_header) + ip_header_length);

    char source_ip[INET_ADDRSTRLEN], dest_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ip_header->ip_src), source_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(ip_header->ip_dst), dest_ip, INET_ADDRSTRLEN);

    int source_port = ntohs(tcp_header->th_sport);
    int dest_port = ntohs(tcp_header->th_dport);

    // Payload starts after TCP header
    int tcp_header_length = tcp_header->th_off * 4;
    const u_char *payload = packet + sizeof(struct ether_header) + ip_header_length + tcp_header_length;
    int payload_len = pkthdr->len - (payload - packet);

    if (payload_len > 0) {
        char payload_str[BUFFER_SIZE] = {0};
        snprintf(payload_str, sizeof(payload_str), "%.*s", payload_len, payload);
        log_message(payload_str, source_ip, source_port, dest_ip, dest_port);
        printf("Captured: %s:%d -> %s:%d - %s\n", source_ip, source_port, dest_ip, dest_port, payload_str);
    }
}

void *capture_thread(void *arg) {
    char errbuf[PCAP_ERRBUF_SIZE];
    handle = pcap_open_live("lo", BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device lo: %s\n", errbuf);
        return NULL;
    }

    char filter_exp[] = "tcp";
    if (pcap_compile(handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return NULL;
    }
    if (pcap_setfilter(handle, &fp) == -1) {
        fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return NULL;
    }

    pcap_loop(handle, -1, packet_handler, NULL);
    return NULL;
}

void handle_signal(int signum) {
    running = 0;
    if (handle != NULL) {
        pcap_breakloop(handle);
    }
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

    if (getuid() != 0) {
        fprintf(stderr, "This program requires root privileges. Please run with sudo.\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_signal);

    pthread_t capture_thread_id;
    if (pthread_create(&capture_thread_id, NULL, capture_thread, NULL) != 0) {
        perror("Failed to create capture thread");
        exit(EXIT_FAILURE);
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

    printf("Logging server is running on port %d and capturing localhost traffic\n", port);

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (new_socket < 0) {
            if (running) perror("Accept failed");
            continue;
        }

        char buffer[BUFFER_SIZE] = {0};
        ssize_t bytes_read = read(new_socket, buffer, BUFFER_SIZE - 1);
        
        if (bytes_read > 0) {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            int client_port = ntohs(client_addr.sin_port);
            
            char server_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(address.sin_addr), server_ip, INET_ADDRSTRLEN);
            
            printf("Received from %s:%d - %s\n", client_ip, client_port, buffer);
            log_message(buffer, client_ip, client_port, server_ip, port);
        }

        close(new_socket);
    }

    pthread_join(capture_thread_id, NULL);
    close(server_fd);
    pcap_close(handle);
    return EXIT_SUCCESS;
}