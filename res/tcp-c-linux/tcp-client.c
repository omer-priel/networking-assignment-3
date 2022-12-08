/*
    TCP/IP client
*/

#include <stdio.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 5060
#define SERVER_IP_ADDRESS "127.0.0.1"

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1)
    {
        printf("Could not create socket : %d", errno);
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    //
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    int rval = inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &serverAddress.sin_addr);
    if (rval <= 0)
    {
        printf("inet_pton() failed");
        return -1;
    }

    // Make a connection to the server with socket SendingSocket.

    if (connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        printf("connect() failed with error code : %d", errno);
    }

    printf("connected to server\n");

    // Sends some data to server
    char message[] = "Good morning, Vietnam\n";
    int messageLen = strlen(message) + 1;

    int bytesSent = send(sock, message, messageLen, 0);

    if (-1 == bytesSent)
    {
        printf("send() failed with error code : %d", errno);
    }
    else if (0 == bytesSent)
    {
        printf("peer has closed the TCP connection prior to send().\n");
    }
    else if (messageLen > bytesSent)
    {
        printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
    }
    else
    {
        printf("message was successfully sent .\n");
    }

    sleep(3);

    // TODO: All open clientSocket descriptors should be kept
    // in some container and closed as well.
    close(sock);

    return 0;
}
