#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

#define MESSAGE_BUF_SIZE 1000

static int sockfd;

int main( int argc, char *agv[] ) {
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    char serverResponse[MESSAGE_BUF_SIZE];

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(9002);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        fprintf(stderr, "Failed to connect to server.\n");
        return -1;
    }
    
    if (recv(sockfd, &serverResponse, sizeof(serverResponse), 0) < 0) {
        fprintf(stderr, "Didn't reciever anything from the server\n");
        return -1;
    }
    
    printf("Test client received mode from server : %s\n", serverResponse);

    close(sockfd);
    return 0;
}
