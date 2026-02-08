#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// ANSI color codes
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

SOCKET sock = 0;

// Thread function to receive messages
DWORD WINAPI receive_messages(LPVOID arg) {
    char buffer[BUFFER_SIZE];
    int valread;
    while ((valread = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[valread] = '\0';
        printf("\n" CYAN "Message: %s\n" RESET "> ", buffer);
        fflush(stdout);
    }
    return 0;
}

int main() {
    struct sockaddr_in serv_addr;
    char message[BUFFER_SIZE];
    char username[64];
    WSADATA wsa;
    HANDLE recv_thread;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf(RED "WSAStartup failed\n" RESET);
        return 1;
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf(RED "Socket creation error\n" RESET);
        WSACleanup();
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // Convert IP address (using inet_addr for Windows)
    serv_addr.sin_addr.s_addr = inet_addr("192.168.43.76");
    if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
        printf(RED "Invalid address\n" RESET);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf(RED "Connection Failed\n" RESET);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf(GREEN "Connected to chat server!\n" RESET);

    // Ask for username and send it to server
    printf(YELLOW "Enter your username: " RESET);
    fgets(username, sizeof(username), stdin);
    size_t ulen = strlen(username);
    if (ulen > 0 && username[ulen - 1] == '\n') username[ulen - 1] = '\0';
    send(sock, username, (int)strlen(username), 0);

    // Create thread to receive messages
    recv_thread = CreateThread(NULL, 0, receive_messages, NULL, 0, NULL);

    // Send messages loop
    while (1) {
        printf("> ");
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) break;

        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') message[len - 1] = '\0';

        if (strcmp(message, "/quit") == 0) {
            send(sock, message, (int)strlen(message), 0);
            printf(YELLOW "Disconnecting...\n" RESET);
            break;
        }

        send(sock, message, (int)strlen(message), 0);
    }

    // Cleanup
    closesocket(sock);
    WSACleanup();
    return 0;
}