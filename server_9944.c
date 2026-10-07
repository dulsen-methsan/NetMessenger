#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>

#define PORT 15944
#define BACKLOG 10
#define MAX_CLIENTS 10
#define MAX_ROOMS 50
#define MAX_ROOM_MEMBERS 10

#define MAX_USERNAME 100
#define MAX_ROOM_NAME 100
#define MAX_FILENAME 256
#define MAX_FILE_SIZE (10 * 1024 * 1024)

#define NID "6199"

#define STORAGE_ROOT "./storage/IT23619944"
#define LOG_FILE "netmsg_IT23619944.log"

typedef struct
{
    int socket_fd;
    char username[MAX_USERNAME];
    int registered;
} Client;

typedef struct
{
    char name[MAX_ROOM_NAME];
    int used;
    int members[MAX_ROOM_MEMBERS];
} Room;

Client clients[MAX_CLIENTS];
Room rooms[MAX_ROOMS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;


/* =========================
   Utility Functions
   ========================= */

int send_all(int fd, const void *buffer, size_t length)
{
    const char *ptr = buffer;

    while (length > 0)
    {
        ssize_t sent = send(fd, ptr, length, 0);

        if (sent <= 0)
        {
            return -1;
        }

        ptr += sent;
        length -= sent;
    }

    return 0;
}


int recv_all(int fd, void *buffer, size_t length)
{
    char *ptr = buffer;

    while (length > 0)
    {
        ssize_t received = recv(fd, ptr, length, 0);

        if (received <= 0)
        {
            return -1;
        }

        ptr += received;
        length -= received;
    }

    return 0;
}


int discard_bytes(int fd, long long length)
{
    char buffer[65536];

    while (length > 0)
    {
        size_t amount;

        if (length > (long long)sizeof(buffer))
        {
            amount = sizeof(buffer);
        }
        else
        {
            amount = (size_t)length;
        }

        if (recv_all(fd, buffer, amount) < 0)
        {
            return -1;
        }

        length -= amount;
    }

    return 0;
}


void log_event(const char *format, ...)
{
    FILE *file;
    time_t now;
    struct tm time_info;
    char timestamp[64];

    va_list args;

    now = time(NULL);
    localtime_r(&now, &time_info);

    strftime(timestamp,
             sizeof(timestamp),
             "%Y-%m-%d %H:%M:%S",
             &time_info);

    pthread_mutex_lock(&log_mutex);

    file = fopen(LOG_FILE, "a");

    if (file != NULL)
    {
        fprintf(file, "[%s] ", timestamp);

        va_start(args, format);
        vfprintf(file, format, args);
        va_end(args);

        fprintf(file, "\n");

        fclose(file);
    }

    pthread_mutex_unlock(&log_mutex);
}


void send_response(int client_fd, const char *message)
{
    char response[1024];

    snprintf(response,
             sizeof(response),
             "%s NID:%s\n",
             message,
             NID);

    send_all(client_fd,
             response,
             strlen(response));
}


/* =========================
   Client Functions
   ========================= */

int find_free_client_slot(void)
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


/* =========================
   Room Functions
   ========================= */

int find_room(const char *room_name)
{
    int i;

    for (i = 0; i < MAX_ROOMS; i++)
    {
        if (rooms[i].used &&
            strcmp(rooms[i].name, room_name) == 0)
        {
            return i;
        }
    }

    return -1;
}


int find_free_room(void)
{
    int i;

    for (i = 0; i < MAX_ROOMS; i++)
    {
        if (!rooms[i].used)
        {
            return i;
        }
    }

    return -1;
}


int room_has_member(int room_index, int client_index)
{
    int i;

    if (room_index < 0)
    {
        return 0;
    }

    for (i = 0; i < MAX_ROOM_MEMBERS; i++)
    {
        if (rooms[room_index].members[i] == client_index)
        {
            return 1;
        }
    }

    return 0;
}


int add_room_member(int room_index, int client_index)
{
    int i;

    if (room_has_member(room_index, client_index))
    {
        return 0;
    }

    for (i = 0; i < MAX_ROOM_MEMBERS; i++)
    {
        if (rooms[room_index].members[i] == -1)
        {
            rooms[room_index].members[i] = client_index;
            return 0;
        }
    }

    return -1;
}


void remove_room_member(int room_index, int client_index)
{
    int i;

    for (i = 0; i < MAX_ROOM_MEMBERS; i++)
    {
        if (rooms[room_index].members[i] == client_index)
        {
            rooms[room_index].members[i] = -1;
        }
    }
}


void cleanup_empty_rooms(void)
{
    int i;
    int j;
    int has_member;

    for (i = 0; i < MAX_ROOMS; i++)
    {
        if (!rooms[i].used)
        {
            continue;
        }

        has_member = 0;

        for (j = 0; j < MAX_ROOM_MEMBERS; j++)
        {
            if (rooms[i].members[j] != -1)
            {
                has_member = 1;
                break;
            }
        }

        if (!has_member)
        {
            rooms[i].used = 0;
        }
    }
}


/* =========================
   Presence Notifications
   ========================= */

void broadcast_presence(int sender_index,
                        const char *type,
                        const char *username)
{
    char message[256];
    int i;

    snprintf(message,
             sizeof(message),
             "MSG PRESENCE %s %s\n",
             type,
             username);

    pthread_mutex_lock(&clients_mutex);

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (i != sender_index &&
            clients[i].registered &&
            clients[i].socket_fd != -1)
        {
            send_all(clients[i].socket_fd,
                     message,
                     strlen(message));
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}


/* =========================
   LIST
   ========================= */

void send_user_list(int client_fd)
{
    char response[1024];
    int offset;
    int first_user;
    int i;

    offset = snprintf(response,
                      sizeof(response),
                      "OK USERS ");

    first_user = 1;

    pthread_mutex_lock(&clients_mutex);

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].registered)
        {
            offset += snprintf(response + offset,
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

    send_all(client_fd,
             response,
             strlen(response));
}


/* =========================
   Broadcast Messaging
   ========================= */

void broadcast_message(int sender_index,
                       const char *message)
{
    char response[1200];
    int i;

    snprintf(response,
             sizeof(response),
             "MSG BCAST %s %s\n",
             clients[sender_index].username,
             message);

    pthread_mutex_lock(&clients_mutex);

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (i != sender_index &&
            clients[i].registered &&
            clients[i].socket_fd != -1)
        {
            send_all(clients[i].socket_fd,
                     response,
                     strlen(response));
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}


/* =========================
   Private Messaging
   ========================= */

int send_private_message(int sender_index,
                         const char *target,
                         const char *message)
{
    int target_index;
    char response[1200];

    pthread_mutex_lock(&clients_mutex);

    target_index = find_client_by_username(target);

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

    send_all(clients[target_index].socket_fd,
             response,
             strlen(response));

    pthread_mutex_unlock(&clients_mutex);

    return 0;
}


/* =========================
   Room Messaging
   ========================= */

void send_room_message(int sender_index,
                       int room_index,
                       const char *message)
{
    char response[1400];
    int i;
    int member_index;

    snprintf(response,
             sizeof(response),
             "MSG ROOM %s %s %s\n",
             rooms[room_index].name,
             clients[sender_index].username,
             message);

    for (i = 0; i < MAX_ROOM_MEMBERS; i++)
    {
        member_index = rooms[room_index].members[i];

        if (member_index >= 0 &&
            member_index != sender_index &&
            clients[member_index].registered &&
            clients[member_index].socket_fd != -1)
        {
            send_all(clients[member_index].socket_fd,
                     response,
                     strlen(response));
        }
    }
}


/* =========================
   File Functions
   ========================= */

int valid_filename(const char *filename)
{
    if (filename == NULL ||
        filename[0] == '\0')
    {
        return 0;
    }

    if (strlen(filename) >= MAX_FILENAME)
    {
        return 0;
    }

    if (strstr(filename, "..") != NULL)
    {
        return 0;
    }

    if (strchr(filename, '/') != NULL)
    {
        return 0;
    }

    if (strchr(filename, '\\') != NULL)
    {
        return 0;
    }

    return 1;
}


int receive_and_store_file(int sender_index,
                           const char *target,
                           const char *filename,
                           long long filesize)
{
    int target_user;
    int target_room;

    char directory[PATH_MAX];
    char filepath[PATH_MAX + MAX_FILENAME + 2];

    FILE *file;

    char buffer[65536];

    long long remaining;


    target_user = find_client_by_username(target);
    target_room = find_room(target);


    if (target_user == -1 &&
        target_room == -1)
    {
        return -3;
    }


    if (filesize < 0 ||
        filesize > MAX_FILE_SIZE)
    {
        return -2;
    }


    if (!valid_filename(filename))
    {
        return -6;
    }


    mkdir("./storage", 0755);
    mkdir(STORAGE_ROOT, 0755);


    snprintf(directory,
             sizeof(directory),
             "%s/%s",
             STORAGE_ROOT,
             clients[sender_index].username);

    mkdir(directory, 0755);


    int path_result =
        snprintf(filepath,
                 sizeof(filepath),
                 "%s/%s",
                 directory,
                 filename);

    if (path_result < 0 ||
        (size_t)path_result >= sizeof(filepath))
    {
        return -6;
    }


    file = fopen(filepath, "wb");

    if (file == NULL)
    {
        return -4;
    }


    remaining = filesize;


    while (remaining > 0)
    {
        size_t amount;

        if (remaining > (long long)sizeof(buffer))
        {
            amount = sizeof(buffer);
        }
        else
        {
            amount = (size_t)remaining;
        }


        if (recv_all(clients[sender_index].socket_fd,
                     buffer,
                     amount) < 0)
        {
            fclose(file);
            return -5;
        }


        if (fwrite(buffer,
                   1,
                   amount,
                   file) != amount)
        {
            fclose(file);
            return -4;
        }


        remaining -= amount;
    }


    fclose(file);


    /* Send file to target */
    {
        char header[512];

        snprintf(header,
                 sizeof(header),
                 "MSG FILE %s %s %lld\n",
                 clients[sender_index].username,
                 filename,
                 filesize);


        if (target_user != -1 &&
            target_user != sender_index)
        {
            FILE *read_file;

            send_all(clients[target_user].socket_fd,
                     header,
                     strlen(header));


            read_file = fopen(filepath, "rb");

            if (read_file != NULL)
            {
                size_t read_bytes;

                while ((read_bytes =
                        fread(buffer,
                              1,
                              sizeof(buffer),
                              read_file)) > 0)
                {
                    send_all(clients[target_user].socket_fd,
                             buffer,
                             read_bytes);
                }

                fclose(read_file);
            }
        }


        if (target_room != -1)
        {
            int i;

            for (i = 0; i < MAX_ROOM_MEMBERS; i++)
            {
                int member =
                    rooms[target_room].members[i];

                if (member >= 0 &&
                    member != sender_index &&
                    clients[member].registered &&
                    clients[member].socket_fd != -1)
                {
                    FILE *read_file;

                    send_all(clients[member].socket_fd,
                             header,
                             strlen(header));


                    read_file = fopen(filepath, "rb");

                    if (read_file != NULL)
                    {
                        size_t read_bytes;

                        while ((read_bytes =
                                fread(buffer,
                                      1,
                                      sizeof(buffer),
                                      read_file)) > 0)
                        {
                            send_all(clients[member].socket_fd,
                                     buffer,
                                     read_bytes);
                        }

                        fclose(read_file);
                    }
                }
            }
        }
    }


    log_event("FILE sender=%s target=%s filename=%s size=%lld",
              clients[sender_index].username,
              target,
              filename,
              filesize);

    return 0;
}


/* =========================
   Client Thread
   ========================= */

void *handle_client(void *argument)
{
    int client_index;
    int client_fd;
    int registered;

    char line[4096];


    client_index = *(int *)argument;

    free(argument);


    client_fd =
        clients[client_index].socket_fd;

    registered = 0;


    printf("Client connected.\n");

    log_event("CONNECT slot=%d",
              client_index);


    while (1)
    {
        size_t position = 0;


        /* Read one line */
        while (position < sizeof(line) - 1)
        {
            char character;

            ssize_t received =
                recv(client_fd,
                     &character,
                     1,
                     0);

            if (received <= 0)
            {
                goto disconnect;
            }


            if (character == '\n')
            {
                break;
            }


            if (character != '\r')
            {
                line[position++] =
                    character;
            }
        }


        line[position] = '\0';


        if (position == 0)
        {
            continue;
        }


        /* REGISTER */
        if (strncmp(line,
                    "REGISTER ",
                    9) == 0)
        {
            char *username =
                line + 9;


            if (registered)
            {
                send_response(client_fd,
                              "ERR 001 ALREADY_REGISTERED");

                continue;
            }


            if (strlen(username) == 0 ||
                strlen(username) >= MAX_USERNAME)
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND");

                continue;
            }


            pthread_mutex_lock(&clients_mutex);


            if (find_client_by_username(username) != -1)
            {
                pthread_mutex_unlock(&clients_mutex);

                send_response(client_fd,
                              "ERR 001 USERNAME_TAKEN");

                continue;
            }


            strncpy(clients[client_index].username,
                    username,
                    MAX_USERNAME - 1);

            clients[client_index]
                .username[MAX_USERNAME - 1] = '\0';


            clients[client_index].registered =
                1;

            registered = 1;


            pthread_mutex_unlock(&clients_mutex);


            {
                char response[256];

                snprintf(response,
                         sizeof(response),
                         "OK REGISTERED %s",
                         username);

                send_response(client_fd,
                              response);
            }


            log_event("REGISTER username=%s",
                      username);


            broadcast_presence(client_index,
                               "JOIN",
                               username);
        }


        /* Commands require registration */
        else if (!registered)
        {
            send_response(client_fd,
                          "ERR 001 NOT_REGISTERED");
        }


        /* LIST */
        else if (strcmp(line,
                        "LIST") == 0)
        {
            send_user_list(client_fd);
        }


        /* BCAST */
        else if (strncmp(line,
                         "BCAST ",
                         6) == 0)
        {
            char *message =
                line + 6;


            if (*message == '\0')
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND");

                continue;
            }


            broadcast_message(client_index,
                              message);


            send_response(client_fd,
                          "OK SENT");


            log_event("BCAST sender=%s message=%s",
                      clients[client_index].username,
                      message);
        }


        /* PMSG */
        else if (strncmp(line,
                         "PMSG ",
                         5) == 0)
        {
            char *data =
                line + 5;

            char *space =
                strchr(data, ' ');


            if (space == NULL)
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND");

                continue;
            }


            *space = '\0';


            char *target =
                data;

            char *message =
                space + 1;


            if (*target == '\0' ||
                *message == '\0')
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND");

                continue;
            }


            if (send_private_message(client_index,
                                     target,
                                     message) == -1)
            {
                send_response(client_fd,
                              "ERR 002 USER_NOT_FOUND");
            }
            else
            {
                send_response(client_fd,
                              "OK SENT");

                log_event("PMSG sender=%s target=%s message=%s",
                          clients[client_index].username,
                          target,
                          message);
            }
        }


        /* JOIN */
        else if (strncmp(line,
                         "JOIN ",
                         5) == 0)
        {
            char *room_name =
                line + 5;

            int room_index;
            int result;


            pthread_mutex_lock(&clients_mutex);


            room_index =
                find_room(room_name);


            if (room_index == -1)
            {
                room_index =
                    find_free_room();


                if (room_index >= 0)
                {
                    rooms[room_index].used =
                        1;


                    strncpy(rooms[room_index].name,
                            room_name,
                            MAX_ROOM_NAME - 1);

                    rooms[room_index]
                        .name[MAX_ROOM_NAME - 1] =
                        '\0';


                    for (int i = 0;
                         i < MAX_ROOM_MEMBERS;
                         i++)
                    {
                        rooms[room_index]
                            .members[i] = -1;
                    }
                }
            }


            if (room_index < 0)
            {
                result = -1;
            }
            else
            {
                result =
                    add_room_member(room_index,
                                    client_index);
            }


            pthread_mutex_unlock(&clients_mutex);


            if (result < 0)
            {
                send_response(client_fd,
                              "ERR 003 ROOM_FULL");
            }
            else
            {
                char response[256];

                snprintf(response,
                         sizeof(response),
                         "OK JOINED %s",
                         room_name);

                send_response(client_fd,
                              response);


                log_event("JOIN username=%s room=%s",
                          clients[client_index].username,
                          room_name);
            }
        }


        /* LEAVE */
        else if (strncmp(line,
                         "LEAVE ",
                         6) == 0)
        {
            char *room_name =
                line + 6;

            int room_index;


            pthread_mutex_lock(&clients_mutex);


            room_index =
                find_room(room_name);


            if (room_index == -1)
            {
                pthread_mutex_unlock(&clients_mutex);

                send_response(client_fd,
                              "ERR 003 ROOM_NOT_FOUND");
            }
            else if (!room_has_member(room_index,
                                      client_index))
            {
                pthread_mutex_unlock(&clients_mutex);

                send_response(client_fd,
                              "ERR 003 NOT_IN_ROOM");
            }
            else
            {
                remove_room_member(room_index,
                                   client_index);

                cleanup_empty_rooms();

                pthread_mutex_unlock(&clients_mutex);


                {
                    char response[256];

                    snprintf(response,
                             sizeof(response),
                             "OK LEFT %s",
                             room_name);

                    send_response(client_fd,
                                  response);
                }


                log_event("LEAVE username=%s room=%s",
                          clients[client_index].username,
                          room_name);
            }
        }


        /* ROOMS */
        else if (strcmp(line,
                        "ROOMS") == 0)
        {
            char response[2048];

            int offset;
            int first;
            int i;


            offset =
                snprintf(response,
                         sizeof(response),
                         "OK ROOMS ");


            first = 1;


            pthread_mutex_lock(&clients_mutex);


            for (i = 0; i < MAX_ROOMS; i++)
            {
                if (rooms[i].used)
                {
                    offset +=
                        snprintf(response + offset,
                                 sizeof(response) - offset,
                                 "%s%s",
                                 first ? "" : ",",
                                 rooms[i].name);

                    first = 0;
                }
            }


            pthread_mutex_unlock(&clients_mutex);


            snprintf(response + offset,
                     sizeof(response) - offset,
                     " NID:%s\n",
                     NID);


            send_all(client_fd,
                     response,
                     strlen(response));
        }


        /* RMSG */
        else if (strncmp(line,
                         "RMSG ",
                         5) == 0)
        {
            char *data =
                line + 5;

            char *space =
                strchr(data, ' ');

            int room_index;
            int member;


            if (space == NULL)
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND");

                continue;
            }


            *space = '\0';


            char *room_name =
                data;

            char *message =
                space + 1;


            pthread_mutex_lock(&clients_mutex);


            room_index =
                find_room(room_name);


            member =
                room_index >= 0 &&
                room_has_member(room_index,
                                client_index);


            if (room_index == -1)
            {
                pthread_mutex_unlock(&clients_mutex);

                send_response(client_fd,
                              "ERR 003 ROOM_NOT_FOUND");
            }
            else if (!member)
            {
                pthread_mutex_unlock(&clients_mutex);

                send_response(client_fd,
                              "ERR 003 NOT_IN_ROOM");
            }
            else
            {
                send_room_message(client_index,
                                  room_index,
                                  message);

                pthread_mutex_unlock(&clients_mutex);


                send_response(client_fd,
                              "OK SENT");


                log_event("RMSG sender=%s room=%s message=%s",
                          clients[client_index].username,
                          room_name,
                          message);
            }
        }


        /* SENDFILE */
        else if (strncmp(line,
                         "SENDFILE ",
                         9) == 0)
        {
            char *data =
                line + 9;

            char *first_space =
                strchr(data, ' ');

            char *filename;
            char *size_text;
            char *second_space;

            long long filesize;
            char *end_pointer;

            int target_user;
            int target_room;
            int result;


            if (first_space == NULL)
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND");

                continue;
            }


            *first_space = '\0';


            filename =
                first_space + 1;


            second_space =
                strchr(filename, ' ');


            if (second_space == NULL)
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND");

                continue;
            }


            *second_space = '\0';


            size_text =
                second_space + 1;


            filesize =
                strtoll(size_text,
                        &end_pointer,
                        10);


            if (*size_text == '\0' ||
                *end_pointer != '\0')
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND");

                continue;
            }


            if (filesize < 0)
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND");

                continue;
            }


            if (filesize > MAX_FILE_SIZE)
            {
                if (discard_bytes(client_fd, filesize) < 0)
                {
                    goto disconnect;
                }

                send_response(client_fd,
                              "ERR 004 FILE_TOO_LARGE");

                continue;
            }


            if (!valid_filename(filename))
            {
                if (discard_bytes(client_fd, filesize) < 0)
                {
                    goto disconnect;
                }

                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND");

                continue;
            }


            pthread_mutex_lock(&clients_mutex);

            target_user =
                find_client_by_username(data);

            target_room =
                find_room(data);

            pthread_mutex_unlock(&clients_mutex);


            if (target_user == -1 &&
                target_room == -1)
            {
                if (discard_bytes(client_fd, filesize) < 0)
                {
                    goto disconnect;
                }

                send_response(client_fd,
                              "ERR 002 USER_NOT_FOUND");

                continue;
            }


            result =
                receive_and_store_file(client_index,
                                        data,
                                        filename,
                                        filesize);


            if (result == -2)
            {
                send_response(client_fd,
                              "ERR 004 FILE_TOO_LARGE");
            }
            else if (result == -3)
            {
                send_response(client_fd,
                              "ERR 002 USER_NOT_FOUND");
            }
            else if (result == -6)
            {
                send_response(client_fd,
                              "ERR 001 INVALID_COMMAND");
            }
            else if (result < 0)
            {
                send_response(client_fd,
                              "ERR 001 FILE_ERROR");
            }
            else
            {
                char response[512];

                snprintf(response,
                         sizeof(response),
                         "OK FILE_RECEIVED %s",
                         filename);

                send_response(client_fd,
                              response);
            }
        }


        /* QUIT */
        else if (strcmp(line,
                        "QUIT") == 0)
        {
            send_response(client_fd,
                          "OK BYE");


            log_event("QUIT username=%s",
                      clients[client_index].username);

            break;
        }


        /* Invalid command */
        else
        {
            send_response(client_fd,
                          "ERR 001 INVALID_COMMAND");
        }
    }


