/**
 * Sender entry point
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
#include <netinet/tcp.h>

#include "config.h"

// functions declarations
int load_input_file(char **partA, int *partASize, char **partB, int *partBSize);
int connect_to_server();
int app_send(int sock, char *message, int messageLen);
int main();

// functions implementations
/**
 * Load file into parts when the first bytes of each parts saved the size of the part
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

    // get the size of the file
    fseek(file, 0, SEEK_END);
    int fileSize = (int)ftell(file);
    fseek(file, 0, SEEK_SET);

    // set the parts size's
    *partASize = fileSize / 2;
    *partBSize = fileSize - *partASize;

    // alocate memory for the parts, buffer structure:
    // 4 bytes - size of the part
    // partXSize bytes - the part content
    *partA = malloc(sizeof(int) + (*partASize) + 1);
    *partB = malloc(sizeof(int) + (*partBSize) + 1);

    // set the size of the parts in the start of the array
    memcpy(*partA, partASize, sizeof(int));
    memcpy(*partB, partBSize, sizeof(int));

    // read the file
    fread((*partA) + sizeof(int), 1, *partASize, file);
    fread((*partB) + sizeof(int), 1, *partBSize, file);

    // close the file
    fclose(file);

    return 0;
}

/**
 * Connecting to the server
 *
 * @param sockaddr_in* serverAddress address of the server
 * @return the socket
 */
inline int connect_to_server(struct sockaddr_in *serverAddress)
{
    // create a client socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1)
    {
        printf("ERROR: Could not create socket: %d\n", errno);
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

    // connect to the server
    if (connect(sock, (struct sockaddr *)serverAddress, sizeof(*serverAddress)) == -1)
    {
        printf("ERROR: connect() failed with error code: %d\n", errno);
        return -1;
    }

    return sock;
}

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

/**
 * entry point of the application
 */
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

    // Connenting to the server
    struct sockaddr_in serverAddress;
    int sock = connect_to_server(&serverAddress);

    if (sock == -1)
    {
        return -1;
    }

    printf("INFO: connected to server\n");

    SLEEP();

    int action = 1; // 1 - run, 0 - close

    // the main loop
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

        // Change congestion control to "reno"
        printf("INFO: Change congestion control to reno\n");
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "reno", 5) != 0)
        {
            printf("ERROR: setsockopt() failed with error code: %d\n", errno);
            return -1;
        }

        // Send second hafe of the file
        if (app_send(sock, partB, partBSize) == -1)
        {
            return -1;
        }

        SLEEP();

        printf("INFO: Second hafe of the file was successfully sent.\n");

        // wait until the server complit the loading
        char temp;
        if (recv(sock, &temp, 1, 0) == -1)
        {
            return -1;
        }

        // ask the user if to send the file again
        printf("Send the file again [yes/no]? ");

        char line[1024];
        scanf("%s", line);

        action = (line[0] == 'y') ? 1 : 0;

        // send the anser to the Receiver
        if (app_send(sock, (action == 1) ? "Y" : "N", 1) == -1)
        {
            return -1;
        }

        if (action == 1)
        {
            // Change congestion control to "cubic"
            printf("INFO: Change congestion control to cubic\n");
            if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "cubic", 5) != 0)
            {
                printf("ERROR: setsockopt() failed with error code: %d\n", errno);
                return -1;
            }
        }
    }

    // close the socket
    close(sock);

    // cleanup
    free(partA);
    free(partB);

    return 0;
}
