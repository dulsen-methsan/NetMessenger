#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 15944
#define BACKLOG 10
#define MAX_CLIENTS 10
#define NID "6199"

typedef struct
{
    int socket_fd;
    char username[100];
    int registered;
} Client;

Client clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


/* Find an available client slot */
int find_free_slot(void)
{
    int i;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket_fd == -1)
        {
            return i;
        }
    }

    return -1;
}


/* Check whether username already exists */
int username_exists(const char *username)
{
    int i;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].registered &&
            strcmp(clients[i].username, username) == 0)
        {
            return 1;
        }
    }

    return 0;
}


/* Find client by username */
int find_client_by_username(const char *username)
{
    int i;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].registered &&
            strcmp(clients[i].username, username) == 0)
        {
            return i;
        }
    }

    return -1;
}


/* Send current registered users */
void send_user_list(int client_fd)
{
    char response[1024];

    int offset = 0;
    int first_user = 1;

    offset += snprintf(response + offset,
                       sizeof(response) - offset,
                       "OK USERS ");

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].registered)
        {
            if (!first_user)
            {
                offset += snprintf(response + offset,
                                   sizeof(response) - offset,
                                   ",");
            }

            offset += snprintf(response + offset,
                               sizeof(response) - offset,
                               "%s",
                               clients[i].username);

            first_user = 0;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    offset += snprintf(response + offset,
                       sizeof(response) - offset,
                       " NID:%s\n",
                       NID);

    send(client_fd,
         response,
         strlen(response),
         0);
}


