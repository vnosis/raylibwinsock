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
#define IPADDRESS "192.168.1.222"

#define DEFAULT_BUFLEN 512 //MAX MTU


//Template array for balls and players
template<typename T, int N>
class Array {
    private:
        T elem[N];
        std::vector<T> vElem{};
    public:
        Array() = default;

        //Template copy 
        template<typename T, int N>
        Array<T,N>& operator=(const Array<T,N>& other) {
            this->vElem.clear();
            this->vElem.insert(this->vElem.begin(), other.vElem.begin(), other.vElem.end());
            return *this;
        }
        int getsize() const;

};

template<typename T, int N>
int Array<T, N>::getsize() const {
    return N;
}


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

//Network wrapper for winsock
namespace NETWORK{
    std::vector<char> serializePacket(Packet*);
    Packet desializePacket(const char*, size_t);
    class Server{
        private:
            WSADATA wsaData;
            SOCKET ListenSocket     = INVALID_SOCKET;
            SOCKET ClientSocket     = INVALID_SOCKET;
            struct addrinfo* result = NULL;
            struct addrinfo hints;
        public:
            //Constructor
            Server() = default;
            int iResult, iSendResult;
            char recvbuf[DEFAULT_BUFLEN];

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
        public:
            Client() = default;
            int iResult;
            char recvbuf[DEFAULT_BUFLEN];

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
        this->iResult = send(this->ConnectSocket, packet.data(), DEFAULT_BUFLEN, 0);
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
        std::cout << "Shutting Down Client Sockets" << std::endl;
        return true;
    }
    std::vector<char> serializePacket(Packet* packet)
    {
        std::vector<char> buffer;

        size_t numPlayers = packet->players.size();

        buffer.insert(buffer.end(), reinterpret_cast<const char*>(&numPlayers),reinterpret_cast<const char*>(&numPlayers) + sizeof(numPlayers));

        for(const Player& player: packet->players) {
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
    //Might need to fix this for 10093 error that pops up every now and then. 
    //Sometimes a netsh winsock reset fixes the issue. 
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
        //closesocket(ListenSocket);
        
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
        this->iSendResult = send(ClientSocket, recvbuffer.data(), DEFAULT_BUFLEN, 0);
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
        std::cout << "Shutting Down Server Sockets" << std::endl;
        return true;
    }
};

//Create Menu Window

//Close window function and close connection
void CheckWindow(bool& userwindow) {
    if(WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)){
        userwindow = !userwindow;
    }
};

// Pong movement
void Pong_Ball(Vector2& ballPosition, Vector2& ballSpeed, int& ballRadius, int& playerX) {
    ballPosition.x += ballSpeed.x;
    ballPosition.y += ballSpeed.y;

    if ((ballPosition.x >= (GetScreenWidth() - ballRadius)) || (ballPosition.x <= ballRadius))
        ballSpeed.x *= -1.0f;
    if ((ballPosition.y >= (GetScreenHeight() - ballRadius)) || (ballPosition.y <= ballRadius))
        ballSpeed.y *= -1.0f;
};

void PlayerMovement(int& y) {
    if(IsKeyDown(KEY_S)) y += 3.0f;
    if(IsKeyDown(KEY_W)) y -= 3.0f;
};

//Have users reading from config file
//not sure if i should make this into  
class Menu{
    private:
        Vector2 ballPosition = {GetScreenWidth()/2.0f, GetScreenHeight()/2.0f};
        Vector2 ballSpeed    = { 5.0f, 4.0f };

    public:
        Menu() = default;

        //Disallow heap allocation 
        void* operator new(size_t) = delete;
        void operator delete(void*) = delete;
        void* operator new[](size_t) = delete;
        void operator delete[](void*) = delete;

        void Draw_Menu() {
            DrawRectangle(GetScreenWidth()/2.0f, GetScreenHeight()/2.0f, 60,20, BLUE);
            DrawRectangle(GetScreenWidth()/5.0f, GetScreenHeight()/2.0f, 60,20, RED);
            DrawText("Server", GetScreenWidth()/2.0f, GetScreenHeight()/2.0f, 20, RED);
            DrawText("Client", GetScreenWidth()/5.0f, GetScreenHeight()/2.0f, 20, BLUE);
        }
};

class Lobby {
    private:
    public:
};


class GameState{
    private:
    public:
    GameState() = default;
};

