#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define SERVER_IP "127.0.0.1"
#define PORT 15944
#define BUFFER_SIZE 1024


int main(void)
{
    int client_fd;

    struct sockaddr_in server_addr;


    /*
     * Create TCP socket.
     */
    client_fd = socket(AF_INET,
                       SOCK_STREAM,
                       0);

    if (client_fd < 0)
    {
        perror("socket");
        return 1;
    }


    /*
     * Configure server address.
     */
    memset(&server_addr,
           0,
           sizeof(server_addr));

    server_addr.sin_family = AF_INET;

    server_addr.sin_port =
        htons(PORT);


    /*
     * Convert server IP.
     */
    if (inet_pton(AF_INET,
                  SERVER_IP,
                  &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");

        close(client_fd);

        return 1;
    }


    /*
     * Connect to server.
     */
    if (connect(client_fd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect");

        close(client_fd);

        return 1;
    }


    printf("Connected to NetMessenger server.\n");

    printf("Server: %s:%d\n",
           SERVER_IP,
           PORT);


    /*
     * Ask for username.
     */
    char username[100];

    printf("Enter username: ");

    fflush(stdout);


    if (fgets(username,
              sizeof(username),
              stdin) == NULL)
    {
        close(client_fd);

        return 1;
    }


    username[strcspn(username,
                     "\r\n")] = '\0';


    /*
     * Create REGISTER command.
     */
    char message[256];

    snprintf(message,
             sizeof(message),
             "REGISTER %s\n",
             username);


    /*
     * Send REGISTER.
     */
    if (send(client_fd,
             message,
             strlen(message),
             0) < 0)
    {
        perror("send");

        close(client_fd);

        return 1;
    }


    /*
     * Receive registration response.
     */
    char buffer[BUFFER_SIZE];

    ssize_t bytes_received;


    bytes_received = recv(client_fd,
                          buffer,
                          sizeof(buffer) - 1,
                          0);


    if (bytes_received <= 0)
    {
        printf("Server disconnected.\n");

        close(client_fd);

        return 1;
    }


    buffer[bytes_received] = '\0';


    printf("Server response: %s",
           buffer);


    /*
     * Registration failed.
     */
    if (strncmp(buffer,
                "OK REGISTERED",
                13) != 0)
    {
        close(client_fd);

        return 1;
    }


    printf("\n");
    printf("Connection is active.\n");
    printf("Available commands: LIST, BCAST <message>, QUIT\n");
    printf("\n");


    /*
     * Interactive client loop.
     *
     * select() allows the client to:
     *
     * 1. Read commands from keyboard.
     * 2. Receive messages from server.
     *
     * at the same time.
     */
    while (1)
    {
        fd_set read_fds;

        int max_fd;


        FD_ZERO(&read_fds);

        FD_SET(STDIN_FILENO,
               &read_fds);

        FD_SET(client_fd,
               &read_fds);


        if (client_fd > STDIN_FILENO)
        {
            max_fd = client_fd;
        }
        else
        {
            max_fd = STDIN_FILENO;
        }


        /*
         * Wait for keyboard input
         * or server data.
         */
        int result =
            select(max_fd + 1,
                   &read_fds,
                   NULL,
                   NULL,
                   NULL);


        if (result < 0)
        {
            perror("select");
            break;
        }


        /*
         * Server sent something.
         */
        if (FD_ISSET(client_fd,
                     &read_fds))
        {
            bytes_received =
                recv(client_fd,
                     buffer,
                     sizeof(buffer) - 1,
                     0);


            if (bytes_received <= 0)
            {
                printf("\nServer disconnected.\n");
                break;
            }


            buffer[bytes_received] = '\0';

            printf("\nServer: %s",
                   buffer);

            printf("NetMessenger> ");

            fflush(stdout);
        }


        /*
         * User typed something.
         */
        if (FD_ISSET(STDIN_FILENO,
                     &read_fds))
        {
            char command[BUFFER_SIZE];


            if (fgets(command,
                      sizeof(command),
                      stdin) == NULL)
            {
                break;
            }


            command[strcspn(command,
                            "\r\n")] = '\0';


            /*
             * Ignore empty command.
             */
            if (strlen(command) == 0)
            {
                printf("NetMessenger> ");

                fflush(stdout);

                continue;
            }


            /*
             * QUIT.
             */
            if (strcmp(command,
                       "QUIT") == 0)
            {
                strcat(command, "\n");


                if (send(client_fd,
                         command,
                         strlen(command),
                         0) < 0)
                {
                    perror("send");
                    break;
                }


                /*
                 * Receive OK BYE.
                 */
                bytes_received =
                    recv(client_fd,
                         buffer,
                         sizeof(buffer) - 1,
                         0);


                if (bytes_received > 0)
                {
                    buffer[bytes_received] = '\0';

                    printf("Server response: %s",
                           buffer);
                }


                break;
            }


            /*
             * Send normal command.
             */
            strcat(command, "\n");


            if (send(client_fd,
                     command,
                     strlen(command),
                     0) < 0)
            {
                perror("send");
                break;
            }
        }
    }


    close(client_fd);

    printf("Disconnected from server.\n");


    return 0;
}
