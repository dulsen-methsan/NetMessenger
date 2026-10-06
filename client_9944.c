#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define PORT 15944

#define BUFFER_SIZE 4096
#define MAX_FILE_SIZE (10 * 1024 * 1024)


/* =========================
   Socket Functions
   ========================= */

int send_all(int fd,
             const void *buffer,
             size_t length)
{
    const char *ptr = buffer;

    while (length > 0)
    {
        ssize_t sent =
            send(fd,
                 ptr,
                 length,
                 0);

        if (sent <= 0)
        {
            return -1;
        }

        ptr += sent;
        length -= sent;
    }

    return 0;
}


int recv_all(int fd,
             void *buffer,
             size_t length)
{
    char *ptr = buffer;

    while (length > 0)
    {
        ssize_t received =
            recv(fd,
                 ptr,
                 length,
                 0);

        if (received <= 0)
        {
            return -1;
        }

        ptr += received;
        length -= received;
    }

    return 0;
}


/* =========================
   Read One Line
   ========================= */

int read_line(int fd,
              char *buffer,
              size_t capacity)
{
    size_t position = 0;


    while (position < capacity - 1)
    {
        char character;

        ssize_t received =
            recv(fd,
                 &character,
                 1,
                 0);


        if (received <= 0)
        {
            return -1;
        }


        if (character == '\n')
        {
            buffer[position] =
                '\0';

            return 0;
        }


        if (character != '\r')
        {
            buffer[position++] =
                character;
        }
    }


    buffer[position] =
        '\0';


    return -2;
}


/* =========================
   Filename Validation
   ========================= */

