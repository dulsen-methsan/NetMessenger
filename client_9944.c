#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define PORT 15944

int main(void)
{
    int client_fd;
    struct sockaddr_in server_addr;

    /* Create TCP socket */
    client_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (client_fd < 0)
    {
        perror("socket");
        return 1;
    }

    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET,
                  SERVER_IP,
                  &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(client_fd);
        return 1;
    }

    /* Connect to server */
    if (connect(client_fd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(client_fd);
        return 1;
    }

    printf("Connected to NetMessenger server.\n");
    printf("Server: %s:%d\n", SERVER_IP, PORT);

    /* Send registration */
    char message[] = "REGISTER dulsen\n";

    if (send(client_fd,
             message,
             strlen(message),
             0) < 0)
    {
        perror("send");
        close(client_fd);
        return 1;
    }

    /* Receive registration response */
    char buffer[1024];

    ssize_t bytes_received;

    bytes_received = recv(client_fd,
                          buffer,
                          sizeof(buffer) - 1,
                          0);

    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0';

        printf("Server response: %s", buffer);
    }

    /* Keep connection open */
    printf("Connection is active. Press Ctrl+C to disconnect.\n");

    while (1)
    {
        sleep(1);
    }

    close(client_fd);

    return 0;
}
