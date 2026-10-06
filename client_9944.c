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


    /* Create socket */
    client_fd =
        socket(AF_INET,
               SOCK_STREAM,
               0);

    if (client_fd < 0)
    {
        perror("socket");
        return 1;
    }


    /* Configure server */
    memset(&server_addr,
           0,
           sizeof(server_addr));

    server_addr.sin_family =
        AF_INET;

    server_addr.sin_port =
        htons(PORT);


    /* Convert IP address */
    if (inet_pton(AF_INET,
                  SERVER_IP,
                  &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");

        close(client_fd);

        return 1;
    }


    /* Connect */
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


    /* Get username */
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


    /* REGISTER */
    char message[256];

    snprintf(message,
             sizeof(message),
             "REGISTER %s\n",
             username);


    if (send(client_fd,
             message,
             strlen(message),
             0) < 0)
    {
        perror("send");

        close(client_fd);

        return 1;
    }


    /* Registration response */
    char buffer[BUFFER_SIZE];

    ssize_t bytes_received =
        recv(client_fd,
             buffer,
             sizeof(buffer) - 1,
             0);


    if (bytes_received <= 0)
    {
        printf("Server disconnected.\n");

        close(client_fd);

        return 1;
    }


    buffer[bytes_received] =
        '\0';


    printf("Server response: %s",
           buffer);


    if (strncmp(buffer,
                "OK REGISTERED",
                13) != 0)
    {
        close(client_fd);

        return 1;
    }


    printf("\n");
    printf("Connection is active.\n");

    printf("Available commands:\n");
    printf("  LIST\n");
    printf("  BCAST <message>\n");
    printf("  PMSG <username> <message>\n");
    printf("  JOIN <room>\n");
    printf("  LEAVE <room>\n");
    printf("  ROOMS\n");
    printf("  RMSG <room> <message>\n");
    printf("  QUIT\n");
    printf("\n");

    printf("NetMessenger> ");

    fflush(stdout);


    /* Interactive loop */
    while (1)
    {
        fd_set read_fds;

        FD_ZERO(&read_fds);

        FD_SET(STDIN_FILENO,
               &read_fds);

        FD_SET(client_fd,
               &read_fds);


        int max_fd =
            client_fd > STDIN_FILENO
            ? client_fd
            : STDIN_FILENO;


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


        /* Server message */
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


            buffer[bytes_received] =
                '\0';


            printf("\nServer: %s",
                   buffer);

            printf("NetMessenger> ");

            fflush(stdout);
        }


        /* User command */
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


            if (strlen(command) == 0)
            {
                printf("NetMessenger> ");

                fflush(stdout);

                continue;
            }


            /* QUIT */
            if (strcmp(command,
                       "QUIT") == 0)
            {
                strcat(command,
                       "\n");


                if (send(client_fd,
                         command,
                         strlen(command),
                         0) < 0)
                {
                    perror("send");
                    break;
                }


                bytes_received =
                    recv(client_fd,
                         buffer,
                         sizeof(buffer) - 1,
                         0);


                if (bytes_received > 0)
                {
                    buffer[bytes_received] =
                        '\0';

                    printf("Server response: %s",
                           buffer);
                }

                break;
            }


            /* Send command */
            strcat(command,
                   "\n");


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
