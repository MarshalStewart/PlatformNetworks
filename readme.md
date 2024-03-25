TODO: Compile with CLANG just for fun

# Example code for a ipv6 socket. need to update the TCP APIs to be split for ipv4 and ipv6
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9003
#define BUFFER_SIZE 1024

int main() {
    int sockfd, newsockfd;
    struct sockaddr_in6 serv_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];

    // Create a TCP IPv6 socket
    sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_addr = in6addr_any;
    serv_addr.sin6_port = htons(PORT);

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(sockfd, 5) < 0) {
        perror("listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    // Accept incoming connection
    client_len = sizeof(client_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &client_len);
    if (newsockfd < 0) {
        perror("accept");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Handle client connection
    printf("Client connected.\n");
    memset(buffer, 0, sizeof(buffer));
    ssize_t num_bytes = read(newsockfd, buffer, sizeof(buffer) - 1);
    if (num_bytes < 0) {
        perror("read");
        close(newsockfd);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Received message from client: %s\n", buffer);

    // Send response to the client
    const char *response = "Hello, client!";
    if (write(newsockfd, response, strlen(response)) < 0) {
        perror("write");
        close(newsockfd);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Close sockets
    close(newsockfd);
    close(sockfd);

    return 0;
}