int valid_filename(const char *filename)
{
    if (filename == NULL ||
        filename[0] == '\0')
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


/* =========================
   Send File
   ========================= */

int send_file(int client_fd,
              const char *target,
              const char *filename)
{
    FILE *file;

    long filesize;

    char header[1024];

    char buffer[65536];

    size_t bytes_read;


    if (!valid_filename(filename))
    {
        printf("Invalid filename.\n");
        return -1;
    }


    file =
        fopen(filename, "rb");


    if (file == NULL)
    {
        perror("fopen");
        return -1;
    }


    fseek(file,
          0,
          SEEK_END);


    filesize =
        ftell(file);


    rewind(file);


    if (filesize < 0 ||
        filesize > MAX_FILE_SIZE)
    {
        fclose(file);

        printf("File is too large. Maximum is 10 MB.\n");

        return -1;
    }


    snprintf(header,
             sizeof(header),
             "SENDFILE %s %s %ld\n",
             target,
             filename,
             filesize);


    if (send_all(client_fd,
                 header,
                 strlen(header)) < 0)
    {
        fclose(file);
        return -1;
    }


    while ((bytes_read =
            fread(buffer,
                  1,
                  sizeof(buffer),
                  file)) > 0)
    {
        if (send_all(client_fd,
                     buffer,
                     bytes_read) < 0)
        {
            fclose(file);
            return -1;
        }
    }


    fclose(file);


    return 0;
}


/* =========================
   Main
   ========================= */

int main(void)
{
    int client_fd;

    struct sockaddr_in server_address;


    client_fd =
        socket(AF_INET,
               SOCK_STREAM,
               0);


    if (client_fd < 0)
    {
        perror("socket");
        return 1;
    }


    memset(&server_address,
           0,
           sizeof(server_address));


    server_address.sin_family =
        AF_INET;

    server_address.sin_port =
        htons(PORT);


    if (inet_pton(AF_INET,
                  SERVER_IP,
                  &server_address.sin_addr) <= 0)
    {
        perror("inet_pton");

        close(client_fd);

        return 1;
    }


    if (connect(client_fd,
                (struct sockaddr *)&server_address,
                sizeof(server_address)) < 0)
    {
        perror("connect");

        close(client_fd);

        return 1;
    }


    printf("Connected to NetMessenger server.\n");

    printf("Server: %s:%d\n",
           SERVER_IP,
           PORT);


    /* =========================
       Registration
       ========================= */

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
                     "\r\n")] =
        '\0';


    char command[4096];


    snprintf(command,
             sizeof(command),
             "REGISTER %s\n",
             username);


    if (send_all(client_fd,
                 command,
                 strlen(command)) < 0)
    {
        perror("send");

        close(client_fd);

        return 1;
    }


    char line[4096];


    if (read_line(client_fd,
                  line,
                  sizeof(line)) < 0)
    {
        printf("Server disconnected.\n");

        close(client_fd);

        return 1;
    }


    printf("Server response: %s\n",
           line);


    if (strncmp(line,
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
    printf("  SENDFILE <target> <filename>\n");
    printf("  QUIT\n");

    printf("\n");


    /* =========================
       Interactive Loop
       ========================= */

    while (1)
    {
        fd_set read_fds;

        int max_fd;


        FD_ZERO(&read_fds);


        FD_SET(STDIN_FILENO,
               &read_fds);


        FD_SET(client_fd,
               &read_fds);


        max_fd =
            client_fd > STDIN_FILENO
            ? client_fd
            : STDIN_FILENO;


        if (select(max_fd + 1,
                   &read_fds,
                   NULL,
                   NULL,
                   NULL) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }


            perror("select");

            break;
        }


        /* =========================
           Incoming Server Message
           ========================= */

        if (FD_ISSET(client_fd,
                     &read_fds))
        {
            if (read_line(client_fd,
                          line,
                          sizeof(line)) < 0)
            {
                printf("\nServer disconnected.\n");
                break;
            }


            /*
             * File notification:
             *
             * MSG FILE <sender> <filename> <filesize>
             */

            if (strncmp(line,
                        "MSG FILE ",
                        9) == 0)
            {
                char sender[100];
                char filename[256];

                long long filesize;

                int parsed;


                parsed =
                    sscanf(line,
                           "MSG FILE %99s %255s %lld",
                           sender,
                           filename,
                           &filesize);


                if (parsed == 3 &&
                    filesize >= 0 &&
                    filesize <= MAX_FILE_SIZE)
                {
                    char save_name[512];

                    FILE *file;

                    char buffer[65536];

                    long long remaining;

                    int success = 1;


                    snprintf(save_name,
                             sizeof(save_name),
                             "received_%s",
                             filename);


                    file =
                        fopen(save_name,
                              "wb");


                    if (file == NULL)
                    {
                        perror("fopen");

                        success = 0;
                    }


                    remaining =
                        filesize;


                    while (remaining > 0)
                    {
                        size_t amount;


                        if (remaining >
                            (long long)sizeof(buffer))
                        {
                            amount =
                                sizeof(buffer);
                        }
                        else
                        {
                            amount =
                                (size_t)remaining;
                        }


                        if (recv_all(client_fd,
                                     buffer,
                                     amount) < 0)
                        {
                            success = 0;
                            break;
                        }


                        if (file != NULL)
                        {
                            fwrite(buffer,
                                   1,
                                   amount,
                                   file);
                        }


                        remaining -= amount;
                    }


                    if (file != NULL)
                    {
                        fclose(file);
                    }


                    if (success)
                    {
                        printf("\nFile received from %s.\n",
                               sender);

                        printf("Saved as: %s\n",
                               save_name);
                    }
                    else
                    {
                        printf("\nFile receiving failed.\n");
                    }
                }
                else
                {
                    printf("\nInvalid file notification.\n");
                }
            }
            else
            {
                printf("\nServer: %s\n",
                       line);
            }


            printf("NetMessenger> ");

            fflush(stdout);
        }


        /* =========================
           Keyboard Input
           ========================= */

        if (FD_ISSET(STDIN_FILENO,
                     &read_fds))
        {
            char input[2048];


            if (fgets(input,
                      sizeof(input),
                      stdin) == NULL)
            {
                break;
            }


            input[strcspn(input,
                          "\r\n")] =
                '\0';


            if (strlen(input) == 0)
            {
                printf("NetMessenger> ");

                fflush(stdout);

                continue;
            }


            /* =========================
               SENDFILE
               ========================= */

            if (strncmp(input,
                        "SENDFILE ",
                        9) == 0)
            {
                char *data =
                    input + 9;


                char *space =
                    strchr(data, ' ');


                if (space == NULL)
                {
                    printf("Usage: SENDFILE <target> <filename>\n");

                    printf("NetMessenger> ");

                    fflush(stdout);

                    continue;
                }


                *space = '\0';


                char *target =
                    data;


                char *filename =
                    space + 1;


                if (*target == '\0' ||
                    *filename == '\0')
                {
                    printf("Usage: SENDFILE <target> <filename>\n");

                    printf("NetMessenger> ");

                    fflush(stdout);

                    continue;
                }


                printf("Sending file...\n");


                if (send_file(client_fd,
                              target,
                              filename) == 0)
                {
                    printf("File data sent. Waiting for server response...\n");
                }
                else
                {
                    printf("File send failed.\n");
                }
            }


            /* =========================
               QUIT
               ========================= */

            else if (strcmp(input,
                            "QUIT") == 0)
            {
                char quit_command[] =
                    "QUIT\n";


                send_all(client_fd,
                         quit_command,
                         strlen(quit_command));


                if (read_line(client_fd,
                              line,
                              sizeof(line)) == 0)
                {
                    printf("Server response: %s\n",
                           line);
                }


                break;
            }


            /* =========================
               Normal Command
               ========================= */

            else
            {
                char outgoing[2200];


                snprintf(outgoing,
                         sizeof(outgoing),
                         "%s\n",
                         input);


                if (send_all(client_fd,
                             outgoing,
                             strlen(outgoing)) < 0)
                {
                    perror("send");

                    break;
                }
            }
        }
    }


    close(client_fd);


    printf("Disconnected from server.\n");


    return 0;
}
