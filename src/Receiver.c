/**
 * Receiver entry point
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
#include <netinet/tcp.h>

#include <sys/time.h> // for measuring time

#include "config.h"

// functions declarations
int app_send(int sock, char *message, int messageLen);
int app_recv_part(int clientSocket, char *socketBuffer, long *averageTime);
int main();

// functions implementations
/**
 * Send message over socket
 *
 * @param int sock socket
 * @param char* message to send
 * @param int messageLen length of the message
 * @return 0 if sended -1 if the sending failed of not all the message sent
 */
inline int app_send(int sock, char *message, int messageLen)
{
    // sending the message
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

/**
 * receive part (hafe) of file from the client
 *
 * @param int clientSocket client socket
 * @param char* socketBuffer buffer to save the part
 * @param int* recvesTime the time that take to get all the file
 * @return 0 if recv the part, -1 - elsewhere
 */
int app_recv_part(int clientSocket, char *socketBuffer, suseconds_t *recvesTime)
{
    int partSize = 0;

    // start to measure
    struct timeval startClock;
    gettimeofday(&startClock, NULL);

    // receive the size of the part
    int recvBytesCount = recv(clientSocket, &partSize, sizeof(int), 0);

    if (recvBytesCount == -1)
    {
        printf("ERROR: recv() failed\n");
        return -1;
    }

    // receive chuncks
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

    // receive the rest of the part
    if (partSize > 0)
    {
        recvBytesCount = recv(clientSocket, socketBuffer, partSize, 0);

        if (recvBytesCount == -1)
        {
            printf("ERROR: recv() failed\n");
            return -1;
        }
    }

    // end the measure
    struct timeval endClock;
    gettimeofday(&endClock, NULL);

    *recvesTime = (endClock.tv_usec - startClock.tv_usec);

    return 0;
}

/**
 * entry point of the application
 */
int main()
{
    signal(SIGPIPE, SIG_IGN);

    // create listening socket
    int listeningSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (listeningSocket == -1)
    {
        printf("ERROR: Could not create listening socket: %d\n", errno);
        return -1;
    }

    // Reuse the address if the server socket on was closed
    // and remains for 45 seconds in TIME-WAIT state till the final removal.
    int enableReuse = 1;
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int)) < 0)
    {
        printf("ERROR: setsockopt() failed with error code: %d\n", errno);
        close(listeningSocket);
        return -1;
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

    // the main loop
    while (1)
    {
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
            // handel client requets
            printf("INFO: A new client connection accepted\n");

            suseconds_t recvesTimeOfAllFiles = 0;
            int recvesPartsCount = 0;

            suseconds_t recvesTime;

            int loadNextFile = 1;
            while (loadNextFile == 1)
            {
                loadNextFile = 0;

                // recve the first part of the file
                int errorcode = app_recv_part(clientSocket, socketBuffer, &recvesTime);
                recvesTimeOfAllFiles += recvesTime;
                recvesPartsCount++;

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
                    // Change congestion control to "reno"
                    printf("INFO: Change the connection way to way reno\n");
                    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_CONGESTION, "reno", 5) != 0)
                    {
                        printf("ERROR: setsockopt() failed with error code: %d\n", errno);
                        errorcode = -1;
                    }
                }

                if (errorcode != -1)
                {
                    // recve the second part of the file
                    errorcode = app_recv_part(clientSocket, socketBuffer, &recvesTime);
                    recvesTimeOfAllFiles += recvesTime;
                    recvesPartsCount++;
                }

                if (errorcode != -1)
                {
                    // known the sender that the filed loaded.
                    errorcode = app_send(clientSocket, "W", 1);
                    SLEEP();
                }

                if (errorcode == -1)
                {
                    close(clientSocket);
                }
                else
                {
                    printf("INFO: Second hafe of the file was successfully recvied.\n");

                    // check if the client finished
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
                            // Change congestion control to "cubic"
                            printf("INFO: Change the connection way to way cubic\n");
                            if (setsockopt(clientSocket, IPPROTO_TCP, TCP_CONGESTION, "cubic", 5) != 0)
                            {
                                printf("ERROR: setsockopt() failed with error code: %d\n", errno);
                                close(clientSocket);
                            }
                            else
                            {
                                loadNextFile = 1;
                            }
                        }
                        else
                        {
                            printf("Recves %d parts, in %ld [ms]\n", recvesPartsCount, recvesTimeOfAllFiles);
                            printf("Recves Average Time of the Files: %ld [ms]\n", recvesTimeOfAllFiles / recvesPartsCount);

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
