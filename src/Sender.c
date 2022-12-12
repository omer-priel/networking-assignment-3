/*
    Sender
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

#include "config.h"

// functions declarations
int load_input_file(char **partA, int *partASize, char **partB, int *partBSize);
int connect_to_server();
int app_send(int sock, char *message, int messageLen);
int main();

// functions implementations
/**
 * Load file into to parts when the first bytes of each parts saved the size of the part
 */
int load_input_file(char **partA, int *partASize, char **partB, int *partBSize)
{
    FILE *file;

    // Opening file in reading mode
    file = fopen(INPUT_FILE_PATH, "r");

    if (NULL == file)
    {
        printf("ERROR: File can't be opened\n");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    int fileSize = (int)ftell(file);
    fseek(file, 0, SEEK_SET);

    *partASize = fileSize / 2;
    *partBSize = fileSize - *partASize;

    *partA = malloc(sizeof(int) + (*partASize) + 1);
    *partB = malloc(sizeof(int) + (*partBSize) + 1);

    memcpy(*partA, partASize, sizeof(int));
    memcpy(*partB, partBSize, sizeof(int));

    fread((*partA) + sizeof(int), 1, *partASize, file);
    fread((*partB) + sizeof(int), 1, *partBSize, file);

    // close the file
    fclose(file);

    return 0;
}

inline int connect_to_server(struct sockaddr_in *serverAddress)
{
    // create a client socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1)
    {
        printf("ERROR: Could not create socket : %d\n", errno);
        return -1;
    }

    // config and save the server address
    bzero(serverAddress, sizeof(*serverAddress));

    serverAddress->sin_family = AF_INET;
    serverAddress->sin_port = htons(SERVER_PORT);
    int rval = inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &serverAddress->sin_addr);
    if (rval <= 0)
    {
        printf("ERROR: inet_pton() failed\n");
        return -1;
    }

    // conent to the server
    if (connect(sock, (struct sockaddr *)serverAddress, sizeof(*serverAddress)) == -1)
    {
        printf("ERROR: connect() failed with error code: %d\n", errno);
        return -1;
    }

    printf("connected to server\n");

    return sock;
}

inline int app_send(int sock, char *message, int messageLen)
{
    int bytesSent = send(sock, message, messageLen, 0);

    if (-1 == bytesSent)
    {
        printf("ERROR: send() failed with error code: %d\n", errno);
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

int main()
{
    // Load the file
    char *partA;
    int partASize;

    char *partB;
    int partBSize;

    if (load_input_file(&partA, &partASize, &partB, &partBSize) == -1)
    {
        return -1;
    }

    printf("DEBUG: A %d\n", 0);

    // Connenting to the server
    struct sockaddr_in serverAddress;
    int sock = connect_to_server(&serverAddress);

    if (sock == -1)
    {
        return -1;
    }

    printf("INFO: connected to server\n");

    SLEEP();

    int action = 1;
    while (action == 1)
    {
        // Send first hafe of the files
        if (app_send(sock, partA, partASize) == -1)
        {
            return -1;
        }

        printf("INFO: First hafe of the file was successfully sent.\n");

        SLEEP();

        // recive the authentication
        int authentication = 0;
        if (recv(sock, (char *)&authentication, sizeof(int), 0) == -1)
        {
            return -1;
        }

        // print the authentication
        printf("The authentication key is %d\n", authentication);

        // Change the connection way to way B
        printf("INFO: Change the connection way to way B\n");

        // Send second hafe of the file
        if (app_send(sock, partB, partBSize) == -1)
        {
            return -1;
        }

        SLEEP();

        printf("INFO: Second hafe of the file was successfully sent.\n");

        // ask the user if to send the file again
        printf("Send the file again [yes/no]? ");

        char line[1024];
        scanf("%s", line);

        action = (line[0] == 'y') ? 1 : 0;

        if (action == 1)
        {
            if (app_send(sock, "Y", 1) == -1)
            {
                return -1;
            }

            SLEEP();

            // Change the connection way to way A
            printf("INFO: Change the connection way to way A\n");
        }
        else
        {
            if (app_send(sock, "N", 1) == -1)
            {
                return -1;
            }
        }
    }

    // close the socket
    close(sock);

    return 0;
}