int main() {
    const int screenWidth  = 800;
    const int screenHeight = 450;
    int playerInitial_Y = screenHeight/2;

    std::shared_ptr<Packet> serverPacket(new Packet());
    std::shared_ptr<Packet> clientPacket(new Packet());
    std::shared_ptr<Player> serverPlayer;

    std::shared_ptr<Player> clientPlayer;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Pong Test");
    
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

    bool serverInit = false;
    bool clientInit = false;
    int playerInitial_X = 25; 
    int clientInitial_X = GetScreenWidth() - 20;

    while(menuWindow) {
        mousePoint = GetMousePosition();
        CheckWindow(menuWindow);

        if(CheckCollisionPointRec(mousePoint, btnBoundServer)) {
            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                serverWindow = true;
                menuWindow = false;
                std::shared_ptr<NETWORK::Server> server(std::make_shared<NETWORK::Server>());
                std::shared_ptr<Player> initServPlayer(std::make_shared<Player>(rand()%1000));
                if(server->InitServer()) serverInit = true;
                serv = std::move(server);
                serverPlayer = std::move(initServPlayer);
            }
        }
        if(CheckCollisionPointRec(mousePoint, btnBoundClient)) {
            if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                clientWindow = true;
                menuWindow = false;
                std::shared_ptr<NETWORK::Client> client(new NETWORK::Client());
                std::shared_ptr<Player> initClntPlayer(std::make_shared<Player>(rand()%1000));
                if(client->InitClient()) clientInit = true;
                clnt = std::move(client);
                clientPlayer = std::move(initClntPlayer);
            }
        }
        BeginDrawing();
        PlayerMovement(playerInitial_Y);
        // DrawRectangle(playerInitial_X, playerInitial_Y,20,50,RED);
        Menu();
        DrawFPS(10,10);
        ClearBackground(RAYWHITE);
        Pong_Ball(ballPosition, ballSpeed, ballRadius, playerInitial_X);
        DrawCircleV(ballPosition, (float)ballRadius, MAROON);
        EndDrawing();
    }

    if (serverWindow) {
        serverPacket->players.push_back(*serverPlayer);
        serverPacket->players[0].balls.ballPosition = ballPosition;
        serverPacket->players[0].balls.ballRadius   = ballRadius;
        serverPacket->players[0].balls.ballSpeed    = ballSpeed;
        serverPacket->players[0].x = playerInitial_X;
        serverPacket->players[0].y = playerInitial_Y;

        std::cout << serverPacket->players[0].balls.ballPosition.x << std::endl; 
        if(!serv->Receive()) std::cout << "Waiting for initial talk\n";
        Packet recvPacket = NETWORK::desializePacket(serv->recvbuf, sizeof serv->recvbuf);
        for(size_t i = 0; i < recvPacket.players.size(); ++i) {
            std::cout << "Player id " << recvPacket.players[i].id << "\n";
        }
    }

    if(clientWindow) {
        clientPacket->players.push_back(*clientPlayer);
        clientPacket->players[0].x = clientInitial_X;
        clientPacket->players[0].y = playerInitial_Y;

        std::cout << "Sending Client id" << clientPlayer->id << "\n";
        std::vector<char> serialPacket = NETWORK::serializePacket(clientPacket.get());
        if(!clnt->Send(serialPacket)) std::cout << "Couldn't send Client Packet\n";
    }
    
    while(serverWindow) {
        CheckWindow(serverWindow);
        BeginDrawing();
        DrawFPS(10,10);
        ClearBackground(RAYWHITE);

        Pong_Ball(serverPacket->players[0].balls.ballPosition, 
                  serverPacket->players[0].balls.ballSpeed, 
                  serverPacket->players[0].balls.ballRadius, playerInitial_X);
        std::vector<char> serialPacket = NETWORK::serializePacket(serverPacket.get());
        if(!serv->Send(serialPacket)) {
            serv->ShutdownInit();
            serverInit = false;
            serverWindow = false;
        }
        DrawCircleV(serverPacket->players[0].balls.ballPosition, 
                    (float)serverPacket->players[0].balls.ballRadius,
                    MAROON);
        
        PlayerMovement(serverPacket->players[0].y);

        DrawRectangle(serverPacket->players[0].x, 
                      serverPacket->players[0].y,
                      20,
                      50,
                      RED);

        serv->Receive();
        Packet recvPacket = NETWORK::desializePacket(serv->recvbuf, sizeof serv->recvbuf);
        std::cout << "Size of player vector " << recvPacket.players.size() << "\n";
        if(recvPacket.players.size()>0){
            for(size_t i = 0; i < recvPacket.players.size(); ++i) {
                std::cout << recvPacket.players[i].id << std::endl;
                std::cout << recvPacket.players[i].balls.ballPosition.x << " " << recvPacket.players[i].balls.ballPosition.y << " Ballposition\n";
                std::cout << recvPacket.players[i].x << " " << recvPacket.players[i].y << " Ballposition\n";
                DrawRectangle(recvPacket.players[i].x,
                              recvPacket.players[i].y,
                              20,
                              50,
                              RED);
            }
        }

        std::cout << serverPacket->players[0].balls.ballPosition.x << " " << serverPacket->players[0].balls.ballPosition.y << " Ballposition\n";
        //DrawCircleV(ballPosition, (float)ballRadius, MAROON);
        EndDrawing();
    }
    while(clientWindow) {
        CheckWindow(clientWindow);
        BeginDrawing();
        PlayerMovement(clientPacket->players[0].y);
        DrawFPS(10,10);
        ClearBackground(RAYWHITE);
        // Pong_Ball(ballPosition, ballSpeed, ballRadius);
        clnt->Receive();
        Packet recvPacket = NETWORK::desializePacket(clnt->recvbuf, sizeof clnt->recvbuf);
        std::cout << "Size of player vector " << recvPacket.players.size() << "\n";
        DrawRectangle(clientPacket->players[0].x,
                      clientPacket->players[0].y,
                      20,
                      50,
                      RED);
        if(recvPacket.players.size()>0){
            for(size_t i = 0; i < recvPacket.players.size(); ++i) {
                std::cout << recvPacket.players[i].id << std::endl;
                std::cout << recvPacket.players[i].balls.ballPosition.x << " " << recvPacket.players[i].balls.ballPosition.y << " Ballposition\n";
                std::cout << recvPacket.players[i].x << " " << recvPacket.players[i].y << " Ballposition\n";
                DrawCircleV(recvPacket.players[i].balls.ballPosition,
                           (float)recvPacket.players[i].balls.ballRadius,
                           MAROON);

                DrawRectangle(recvPacket.players[i].x, 
                            recvPacket.players[i].y,
                            20,
                            50,
                            RED);
            }
        }
        
        std::vector<char> serialClntPacket = NETWORK::serializePacket(clientPacket.get());
        if(!clnt->Send(serialClntPacket)) {
            clnt->ShutDownInit();
            clientInit = false;
            clientWindow = false;
        };
        EndDrawing();
    }

    // if(serverInit) serv->ShutdownInit();
    // if(clientInit) clnt->ShutDownInit();
    CloseWindow();
        
    return 0;
      
};