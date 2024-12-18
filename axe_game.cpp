#define WIN32_LEAN_AND_MEAN
#if defined(_WIN32)
    #define NOGDI
    #define NOUSER
#endif

#include <winsock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#if defined(_WIN32)
    #undef near
    #undef far
#endif
#include "raylib.h"
#include <iostream>
#include <cstdio>
#include <random>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_PORT "8080"
#define IPADDRESS "192.168.1.222"

struct Player{
    int id;
    int x;
    int y;
    int radius;
    Player(int pid):id(pid){};
};

typedef struct{
    std::vector<Player*> players;
} Packet;

void startServer(Packet& packets) {
    WSADATA wsaData;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed with error: \n");
        exit(1);
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = AI_PASSIVE;

    if(getaddrinfo(NULL, DEFAULT_PORT, &hints, &result)){
        std::cout << "getaddrinfo failed with error: \n";
        exit(1);
    }

    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        exit(1);
    }

    if(bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen)) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        exit(1);
    }

    freeaddrinfo(result);

    if(listen(ListenSocket, SOMAXCONN)){
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        exit(1);
    }

    ClientSocket = accept(ListenSocket, NULL, NULL);
    if(ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        exit(1);
    }

    closesocket(ListenSocket);

    int iResult =1, iSendResult =1;

    do {

        iResult = recv(ClientSocket, (char*)&packets,sizeof packets, 0);
            for(size_t i = 0; i < packets.players.size(); i++) {
                std::cout << packets.players[i]->id << std::endl;
            }
            if (iResult > 0){
                iSendResult = send(ClientSocket, (char*)&packets, iResult,0);
                if(iSendResult == SOCKET_ERROR) {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(ClientSocket);
                    WSACleanup();
                    exit(1);
                }

            }
            else if(iResult == 0)
                printf("Connection closing.. \n");
            else {
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                exit(1);
            }
        } while(iResult > 0);

    if(shutdown(ClientSocket, SD_SEND)) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        exit(1);
    }

    closesocket(ClientSocket);
    WSACleanup();
};

void startClient(Packet& packets) {
    Player* client = new Player(rand()%1000);
    packets.players.push_back(client);
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;

    struct addrinfo  *result = NULL,
                     *ptr   = NULL,
                     hints;

    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed with error: \n");
        exit(1);
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = AI_PASSIVE;

    if(getaddrinfo(IPADDRESS, DEFAULT_PORT, &hints, &result) != 0) {
        printf("getaddrinfo failed with errror \n");
        WSACleanup();
        exit(1);
    }

    for(ptr=result; ptr!= NULL; ptr=ptr->ai_next) {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if(ConnectSocket == INVALID_SOCKET) {
            printf("Socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            exit(1);
        }

        //connnect to server
        if(connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen)) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if(ConnectSocket == INVALID_SOCKET) {
        printf("unable to connect to server! \n");
        startServer(packets);
        WSACleanup();
        exit(1);
    }

    if(send(ConnectSocket, (char*)&packets, sizeof packets, 0) == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        exit(1);
    }

    int iResult;
    do {
        iResult = recv(ConnectSocket, (char*)&packets, sizeof packets, 0);
        if(iResult >0) {
            for(size_t i = 0; i<packets.players.size(); i++) {
                std::cout << packets.players[i] << std::endl;
            }
        } 
        else if (iResult == 0)
            printf("Connection closed\n");
        else    
            printf("recv failed with error: %d\n", WSAGetLastError());
    } while(iResult >0);

    closesocket(ConnectSocket);
    WSACleanup();
};



int main() {

    int playerid = rand()%1000;
    Player playerp(playerid);
    Packet myPacket;
    myPacket.players.push_back(&playerp); 


    startClient(myPacket);
    //startServer(myPacket);

    return 1;
};