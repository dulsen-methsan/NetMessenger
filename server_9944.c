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


/* Check whether a username is already registered */
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


/* Handle one connected client */
void *handle_client(void *arg)
{
    int client_index = *(int *)arg;

    free(arg);

    int client_fd = clients[client_index].socket_fd;

    char buffer[1024];

    printf("Client connected.\n");

    /*
     * Keep receiving commands while the
     * client connection remains active.
     */
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

        /* Remove newline character */
        buffer[strcspn(buffer, "\n")] = '\0';


        /*
         * REGISTER command
         */
        if (strncmp(buffer, "REGISTER ", 9) == 0)
        {
            char username[100];

            strncpy(username,
                    buffer + 9,
                    sizeof(username) - 1);

            username[sizeof(username) - 1] = '\0';

            printf("Register request received for username: %s\n",
                   username);

            char response[256];

            pthread_mutex_lock(&clients_mutex);

            /*
             * Check whether username already exists.
             */
            if (username_exists(username))
            {
                snprintf(response,
                         sizeof(response),
                         "ERR 001 USERNAME_TAKEN NID:%s\n",
                         NID);
            }
            else
            {
                /*
                 * Store the new username.
                 */
                strncpy(clients[client_index].username,
                        username,
                        sizeof(clients[client_index].username) - 1);

                clients[client_index].username[
                    sizeof(clients[client_index].username) - 1
                ] = '\0';

                clients[client_index].registered = 1;

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
     * Client disconnected.
     * Clean up its slot.
     */
    close(client_fd);

    pthread_mutex_lock(&clients_mutex);

    clients[client_index].socket_fd = -1;
    clients[client_index].registered = 0;
    clients[client_index].username[0] = '\0';

    pthread_mutex_unlock(&clients_mutex);

    return NULL;
}


/* Main server function */
int main(void)
{
    int server_fd;

    struct sockaddr_in server_addr;

    int i;


    /*
     * Initialize all client slots.
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
     * Bind socket to port 15944.
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
     * Start listening for clients.
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
     * Continuously accept new clients.
     */
    while (1)
    {
        struct sockaddr_in client_addr;

        socklen_t client_len =
            sizeof(client_addr);

        int client_fd;


        /*
         * Accept a client connection.
         */
        client_fd = accept(server_fd,
                           (struct sockaddr *)&client_addr,
                           &client_len);

        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }


        /*
         * Find an available client slot.
         */
        pthread_mutex_lock(&clients_mutex);

        int client_index =
            find_free_slot();

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
         * Store client socket.
         */
        clients[client_index].socket_fd =
            client_fd;

        clients[client_index].registered =
            0;

        pthread_mutex_unlock(&clients_mutex);


        /*
         * Allocate memory for the thread argument.
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
         * Create a separate thread
         * for the connected client.
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


        /*
         * Detach the thread so that
         * its resources are released
         * automatically when it finishes.
         */
        pthread_detach(thread);
    }


    close(server_fd);

    return 0;
}
