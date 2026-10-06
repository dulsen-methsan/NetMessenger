#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 15944
#define BACKLOG 10
#define MAX_CLIENTS 10
#define MAX_ROOMS 20
#define ROOM_NAME_LEN 100
#define NID "6199"

typedef struct
{
    int socket_fd;
    char username[100];
    int registered;
} Client;

typedef struct
{
    char name[ROOM_NAME_LEN];
    int members[MAX_CLIENTS];
    int member_count;
    int active;
} Room;

Client clients[MAX_CLIENTS];
Room rooms[MAX_ROOMS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


/* Send formatted response */
void send_response(int fd, const char *format, ...)
{
    char response[2048];
    va_list args;

    va_start(args, format);
    vsnprintf(response, sizeof(response), format, args);
    va_end(args);

    send(fd, response, strlen(response), 0);
}


/* Find free client slot */
int find_free_slot(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket_fd == -1)
        {
            return i;
        }
    }

    return -1;
}


/* Check whether username exists */
int username_exists(const char *username)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
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
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].registered &&
            strcmp(clients[i].username, username) == 0)
        {
            return i;
        }
    }

    return -1;
}


/* Send current user list */
void send_user_list(int client_fd)
{
    char response[1024];

    int offset =
        snprintf(response,
                 sizeof(response),
                 "OK USERS ");

    int first_user = 1;

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].registered)
        {
            offset +=
                snprintf(response + offset,
                         sizeof(response) - offset,
                         "%s%s",
                         first_user ? "" : ",",
                         clients[i].username);

            first_user = 0;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    snprintf(response + offset,
             sizeof(response) - offset,
             " NID:%s\n",
             NID);

    send(client_fd,
         response,
         strlen(response),
         0);
}


/* Broadcast message */
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


/* Send private message */
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


/* Find room */
int find_room(const char *room_name)
{
    for (int i = 0; i < MAX_ROOMS; i++)
    {
        if (rooms[i].active &&
            strcmp(rooms[i].name, room_name) == 0)
        {
            return i;
        }
    }

    return -1;
}


/* Find free room */
int find_free_room(void)
{
    for (int i = 0; i < MAX_ROOMS; i++)
    {
        if (!rooms[i].active)
        {
            return i;
        }
    }

    return -1;
}


/* Check room membership */
int is_room_member(int room_index,
                   int client_index)
{
    if (room_index < 0 ||
        !rooms[room_index].active)
    {
        return 0;
    }

    for (int i = 0;
         i < rooms[room_index].member_count;
         i++)
    {
        if (rooms[room_index].members[i] ==
            client_index)
        {
            return 1;
        }
    }

    return 0;
}


/* Join or create room */
int join_room(int client_index,
              const char *room_name)
{
    pthread_mutex_lock(&clients_mutex);

    int room_index =
        find_room(room_name);

    if (room_index == -1)
    {
        room_index =
            find_free_room();

        if (room_index == -1)
        {
            pthread_mutex_unlock(&clients_mutex);
            return -2;
        }

        rooms[room_index].active = 1;

        strncpy(rooms[room_index].name,
                room_name,
                ROOM_NAME_LEN - 1);

        rooms[room_index]
            .name[ROOM_NAME_LEN - 1] = '\0';

        rooms[room_index].member_count = 0;
    }

    if (is_room_member(room_index,
                       client_index))
    {
        pthread_mutex_unlock(&clients_mutex);
        return 0;
    }

    if (rooms[room_index].member_count >=
        MAX_CLIENTS)
    {
        pthread_mutex_unlock(&clients_mutex);
        return -3;
    }

    rooms[room_index]
        .members[rooms[room_index].member_count++] =
        client_index;

    pthread_mutex_unlock(&clients_mutex);

    return 1;
}


