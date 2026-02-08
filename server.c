#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> // For console colors

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void setColor(int color)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

int main()
{
    WSADATA wsa;
    SOCKET server_fd, new_socket, client_socket[MAX_CLIENTS];
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int max_sd, sd, activity, valread;

    // Username tracking
    char usernames[MAX_CLIENTS][64];
    int has_name[MAX_CLIENTS]; // 0 = no username yet, 1 = set

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        setColor(4); // Red
        printf("WSAStartup failed\n");
        setColor(7); // Reset to White
        return 1;
    }

    // Initialize client arrays
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_socket[i] = 0;
        has_name[i] = 0;
        usernames[i][0] = '\0';
    }

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET)
    {
        setColor(4); // Red for error
        printf("Socket failed\n");
        setColor(7); // Reset to default (white)
        WSACleanup();
        return 1;
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
    {
        setColor(4); // Dark red
        printf("Bind failed\n");
        setColor(7);
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Start listening
    if (listen(server_fd, 3) == SOCKET_ERROR)
    {
        setColor(4); // Red for error
        printf("Listen failed\n");
        setColor(7); // Reset to default (white)
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    setColor(10); // Light Green for success
    printf("Server started on port %d...\n", PORT);
    setColor(7); // Reset to default (white)

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = (int)server_fd;

        // Add client sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            sd = (int)client_socket[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        // Wait for activity
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity == SOCKET_ERROR)
        {
            setColor(4); // Red for error
            printf("Select error\n");
            setColor(7); // Reset to default (white)
            break;
        }

        // New connection
        if (FD_ISSET(server_fd, &readfds))
        {
            new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
            if (new_socket == INVALID_SOCKET)
            {
                setColor(4); // Red for error
                printf("Accept failed\n");
                setColor(7); // Reset to default
                continue;
            }
            setColor(11); // Cyan
            printf("New client connected: socket %d\n", (int)new_socket);
            setColor(7); // Reset to white

            // Add to client list
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    has_name[i] = 0;
                    usernames[i][0] = '\0';
                    break;
                }
            }
        }

        // IO operation on client sockets
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            SOCKET cs = client_socket[i];
            if (cs != 0 && FD_ISSET(cs, &readfds))
            {
                valread = recv(cs, buffer, BUFFER_SIZE, 0);
                if (valread <= 0)
                {
                    // Client disconnected
                    if (has_name[i])
                    {
                        setColor(6); // Yellow for disconnect warning
                        printf("Client '%s' disconnected: socket %d\n", usernames[i], (int)cs);
                        setColor(7); // Reset to default

                        // Announce to others
                        char leftmsg[128];
                        setColor(6); // Yellow for disconnect notice
                        snprintf(leftmsg, sizeof(leftmsg), "[left] %s has disconnected", usernames[i]);
                        setColor(7); // Reset to default
                        for (int j = 0; j < MAX_CLIENTS; j++)
                        {
                            if (client_socket[j] != 0 && j != i)
                            {
                                send(client_socket[j], leftmsg, (int)strlen(leftmsg), 0);
                            }
                        }
                    }
                    else
                    {
                        setColor(6); // Yellow for disconnect
                        printf("Client disconnected: socket %d\n", (int)cs);
                        setColor(7); // Reset to default
                    }

                    closesocket(cs);
                    client_socket[i] = 0;
                    has_name[i] = 0;
                    usernames[i][0] = '\0';
                }
                else
                {
                    buffer[valread] = '\0';
                    size_t blen = strlen(buffer);
                    if (blen > 0 && buffer[blen - 1] == '\n')
                        buffer[blen - 1] = '\0';

                    if (!has_name[i])
                    {
                        // First message is the username
                        int duplicate = 0;
                        for (int k = 0; k < MAX_CLIENTS; k++)
                        {
                            if (client_socket[k] != 0 && has_name[k] &&
                                strcmp(usernames[k], buffer) == 0)
                            {
                                duplicate = 1;
                                break;
                            }
                        }

                        if (duplicate)
                        {
                            setColor(6); // Yellow for warning
                            char errmsg[] = "Username already taken. Disconnecting...\n";
                            setColor(7); // Reset to default
                            send(cs, errmsg, (int)strlen(errmsg), 0);
                            closesocket(cs);
                            client_socket[i] = 0;
                            has_name[i] = 0;
                            usernames[i][0] = '\0';
                            setColor(6); // Yellow for warning
                            printf("Rejected duplicate username: %s\n", buffer);
                            setColor(7); // Reset to default
                        }
                        else
                        {
                            strncpy(usernames[i], buffer, sizeof(usernames[i]) - 1);
                            usernames[i][sizeof(usernames[i]) - 1] = '\0';
                            has_name[i] = 1;
                            setColor(10); // Light Green for success
                            printf("Client %d set username: %s\n", (int)cs, usernames[i]);
                            setColor(7); // Reset to default

                            // Send active clients list to the new client
                            setColor(10); // Light Green for informational message
                            char activeList[512] = "[active] Currently connected: ";
                            setColor(7); // Reset to default
                            int first = 1;
                            for (int k = 0; k < MAX_CLIENTS; k++)
                            {
                                if (client_socket[k] != 0 && has_name[k])
                                {
                                    if (!first)
                                        strcat(activeList, ", ");
                                    strcat(activeList, usernames[k]);
                                    first = 0;
                                }
                            }
                            send(cs, activeList, (int)strlen(activeList), 0);

                            // Announce join
                            char joinmsg[128];
                            setColor(10); // Light Green for join success
                            snprintf(joinmsg, sizeof(joinmsg), "[join] %s has connected", usernames[i]);
                            setColor(7); // Reset to default
                            for (int j = 0; j < MAX_CLIENTS; j++)
                            {
                                if (client_socket[j] != 0 && j != i)
                                {
                                    send(client_socket[j], joinmsg, (int)strlen(joinmsg), 0);
                                }
                            }
                        }
                    }
                    else
                    {
                        if (strcmp(buffer, "/quit") == 0)
                        {
                            // Announce disconnect
                            char leftmsg[128];
                            setColor(6); // Yellow for disconnect notice
                            snprintf(leftmsg, sizeof(leftmsg), "[left] %s has disconnected", usernames[i]);
                            setColor(7); // Reset to default
                            for (int j = 0; j < MAX_CLIENTS; j++)
                            {
                                if (client_socket[j] != 0 && j != i)
                                {
                                    send(client_socket[j], leftmsg, (int)strlen(leftmsg), 0);
                                }
                            }

                            setColor(6); // Yellow for /quit disconnect
                            printf("Client '%s' disconnected via /quit: socket %d\n", usernames[i], (int)cs);
                            setColor(7); // Reset to default
                            closesocket(cs);
                            client_socket[i] = 0;
                            has_name[i] = 0;
                            usernames[i][0] = '\0';
                        }
                        else if (strcmp(buffer, "/who") == 0)
                        {
                            // Build active clients list
                            setColor(10); // Light Green for informational message
                            char activeList[512] = "[active] Currently connected: ";
                            setColor(7); // Reset to default
                            int first = 1;
                            for (int k = 0; k < MAX_CLIENTS; k++)
                            {
                                if (client_socket[k] != 0 && has_name[k])
                                {
                                    if (!first)
                                        strcat(activeList, ", ");
                                    strcat(activeList, usernames[k]);
                                    first = 0;
                                }
                            }
                            send(cs, activeList, (int)strlen(activeList), 0);
                        }
                        else
                        {
                            // Normal chat: prefix with username
                            char out[BUFFER_SIZE + 96];
                            snprintf(out, sizeof(out), "%s: %s", usernames[i], buffer);
                            setColor(7); // Default white
                            printf("%s\n", out);

                            // Save to chatlog.txt
                            FILE *log = fopen("chatlog.txt", "a");
                            if (log != NULL)
                            {
                                fprintf(log, "%s\n", out);
                                fclose(log);
                            }

                            // Broadcast to all other clients
                            for (int j = 0; j < MAX_CLIENTS; j++)
                            {
                                if (client_socket[j] != 0 && j != i)
                                {
                                    send(client_socket[j], out, (int)strlen(out), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}