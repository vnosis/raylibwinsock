#define PLATFORM_WINDOWS 1
#define PLATFORM_MAC     2
#define PLATFORM_UNIX    3


//To make raylib work with winsock
#if defined(_WIN32)
    #define PLATFORM PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOUSER
    #undef near
    #undef far
#elif defined(__APPLE__)
    #define PLATFORM PLATFORM_MAC
#else 
    #define PLATFORM PLATFORM_UNIX
#endif

#if PLATFORM == PLATFORM_WINDOWS
    #include <Windows.h>
    #include <ws2tcpip.h>

    // Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
    #pragma comment (lib, "Ws2_32.lib")
    #pragma comment (lib, "Mswsock.lib")
    #pragma comment (lib, "AdvApi32.lib")

#elif PLATFORM == PLATFORM_MAC || 
      PLATOFRM == PLATFORM_UNIX
      #include <sys/socket.h>
      #include <netinet/in.h>
      #include <fcntl.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include <iostream>
#include <cstdio>
#include <random>
#include <string.h>
#include <algorithm>
#include <cstring>
#include <thread>
#include <memory>

#define DEFAULT_PORT "8083"
#define IPADDRESS "127.0.0.1"
#define DEFAULT_BUFLEN 1024 //MAX MTU

struct Ball {
    Vector2 ballPosition;
    Vector2 ballSpeed;
    int ballRadius;
};

struct Player{
    int id;
    int x;
    int y;
    int radius;
    Player(int pid):id(pid){};
    Ball balls;
};

enum Connection {
   Success = 0,
   Error, 
};

typedef struct{
    std::vector<Player> players;
} Packet;

namespace NETWORK{
    std::vector<char> serializePacket(Packet);
    Packet desializePacket(const char*, size_t);
    class Server{
        private:
            WSADATA wsaData;
            SOCKET ListenSocket     = INVALID_SOCKET;
            SOCKET ClientSocket     = INVALID_SOCKET;
            struct addrinfo* result = NULL;
            struct addrinfo hints;
            char recvbuf[DEFAULT_BUFLEN];
        public:
            //Constructor
            Server() = default;
            int iResult, iSendResult;

            //Init WSAInit
            bool WSAInit();
            //Initialize hints addrinfo
            void HintsInit();
            //Initialize result 
            bool GetAddrInit();
            // Create a socket
            bool SocketInit();
            // bind socket
            bool BindSocketInit();
            //Listen on Socket
            bool ListenInit();
            //Accept client
            bool AcceptClient();
            //Call all server methods above^^
            bool InitServer();
            //Send
            bool Send(std::vector<char>);
            //receive 
            bool Receive();
            //shutdown
            bool ShutdownInit();
            
    };

    class Client {
        private:
            WSADATA m_wsaData;
            SOCKET ConnectSocket = INVALID_SOCKET;
            struct addrinfo *result = NULL,
                            *ptr = NULL,
                            hints;
            char recvbuf[DEFAULT_BUFLEN];
        public:

            Client() = default;
            int iResult;
            //Initialize Winsock
            bool WSAInit();
            //Initialize hints addrinfo
            void HintsInit();
            //Resolve the server address&port
            bool GetaddrInit();
            //Attempt to connect to an address until one succeeds
            bool SocketConnectInit();
            //Init Client with above methods^^
            bool InitClient();
            // Send buffer packet
            bool Send(std::vector<char>);
            // Receive unserialized buffer
            bool Receive();
            //Shutdown connectoin
            bool ShutDownInit();
    };
    