/* Leave room */
int leave_room(int client_index,
               const char *room_name)
{
    pthread_mutex_lock(&clients_mutex);

    int room_index =
        find_room(room_name);

    if (room_index == -1)
    {
        pthread_mutex_unlock(&clients_mutex);
        return -1;
    }

    for (int i = 0;
         i < rooms[room_index].member_count;
         i++)
    {
        if (rooms[room_index].members[i] ==
            client_index)
        {
            for (int j = i;
                 j < rooms[room_index].member_count - 1;
                 j++)
            {
                rooms[room_index].members[j] =
                    rooms[room_index].members[j + 1];
            }

            rooms[room_index].member_count--;

            pthread_mutex_unlock(&clients_mutex);

            return 1;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    return 0;
}


/* Send room list */
void send_room_list(int client_fd)
{
    char response[2048];

    int offset =
        snprintf(response,
                 sizeof(response),
                 "OK ROOMS ");

    int first_room = 1;

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_ROOMS; i++)
    {
        if (rooms[i].active)
        {
            offset +=
                snprintf(response + offset,
                         sizeof(response) - offset,
                         "%s%s",
                         first_room ? "" : ",",
                         rooms[i].name);

            first_room = 0;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    snprintf(response + offset,
             sizeof(response) - offset,
             " NID:%s\n",
             NID);

    send(client_fd,
         response,
         strlen(response),
         0);
}


/* Send room message */
int send_room_message(int sender_index,
                      const char *room_name,
                      const char *message)
{
    char response[1200];

    pthread_mutex_lock(&clients_mutex);

    int room_index =
        find_room(room_name);

    if (room_index == -1)
    {
        pthread_mutex_unlock(&clients_mutex);
        return -1;
    }

    if (!is_room_member(room_index,
                        sender_index))
    {
        pthread_mutex_unlock(&clients_mutex);
        return -2;
    }

    snprintf(response,
             sizeof(response),
             "MSG ROOM %s %s %s\n",
             room_name,
             clients[sender_index].username,
             message);

    for (int i = 0;
         i < rooms[room_index].member_count;
         i++)
    {
        int member_index =
            rooms[room_index].members[i];

        if (member_index != sender_index &&
            clients[member_index].registered &&
            clients[member_index].socket_fd != -1)
        {
            send(clients[member_index].socket_fd,
                 response,
                 strlen(response),
                 0);
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    return 0;
}


/* Remove client from all rooms */
void remove_client_from_all_rooms(int client_index)
{
    pthread_mutex_lock(&clients_mutex);

    for (int room = 0;
         room < MAX_ROOMS;
         room++)
    {
        if (!rooms[room].active)
        {
            continue;
        }

        for (int i = 0;
             i < rooms[room].member_count;
             i++)
        {
            if (rooms[room].members[i] ==
                client_index)
            {
                for (int j = i;
                     j < rooms[room].member_count - 1;
                     j++)
                {
                    rooms[room].members[j] =
                        rooms[room].members[j + 1];
                }

                rooms[room].member_count--;

                i--;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}


/* Handle client */
void *handle_client(void *arg)
{
    int client_index =
        *(int *)arg;

    free(arg);

    int client_fd =
        clients[client_index].socket_fd;

    int registered = 0;

    char buffer[1024];

    printf("Client connected.\n");

    while (1)
    {
        ssize_t bytes_received =
            recv(client_fd,
                 buffer,
                 sizeof(buffer) - 1,
                 0);

        if (bytes_received <= 0)
        {
            printf("Client disconnected.\n");
            break;
        }

        buffer[bytes_received] = '\0';

        buffer[strcspn(buffer,
                       "\r\n")] = '\0';


        /* REGISTER */
        if (strncmp(buffer,
                    "REGISTER ",
                    9) == 0)
        {
            char username[100];

            strncpy(username,
                    buffer + 9,
                    sizeof(username) - 1);

            username[sizeof(username) - 1] =
                '\0';

            char response[256];

            if (registered)
            {
                snprintf(response,
                         sizeof(response),
                         "ERR 001 ALREADY_REGISTERED NID:%s\n",
                         NID);
            }
            else
            {
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

                    clients[client_index]
                        .username[sizeof(clients[client_index].username) - 1] =
                        '\0';

                    clients[client_index].registered =
                        1;

                    registered = 1;

                    snprintf(response,
                             sizeof(response),
                             "OK REGISTERED %s NID:%s\n",
                             username,
                             NID);
                }

                pthread_mutex_unlock(&clients_mutex);
            }

            send(client_fd,
                 response,
                 strlen(response),
                 0);
        }


        /* LIST */
        else if (strcmp(buffer, "LIST") == 0)
        {
            if (!registered)
            {
                send_response(client_fd,
                              "ERR 001 NOT_REGISTERED NID:%s\n",
                              NID);
                continue;
            }

            send_user_list(client_fd);
        }


        /* BCAST */
        else if (strncmp(buffer,
                         "BCAST ",
                         6) == 0)
        {
            if (!registered)
            {
                send_response(client_fd,
                              "ERR 001 NOT_REGISTERED NID:%s\n",
                              NID);
                continue;
            }

            char *message =
                buffer + 6;

            if (*message == '\0')
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND NID:%s\n",
                              NID);
                continue;
            }

            broadcast_message(client_index,
                              message);

            send_response(client_fd,
                          "OK SENT NID:%s\n",
                          NID);
        }


        /* PMSG */
        else if (strncmp(buffer,
                         "PMSG ",
                         5) == 0)
        {
            if (!registered)
            {
                send_response(client_fd,
                              "ERR 001 NOT_REGISTERED NID:%s\n",
                              NID);
                continue;
            }

            char command_copy[1024];

            strncpy(command_copy,
                    buffer + 5,
                    sizeof(command_copy) - 1);

            command_copy[sizeof(command_copy) - 1] =
                '\0';

            char *space =
                strchr(command_copy, ' ');

            if (space == NULL)
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND NID:%s\n",
                              NID);
                continue;
            }

            *space = '\0';

            char *target_username =
                command_copy;

            char *message =
                space + 1;

            if (*target_username == '\0' ||
                *message == '\0')
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND NID:%s\n",
                              NID);
                continue;
            }

            int result =
                send_private_message(client_index,
                                      target_username,
                                      message);

            if (result == -1)
            {
                send_response(client_fd,
                              "ERR 002 USER_NOT_FOUND NID:%s\n",
                              NID);
            }
            else
            {
                send_response(client_fd,
                              "OK SENT NID:%s\n",
                              NID);
            }
        }


        /* JOIN */
        else if (strncmp(buffer,
                         "JOIN ",
                         5) == 0)
        {
            if (!registered)
            {
                send_response(client_fd,
                              "ERR 001 NOT_REGISTERED NID:%s\n",
                              NID);
                continue;
            }

            char room_name[ROOM_NAME_LEN];

            strncpy(room_name,
                    buffer + 5,
                    sizeof(room_name) - 1);

            room_name[sizeof(room_name) - 1] =
                '\0';

            if (*room_name == '\0')
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND NID:%s\n",
                              NID);
                continue;
            }

            int result =
                join_room(client_index,
                          room_name);

            if (result == -2)
            {
                send_response(client_fd,
                              "ERR 005 ROOM_LIMIT_REACHED NID:%s\n",
                              NID);
            }
            else if (result == -3)
            {
                send_response(client_fd,
                              "ERR 005 ROOM_FULL NID:%s\n",
                              NID);
            }
            else
            {
                send_response(client_fd,
                              "OK JOINED %s NID:%s\n",
                              room_name,
                              NID);
            }
        }


        /* LEAVE */
        else if (strncmp(buffer,
                         "LEAVE ",
                         6) == 0)
        {
            if (!registered)
            {
                send_response(client_fd,
                              "ERR 001 NOT_REGISTERED NID:%s\n",
                              NID);
                continue;
            }

            char room_name[ROOM_NAME_LEN];

            strncpy(room_name,
                    buffer + 6,
                    sizeof(room_name) - 1);

            room_name[sizeof(room_name) - 1] =
                '\0';

            if (*room_name == '\0')
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND NID:%s\n",
                              NID);
                continue;
            }

            int result =
                leave_room(client_index,
                           room_name);

            if (result == -1)
            {
                send_response(client_fd,
                              "ERR 003 ROOM_NOT_FOUND NID:%s\n",
                              NID);
            }
            else if (result == 0)
            {
                send_response(client_fd,
                              "ERR 003 NOT_IN_ROOM NID:%s\n",
                              NID);
            }
            else
            {
                send_response(client_fd,
                              "OK LEFT %s NID:%s\n",
                              room_name,
                              NID);
            }
        }


        /* ROOMS */
        else if (strcmp(buffer,
                        "ROOMS") == 0)
        {
            if (!registered)
            {
                send_response(client_fd,
                              "ERR 001 NOT_REGISTERED NID:%s\n",
                              NID);
                continue;
            }

            send_room_list(client_fd);
        }


        /* RMSG */
        else if (strncmp(buffer,
                         "RMSG ",
                         5) == 0)
        {
            if (!registered)
            {
                send_response(client_fd,
                              "ERR 001 NOT_REGISTERED NID:%s\n",
                              NID);
                continue;
            }

            char command_copy[1024];

            strncpy(command_copy,
                    buffer + 5,
                    sizeof(command_copy) - 1);

            command_copy[sizeof(command_copy) - 1] =
                '\0';

            char *space =
                strchr(command_copy, ' ');

            if (space == NULL)
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND NID:%s\n",
                              NID);
                continue;
            }

            *space = '\0';

            char *room_name =
                command_copy;

            char *message =
                space + 1;

            if (*room_name == '\0' ||
                *message == '\0')
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND NID:%s\n",
                              NID);
                continue;
            }

            int result =
                send_room_message(client_index,
                                  room_name,
                                  message);

            if (result == -1)
            {
                send_response(client_fd,
                              "ERR 003 ROOM_NOT_FOUND NID:%s\n",
                              NID);
            }
            else if (result == -2)
            {
                send_response(client_fd,
                              "ERR 003 NOT_IN_ROOM NID:%s\n",
                              NID);
            }
            else
            {
                send_response(client_fd,
                              "OK SENT NID:%s\n",
                              NID);
            }
        }


        /* QUIT */
        else if (strcmp(buffer,
                        "QUIT") == 0)
        {
            send_response(client_fd,
                          "OK BYE NID:%s\n",
                          NID);

            break;
        }


        /* Invalid command */
        else
        {
            send_response(client_fd,
                          "ERR 001 INVALID_COMMAND NID:%s\n",
                          NID);
        }
    }


    close(client_fd);

    remove_client_from_all_rooms(client_index);

    pthread_mutex_lock(&clients_mutex);

    clients[client_index].socket_fd =
        -1;

    clients[client_index].registered =
        0;

    clients[client_index].username[0] =
        '\0';

    pthread_mutex_unlock(&clients_mutex);

    return NULL;
}