/* Broadcast message to all other clients */
void broadcast_message(int sender_index,
                       const char *message)
{
    char response[1200];

    snprintf(response,
             sizeof(response),
             "MSG BCAST %s %s\n",
             clients[sender_index].username,
             message);

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (i != sender_index &&
            clients[i].registered &&
            clients[i].socket_fd != -1)
        {
            send(clients[i].socket_fd,
                 response,
                 strlen(response),
                 0);
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}


/* Send private message to one user */
int send_private_message(int sender_index,
                         const char *target_username,
                         const char *message)
{
    char response[1200];

    pthread_mutex_lock(&clients_mutex);

    int target_index =
        find_client_by_username(target_username);

    if (target_index == -1)
    {
        pthread_mutex_unlock(&clients_mutex);

        return -1;
    }

    /*
     * Create private message.
     *
     * NID is NOT included because MSG lines
     * forwarded to clients do not contain NID.
     */
    snprintf(response,
             sizeof(response),
             "MSG PRIV %s %s\n",
             clients[sender_index].username,
             message);

    send(clients[target_index].socket_fd,
         response,
         strlen(response),
         0);

    pthread_mutex_unlock(&clients_mutex);

    return 0;
}


/* Handle one connected client */
void *handle_client(void *arg)
{
    int client_index = *(int *)arg;

    free(arg);

    int client_fd =
        clients[client_index].socket_fd;

    char buffer[1024];

    int registered = 0;

    printf("Client connected.\n");


    while (1)
    {
        ssize_t bytes_received;

        bytes_received = recv(client_fd,
                              buffer,
                              sizeof(buffer) - 1,
                              0);

        if (bytes_received <= 0)
        {
            printf("Client disconnected.\n");
            break;
        }

        buffer[bytes_received] = '\0';

        /* Remove newline */
        buffer[strcspn(buffer, "\r\n")] = '\0';


        /*
         * REGISTER
         */
        if (strncmp(buffer, "REGISTER ", 9) == 0)
        {
            char username[100];

            strncpy(username,
                    buffer + 9,
                    sizeof(username) - 1);

            username[sizeof(username) - 1] = '\0';


            if (registered)
            {
                char response[256];

                snprintf(response,
                         sizeof(response),
                         "ERR 001 ALREADY_REGISTERED NID:%s\n",
                         NID);

                send(client_fd,
                     response,
                     strlen(response),
                     0);

                continue;
            }


            printf("Register request received for username: %s\n",
                   username);


            char response[256];

            pthread_mutex_lock(&clients_mutex);

            if (username_exists(username))
            {
                snprintf(response,
                         sizeof(response),
                         "ERR 001 USERNAME_TAKEN NID:%s\n",
                         NID);
            }
            else
            {
                strncpy(clients[client_index].username,
                        username,
                        sizeof(clients[client_index].username) - 1);

                clients[client_index].username[
                    sizeof(clients[client_index].username) - 1
                ] = '\0';

                clients[client_index].registered = 1;

                registered = 1;

                snprintf(response,
                         sizeof(response),
                         "OK REGISTERED %s NID:%s\n",
                         username,
                         NID);
            }

            pthread_mutex_unlock(&clients_mutex);

            send(client_fd,
                 response,
                 strlen(response),
                 0);
        }


        /*
         * LIST
         */
        else if (strcmp(buffer, "LIST") == 0)
        {
            if (!registered)
            {
                char response[256];

                snprintf(response,
                         sizeof(response),
                         "ERR 001 NOT_REGISTERED NID:%s\n",
                         NID);

                send(client_fd,
                     response,
                     strlen(response),
                     0);

                continue;
            }

            printf("LIST request received.\n");

            send_user_list(client_fd);
        }


        /*
         * BCAST
         */
        else if (strncmp(buffer, "BCAST ", 6) == 0)
        {
            if (!registered)
            {
                char response[256];

                snprintf(response,
                         sizeof(response),
                         "ERR 001 NOT_REGISTERED NID:%s\n",
                         NID);

                send(client_fd,
                     response,
                     strlen(response),
                     0);

                continue;
            }


            char *message = buffer + 6;


            if (strlen(message) == 0)
            {
                char response[256];

                snprintf(response,
                         sizeof(response),
                         "ERR 001 INVALID_COMMAND NID:%s\n",
                         NID);

                send(client_fd,
                     response,
                     strlen(response),
                     0);

                continue;
            }


            printf("Broadcast from %s: %s\n",
                   clients[client_index].username,
                   message);


            broadcast_message(client_index,
                              message);


            char response[256];

            snprintf(response,
                     sizeof(response),
                     "OK SENT NID:%s\n",
                     NID);

            send(client_fd,
                 response,
                 strlen(response),
                 0);
        }


        /*
         * PMSG
         *
         * Format:
         * PMSG <username> <message>
         */
        else if (strncmp(buffer, "PMSG ", 5) == 0)
        {
            if (!registered)
            {
                char response[256];

                snprintf(response,
                         sizeof(response),
                         "ERR 001 NOT_REGISTERED NID:%s\n",
                         NID);

                send(client_fd,
                     response,
                     strlen(response),
                     0);

                continue;
            }


            char command_copy[1024];

            strncpy(command_copy,
                    buffer + 5,
                    sizeof(command_copy) - 1);

            command_copy[sizeof(command_copy) - 1] = '\0';


            /*
             * Separate target username
             * from message.
             */
            char *space =
                strchr(command_copy, ' ');


            if (space == NULL)
            {
                char response[256];

                snprintf(response,
                         sizeof(response),
                         "ERR 001 INVALID_COMMAND NID:%s\n",
                         NID);

                send(client_fd,
                     response,
                     strlen(response),
                     0);

                continue;
            }


            *space = '\0';

            char *target_username =
                command_copy;

            char *message =
                space + 1;


            if (strlen(target_username) == 0 ||
                strlen(message) == 0)
            {
                char response[256];

                snprintf(response,
                         sizeof(response),
                         "ERR 001 INVALID_COMMAND NID:%s\n",
                         NID);

                send(client_fd,
                     response,
                     strlen(response),
                     0);

                continue;
            }


            printf("Private message from %s to %s: %s\n",
                   clients[client_index].username,
                   target_username,
                   message);


            /*
             * Send private message.
             */
            int result =
                send_private_message(client_index,
                                     target_username,
                                     message);


            if (result == -1)
            {
                char response[256];

                snprintf(response,
                         sizeof(response),
                         "ERR 002 USER_NOT_FOUND NID:%s\n",
                         NID);

                send(client_fd,
                     response,
                     strlen(response),
                     0);
            }
            else
            {
                char response[256];

                snprintf(response,
                         sizeof(response),
                         "OK SENT NID:%s\n",
                         NID);

                send(client_fd,
                     response,
                     strlen(response),
                     0);
            }
        }


        /*
         * QUIT
         */
        else if (strcmp(buffer, "QUIT") == 0)
        {
            printf("QUIT request received.\n");

            char response[256];

            snprintf(response,
                     sizeof(response),
                     "OK BYE NID:%s\n",
                     NID);

            send(client_fd,
                 response,
                 strlen(response),
                 0);

            break;
        }


        /*
         * Invalid command
         */
        else
        {
            char response[256];

            snprintf(response,
                     sizeof(response),
                     "ERR 001 INVALID_COMMAND NID:%s\n",
                     NID);

            send(client_fd,
                 response,
                 strlen(response),
                 0);
        }
    }


    /*
     * Clean up disconnected client.
     */
    close(client_fd);

    pthread_mutex_lock(&clients_mutex);

    clients[client_index].socket_fd = -1;
    clients[client_index].registered = 0;
    clients[client_index].username[0] = '\0';

    pthread_mutex_unlock(&clients_mutex);


    return NULL;
}


/* Main server */
int main(void)
{
    int server_fd;

    struct sockaddr_in server_addr;

    int i;


    /*
     * Initialize client slots.
     */
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].socket_fd = -1;
        clients[i].registered = 0;
        clients[i].username[0] = '\0';
    }


    /*
     * Create TCP socket.
     */
    server_fd = socket(AF_INET,
                       SOCK_STREAM,
                       0);

    if (server_fd < 0)
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

    server_addr.sin_addr.s_addr =
        INADDR_ANY;

    server_addr.sin_port =
        htons(PORT);


    /*
     * Bind socket.
     */
    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");

        close(server_fd);

        return 1;
    }


    /*
     * Listen.
     */
    if (listen(server_fd,
               BACKLOG) < 0)
    {
        perror("listen");

        close(server_fd);

        return 1;
    }


    printf("NetMessenger server started.\n");

    printf("Listening on TCP port %d...\n",
           PORT);


    /*
     * Accept clients continuously.
     */
    while (1)
    {
        struct sockaddr_in client_addr;

        socklen_t client_len =
            sizeof(client_addr);

        int client_fd;


        client_fd = accept(server_fd,
                           (struct sockaddr *)&client_addr,
                           &client_len);

        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }


        pthread_mutex_lock(&clients_mutex);

        int client_index =
            find_free_slot();


        /*
         * Server full.
         */
        if (client_index == -1)
        {
            pthread_mutex_unlock(&clients_mutex);

            char response[256];

            snprintf(response,
                     sizeof(response),
                     "ERR 005 SERVER_FULL NID:%s\n",
                     NID);

            send(client_fd,
                 response,
                 strlen(response),
                 0);

            close(client_fd);

            continue;
        }


        /*
         * Store client.
         */
        clients[client_index].socket_fd =
            client_fd;

        clients[client_index].registered =
            0;

        clients[client_index].username[0] =
            '\0';

        pthread_mutex_unlock(&clients_mutex);


        /*
         * Allocate thread argument.
         */
        int *index =
            malloc(sizeof(int));

        if (index == NULL)
        {
            perror("malloc");

            close(client_fd);

            pthread_mutex_lock(&clients_mutex);

            clients[client_index].socket_fd = -1;

            pthread_mutex_unlock(&clients_mutex);

            continue;
        }


        *index = client_index;


        /*
         * Create client thread.
         */
        pthread_t thread;

        if (pthread_create(&thread,
                           NULL,
                           handle_client,
                           index) != 0)
        {
            perror("pthread_create");

            free(index);

            close(client_fd);

            pthread_mutex_lock(&clients_mutex);

            clients[client_index].socket_fd = -1;

            pthread_mutex_unlock(&clients_mutex);

            continue;
        }


        pthread_detach(thread);
    }


    close(server_fd);

    return 0;
}
