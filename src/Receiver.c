/*
    Receiver
*/

#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <time.h> // for measuring time

// Flags
#include "config.h"

// functions declarations
int app_send(int sock, char *message, int messageLen);
int app_recv_part(int clientSocket, char *socketBuffer, long *averageTime);
int main();

// functions implementations
inline int app_send(int sock, char *message, int messageLen)
{
    int bytesSent = send(sock, message, messageLen, 0);

    if (-1 == bytesSent)
    {
        printf("ERROR: send() failed with error code: %d", errno);
        return -1;
    }
    else if (0 == bytesSent)
    {
        printf("ERROR: Peer has closed the TCP connection prior to send().\n");
        return -1;
    }
    else if (messageLen > bytesSent)
    {
        printf("ERROR: Sent only %d bytes from the required %d.\n", messageLen, bytesSent);
        return -1;
    }

    return 0;
}

int app_recv_part(int clientSocket, char *socketBuffer, long *recvesTime)
{
    int partSize = 0;

    clock_t startClock = clock();
    int recvBytesCount = recv(clientSocket, &partSize, sizeof(int), 0);

    if (recvBytesCount == -1)
    {
        printf("ERROR: recv() failed\n");
        return -1;
    }

    while (partSize >= SERVER_CHUNK_SIZE)
    {
        recvBytesCount = recv(clientSocket, socketBuffer, SERVER_CHUNK_SIZE, 0);

        if (recvBytesCount == -1)
        {
            printf("ERROR: recv() failed\n");
            return -1;
        }

        partSize -= recvBytesCount;
    }

    if (partSize > 0)
    {
        recvBytesCount = recv(clientSocket, socketBuffer, partSize, 0);

        if (recvBytesCount == -1)
        {
            printf("ERROR: recv() failed\n");
            return -1;
        }
    }

    clock_t endClock = clock();

    *recvesTime = (endClock - startClock);

    return 0;
}

int main()
{
    signal(SIGPIPE, SIG_IGN);

    // Open the listening (server) socket
    int listeningSocket = -1;

    if ((listeningSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("ERROR: Could not create listening socket: %d\n", errno);
    }

    // Reuse the address if the server socket on was closed
    // and remains for 45 seconds in TIME-WAIT state till the final removal.
    int enableReuse = 1;
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0)
    {
        printf("ERROR: setsockopt() failed with error code : %d\n", errno);
    }

    // bind all the server addresses to the listening socket
    struct sockaddr_in serverAddress;
    bzero(&serverAddress, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(SERVER_PORT);
    if (bind(listeningSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        printf("ERROR: Bind failed with error code: %d\n", errno);
        close(listeningSocket);
        return -1;
    }

    // start to listen for accepts
    if (listen(listeningSocket, MAXIMUM_CONNECTIONS) == -1)
    {
        printf("ERROR: listen() failed with error code: %d\n", errno);
        close(listeningSocket);
        return -1;
    }

    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);

    char socketBuffer[SERVER_CHUNK_SIZE + 1];
    int connectionWay = 1;

    while (1)
    {
        if (connectionWay == 2)
        {
            // Change the connection way to way A
            connectionWay = 1;
            printf("INFO: Change the connection way to way A\n");
        }

        // wating for client accept
        printf("INFO: Waiting for client accept\n");

        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddressLen = sizeof(clientAddress);
        int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (clientSocket == -1)
        {
            printf("ERROR: Listen failed with error code: %d\n", errno);
            close(clientSocket);
        }
        else
        {
            printf("INFO: A new client connection accepted\n");

            long recvesTimeA;
            long recvesTimeB;

            int loadNextFile = 1;
            while (loadNextFile == 1)
            {
                loadNextFile = 0;

                if (connectionWay == 2)
                {
                    // Change the connection way to way A
                    connectionWay = 1;
                    printf("INFO: Change the connection way to way A\n");
                }

                // recve the first part of the file
                int errorcode = app_recv_part(clientSocket, socketBuffer, &recvesTimeA);

                if (errorcode != -1)
                {
                    printf("INFO: The first hafe of the file was successfully recvied.\n");

                    // send the authentication
                    int idsXor = AUTHOR_A_ID_LAST_4 ^ AUTHOR_B_ID_LAST_4;
                    errorcode = app_send(clientSocket, (char *)&idsXor, sizeof(int));
                    SLEEP();
                }

                if (errorcode != -1)
                {
                    // Change the connection way to way B
                    connectionWay = 2;
                    printf("INFO: Change the connection way to way B\n");

                    // recve the second part of the file
                    errorcode = app_recv_part(clientSocket, socketBuffer, &recvesTimeB);
                }

                if (errorcode == -1)
                {
                    close(clientSocket);
                }
                else
                {
                    printf("INFO: Second hafe of the file was successfully recvied.\n");

                    // check if to exist
                    char hasNext;

                    int recvBytesCount = recv(clientSocket, &hasNext, sizeof(char), 0);

                    if (recvBytesCount == -1)
                    {
                        printf("ERROR: recv() failed\n");
                        close(clientSocket);
                    }
                    else
                    {
                        if (hasNext == 'Y')
                        {
                            loadNextFile = 1;
                        }
                        else
                        {
                            // Print out the times.
                            printf("First part take: %ld\n", recvesTimeA);
                            printf("Second part take: %ld\n", recvesTimeB);

                            close(clientSocket);
                            SLEEP();
                        }
                    }
                }
            }
        }
    }

    // close the listening socket - never
    close(listeningSocket);

    return 0;
}