disconnect:

    if (registered)
    {
        char username[MAX_USERNAME];


        strncpy(username,
                clients[client_index].username,
                MAX_USERNAME - 1);

        username[MAX_USERNAME - 1] =
            '\0';


        pthread_mutex_lock(&clients_mutex);


        for (int i = 0; i < MAX_ROOMS; i++)
        {
            if (rooms[i].used)
            {
                remove_room_member(i,
                                   client_index);
            }
        }


        cleanup_empty_rooms();


        clients[client_index].registered =
            0;

        clients[client_index].username[0] =
            '\0';


        pthread_mutex_unlock(&clients_mutex);


        broadcast_presence(client_index,
                           "LEAVE",
                           username);


        log_event("DISCONNECT username=%s",
                  username);
    }


    close(client_fd);


    pthread_mutex_lock(&clients_mutex);

    clients[client_index].socket_fd =
        -1;

    pthread_mutex_unlock(&clients_mutex);


    return NULL;
}


/* =========================
   Main
   ========================= */

int main(void)
{
    int server_fd;

    struct sockaddr_in server_address;

    int option = 1;

    int i;


    mkdir("./storage", 0755);
    mkdir(STORAGE_ROOT, 0755);


    for (i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].socket_fd = -1;
        clients[i].registered = 0;
        clients[i].username[0] = '\0';
    }


    for (i = 0; i < MAX_ROOMS; i++)
    {
        rooms[i].used = 0;

        for (int j = 0;
             j < MAX_ROOM_MEMBERS;
             j++)
        {
            rooms[i].members[j] = -1;
        }
    }


    server_fd =
        socket(AF_INET,
               SOCK_STREAM,
               0);


    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }


    setsockopt(server_fd,
               SOL_SOCKET,
               SO_REUSEADDR,
               &option,
               sizeof(option));


    memset(&server_address,
           0,
           sizeof(server_address));


    server_address.sin_family =
        AF_INET;

    server_address.sin_addr.s_addr =
        INADDR_ANY;

    server_address.sin_port =
        htons(PORT);


    if (bind(server_fd,
             (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0)
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


    printf("NetMessenger server started.\n");
    printf("Listening on TCP port %d...\n",
           PORT);


    log_event("SERVER_START port=%d",
              PORT);


    while (1)
    {
        struct sockaddr_in client_address;

        socklen_t client_length =
            sizeof(client_address);

        int client_fd;

        int client_index;

        int *index_pointer;

        pthread_t thread;


        client_fd =
            accept(server_fd,
                   (struct sockaddr *)&client_address,
                   &client_length);


        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }


        pthread_mutex_lock(&clients_mutex);


        client_index =
            find_free_client_slot();


        if (client_index == -1)
        {
            pthread_mutex_unlock(&clients_mutex);

            send_response(client_fd,
                          "ERR 005 SERVER_FULL");

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


        index_pointer =
            malloc(sizeof(int));


        if (index_pointer == NULL)
        {
            close(client_fd);

            pthread_mutex_lock(&clients_mutex);

            clients[client_index].socket_fd =
                -1;

            pthread_mutex_unlock(&clients_mutex);

            continue;
        }


        *index_pointer =
            client_index;


        if (pthread_create(&thread,
                           NULL,
                           handle_client,
                           index_pointer) != 0)
        {
            free(index_pointer);

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