/* Main server */
int main(void)
{
    int server_fd =
        socket(AF_INET,
               SOCK_STREAM,
               0);

    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    int option = 1;

    setsockopt(server_fd,
               SOL_SOCKET,
               SO_REUSEADDR,
               &option,
               sizeof(option));

    struct sockaddr_in server_addr;

    memset(&server_addr,
           0,
           sizeof(server_addr));

    server_addr.sin_family =
        AF_INET;

    server_addr.sin_addr.s_addr =
        INADDR_ANY;

    server_addr.sin_port =
        htons(PORT);


    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }


    if (listen(server_fd,
               BACKLOG) < 0)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }


    for (int i = 0;
         i < MAX_CLIENTS;
         i++)
    {
        clients[i].socket_fd = -1;
        clients[i].registered = 0;
        clients[i].username[0] = '\0';
    }


    for (int i = 0;
         i < MAX_ROOMS;
         i++)
    {
        rooms[i].active = 0;
        rooms[i].member_count = 0;
    }


    printf("NetMessenger server started.\n");
    printf("Listening on TCP port %d...\n",
           PORT);


    while (1)
    {
        struct sockaddr_in client_addr;

        socklen_t client_len =
            sizeof(client_addr);

        int client_fd =
            accept(server_fd,
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

        if (client_index == -1)
        {
            pthread_mutex_unlock(&clients_mutex);

            send_response(client_fd,
                          "ERR 005 SERVER_FULL NID:%s\n",
                          NID);

            close(client_fd);

            continue;
        }


        clients[client_index].socket_fd =
            client_fd;

        clients[client_index].registered =
            0;

        clients[client_index].username[0] =
            '\0';

        pthread_mutex_unlock(&clients_mutex);


        int *index =
            malloc(sizeof(int));

        if (index == NULL)
        {
            close(client_fd);

            pthread_mutex_lock(&clients_mutex);

            clients[client_index].socket_fd =
                -1;

            pthread_mutex_unlock(&clients_mutex);

            continue;
        }


        *index = client_index;

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

            clients[client_index].socket_fd =
                -1;

            pthread_mutex_unlock(&clients_mutex);

            continue;
        }

        pthread_detach(thread);
    }

    close(server_fd);

    return 0;
}