    bool Client::WSAInit()
    {
        this->iResult = WSAStartup(MAKEWORD(2,2), &m_wsaData);
        if(this->iResult != 0){
            printf("WSAStartup failed with error: %d\n", this->iResult);
            return false;
        }
        return true;
    }
    void Client::HintsInit()
    {
        ZeroMemory(&this->hints, sizeof this->hints);
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
    }
    bool Client::GetaddrInit()
    {
        this->iResult = getaddrinfo(IPADDRESS, DEFAULT_PORT, &this->hints,&this->result);
        if(iResult != 0) {
            printf("getaddrinfo failed with error: %d\n", this->iResult);
            WSACleanup();
            return false;
        }
        return true;
    }
    bool Client::SocketConnectInit()
    {
        //Attempt to connect to an address until one succeeds
        for(ptr=result; ptr != NULL; ptr = ptr->ai_next){
            //Create a socket for connecting to server
            this->ConnectSocket = socket(ptr->ai_family,
                                         ptr->ai_socktype, 
                                         ptr->ai_protocol);

            if (this->ConnectSocket == INVALID_SOCKET) {
                printf("socket failed with error: %d\n", WSAGetLastError());
                WSACleanup();
                return false;
            }
            //Connect to server
            iResult = connect(this->ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
            if(iResult == SOCKET_ERROR) {
                closesocket(this->ConnectSocket);
                ConnectSocket = INVALID_SOCKET;
                continue;
            }
            break;
        }
        freeaddrinfo(this->result);

        if(this->ConnectSocket == INVALID_SOCKET) {
            printf("Unable to connecto to server!\n");
            WSACleanup();
            return false;
        }
        return true;
    }
    bool Client::InitClient()
    {
        if(!this->WSAInit()) {
            std::cout << "WSAInit() error";
            return false;
        }
        HintsInit();
        if(!this->GetaddrInit()){
            std::cout << "GetaddrInit() error\n";
            return false;
        }
        if(!this->SocketConnectInit()) {
            std::cout << "SocketConnectionInit() error\n";
            return false;
        }
        return true;
    }
    bool Client::Send(std::vector<char> packet)
    {
        this->iResult = send(this->ConnectSocket, packet.data(), sizeof packet, 0);
        if( this->iResult ==  SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(this->ConnectSocket);
            WSACleanup();
            return false;
        }
        return true;
    }
    bool Client::Receive()
    {
        this->iResult = recv(this->ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if(this->iResult > 0) {
            printf("Bytes received: %d\n", this->iResult);
            return true;
        }
        else if(this->iResult == 0) {
            printf("Connection closed\n");
            return false;
        }
        else{
            printf("recv failed with error: %d\n", WSAGetLastError());
            return false;
        }
    }
    bool Client::ShutDownInit()
    {
        this->iResult = shutdown(this->ConnectSocket, SD_SEND);
        if(iResult == SOCKET_ERROR) {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(this->ConnectSocket);
            WSACleanup();
            return false;
        }
        return true;
    }
    std::vector<char> serializePacket(Packet packet)
    {
        std::vector<char> buffer;

        size_t numPlayers = packet.players.size();
        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&numPlayers),reinterpret_cast<const char*>(&numPlayers) + sizeof(numPlayers));

        for(const Player& player: packet.players) {
            buffer.insert(buffer.end(), reinterpret_cast<const char*>(&player), reinterpret_cast<const char*>(&player) + sizeof(Player));
        }

        return buffer;
    }
    Packet desializePacket(const char *data, size_t datasize)
    {
        Packet packet;
        size_t offset = 0;
        size_t numPlayers = 0;

        std::memcpy(&numPlayers,data + offset, sizeof(numPlayers));
        offset += sizeof(numPlayers);

        for(size_t i = 0; i < numPlayers; ++i) {
            Player player(0);

            std::memcpy(&player, data + offset, sizeof(Player));
            offset += sizeof(Player);
            packet.players.push_back(player);
        }
        return packet;
    }
    bool Server::WSAInit()
    {
        this->iResult = WSAStartup(MAKEWORD(2,2), &this->wsaData);
        if(iResult != 0) {
            printf("WSAStartup failed with error: %d\n", this->iResult);
            return false;
        }
        return true;
    }
    void Server::HintsInit()
    {
        ZeroMemory(&this->hints, sizeof hints);
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags    = AI_PASSIVE;
    }
    bool Server::GetAddrInit()
    {
        this->iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
        if (iResult != 0) {
            printf("getaddrinfo failed with error: %d\n", iResult);
            WSACleanup();
            return false;
        }
        return true;
    }
    bool Server::SocketInit()
    {
        this->ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if(ListenSocket ==INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            freeaddrinfo(result);
            WSACleanup();
            return false;
        }
        return true;
    }
    bool Server::BindSocketInit()
    {
        this->iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if(iResult == SOCKET_ERROR) {
            printf("bind failed with error: %d\n", WSAGetLastError());
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            return false;
        }
        return true;
    }
    bool Server::ListenInit()
    {
        this->iResult = listen(ListenSocket, SOMAXCONN);
        if(iResult == SOCKET_ERROR) {
            printf("Listen failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return false;
        }
        return true;
    }
    bool Server::AcceptClient()
    {
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if(ClientSocket == INVALID_SOCKET) {
            printf("Client is an INVALID_SOCKET: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return false;
        }
        //No longer need server socket if one player is connecting 
        //Delete if i decide to have more players join
        closesocket(ListenSocket);
        return true;
    }
    bool Server::InitServer()
    {
        if (!this->WSAInit()) {
            std::cout << "WSAInit() error\n";
            return false;
        }
        HintsInit();
        if (!this->GetAddrInit()) {
            std::cout << "GetAddrInit() error\n";
            return false;
        }
        if(!this->SocketInit()) {
            std::cout << "SocketInit() error\n";
            return false;
        }
        if(!this->BindSocketInit()) {
            std::cout << "BindSocketInit() error\n";
            return false;
        }
        if(!this->ListenInit()) {
            std::cout << "ListenInit() error:\n";
            return false;
        }
        if(!this->AcceptClient()) {
            std::cout << "ListenSocket() error\n";
            return false;
        }
        std::cout << "Server Accepted Client \n";
        
        return true;
    }
    bool Server::Send(std::vector<char> recvbuffer)
    {
        this->iSendResult = send(ClientSocket, recvbuffer.data(), sizeof recvbuffer, 0);
        if(iSendResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return false;
        }
        return true;
    }
    bool Server::Receive()
    {
        iResult = recv(this->ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            printf("Received Cliento info");
            return true;
        } 
        else if( iResult == 0) {
            printf("Connection closing\n");
            return false;
        }
        else {
            printf("Receive had an error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return false;
        }
    }
    bool Server::ShutdownInit()
    {
        iResult = shutdown(ClientSocket, SD_SEND);
        if(iResult == SOCKET_ERROR) {
            printf("Shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return false;
        }
        //cleanup
        if(ListenSocket != INVALID_SOCKET) {
            closesocket(ListenSocket);
        }
        closesocket(ClientSocket);
        WSACleanup();
        return true;
    }
};



void startServer() {
    try{

    std::cout <<"Starting Server" << std::endl;
    WSADATA wsaData;
    int iResult;

    char recvBuffer[1024];

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return;
    }

    // Accept a client socket
    std::cout << "ABout to Accept\n";
    ClientSocket = accept(ListenSocket, NULL, NULL);
    std::cout << "Accepting Client... \n";
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return;
    }

    // No longer need server socket
    closesocket(ListenSocket);

    // Receive until the peer shuts down the connection
    /*
    do {

        iResult = recv(ClientSocket, recvBuffer, sizeof recvBuffer, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);

            packets = deserializePacket(recvBuffer,iResult);

            for(size_t i = 0; i < packets.players.size(); ++i) {
                std::cout << packets.players[i].id << std::endl;
                std::cout << packets.players[i].y << std::endl;
                std::cout << packets.players[i].x << std::endl;
            }
        // Echo the buffer back to the sender
            std::vector<char> recvecBuffer = serializePacket(packets);
            iSendResult = send( ClientSocket, recvecBuffer.data(), sizeof recvecBuffer, 0 );
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes sent: %d\n", iSendResult);
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else  {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

    } while (iResult > 0);
    */

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();
    } catch(const std::exception& e) {
        std::cerr << "ServerThread Exception: " << e.what() << "\n";
    }

};

int startClient() {
    Packet playerpacket;
    Player client(rand()%100);
    client.radius = 50;
    client.x = 50;
    client.y = 50;
    playerpacket.players.push_back(client);

    char recvBuffer[1024];
    
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    int iResult;
    
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(IPADDRESS, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %d\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    std::vector<char> recvecBuffer = NETWORK::serializePacket(playerpacket);
    iResult = send( ConnectSocket, recvecBuffer.data(), sizeof recvecBuffer, 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %d\n", iResult);

    // shutdown the connection since no more data will be sent
    /*
     iResult = shutdown(ConnectSocket, SD_SEND);
     if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
     }
    */
   

    // Receive until the peer closes the connection
    Packet playerpackets;
    /*
    do {
        std::cout << "here" << std::endl;
        iResult = recv(ConnectSocket, recvBuffer, sizeof(recvBuffer), 0);
        if ( iResult > 0 ) {
            std::cout << "helloo " << std::endl;
            printf("Bytes received: %d\n", iResult);
            playerpackets = deserializePacket(recvBuffer, iResult);
            for(size_t i = 0; i < playerpackets.players.size(); i++) {
                std::cout << playerpackets.players[i].id << " id\n";
            }
        }
       else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while( iResult > 0 );
    */
    

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
};

//Create Menu Window
void Menu() {
    DrawRectangle(GetScreenWidth()/2.0f, GetScreenHeight()/2.0f, 60,20, BLUE);
    DrawRectangle(GetScreenWidth()/5.0f, GetScreenHeight()/2.0f, 60,20, RED);
    DrawText("Server", GetScreenWidth()/2.0f, GetScreenHeight()/2.0f, 20, RED);
    DrawText("Client", GetScreenWidth()/5.0f, GetScreenHeight()/2.0f, 20, BLUE);
};

//TODO 
//Close window function and close connection

// Pong movement
void Pong_Ball(Vector2& ballPosition, Vector2& ballSpeed, int& ballRadius) {
    ballPosition.x += ballSpeed.x;
    ballPosition.y += ballSpeed.y;

    if ((ballPosition.x >= (GetScreenWidth() - ballRadius)) || (ballPosition.x <= ballRadius))
        ballSpeed.x *= -1.0f;
    if ((ballPosition.y >= (GetScreenHeight() - ballRadius)) || (ballPosition.y <= ballRadius))
        ballSpeed.y *= -1.0f;
};

struct ServerThread {
    static std::thread startServerThread;
    void start(){
        std::cout << "Thread" << std::endl;
        std::thread startServerThread(startServer);
        std::cout << "Thread END" << std::endl;
    }
    void finish(){
        std::cout << "Finish" << std::endl;
        startServerThread.join();
        std::cout << "Finished" << std::endl;
    }
    
};


int main() {
    const int screenWidth  = 800;
    const int screenHeight = 450;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Pong Test");
    Vector2 ballPosition = {GetScreenWidth()/2.0f, GetScreenHeight()/2.0f};
    Vector2 ballSpeed    = { 5.0f, 4.0f };
    int ballRadius       = 20;

    //Mouse
    Vector2 mousePoint = {0.0f, 0.0f};

    //button
    Rectangle btnBoundServer = {GetScreenWidth()/2.0f, GetScreenHeight()/2.0f, 60,20};
    Rectangle btnBoundClient = {GetScreenWidth()/5.0f, GetScreenHeight()/2.0f, 60,20};
    
    SetTargetFPS(60);

    //Menu 
    bool menuWindow       = true;
    bool lookingForServer = false;
    bool waitingForClient = false;
    bool clientWindow = false, serverWindow = false;
    std::thread serverThread;

    //Scoped shared_ptr<>
    std::shared_ptr<NETWORK::Server> serv;
    std::shared_ptr<NETWORK::Client> clnt;

    while(menuWindow) {
        mousePoint = GetMousePosition();

        if(CheckCollisionPointRec(mousePoint, btnBoundServer)) {
            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                std::shared_ptr<NETWORK::Server> server(new NETWORK::Server());
                server->InitServer();
                serv = std::move(server);
            }
            
        }
        if(CheckCollisionPointRec(mousePoint, btnBoundClient)) {
            if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                std::shared_ptr<NETWORK::Client> client(new NETWORK::Client());
                client->InitClient();    
                clnt = std::move(client);
            }
        }

        BeginDrawing();
        Menu();
        DrawFPS(10,10);
        ClearBackground(RAYWHITE);
        Pong_Ball(ballPosition, ballSpeed, ballRadius);
        DrawCircleV(ballPosition, (float)ballRadius, MAROON);
        EndDrawing();
    }

    return 0;
      
};