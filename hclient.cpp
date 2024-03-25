#include <iostream>
#include <ctime>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <string>
#include <utility>
#include <sstream>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <vector>
#pragma comment(lib, "ws2_32.lib")

class Client {
private:
    SOCKET sock;
    sockaddr_in server;

public:
    Client(const std::string& server_ip, int port) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << "\n";
            return;
        }

        // Create socket
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            std::cerr << "Could not create socket: " << WSAGetLastError() << "\n";
            WSACleanup();
            return;
        }

        server.sin_addr.s_addr = inet_addr(server_ip.c_str());
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        // Connect to remote server
        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
            std::cerr << "Connection failed: " << WSAGetLastError() << "\n";
            closesocket(sock);
            WSACleanup();
            return;
        }

        std::cout << "Connected\n";
    }

    ~Client() {
        closesocket(sock);
        WSACleanup();
    }

    void sendMessage(const std::string& message) {


        if (send(sock, message.c_str(), message.length(), 0) == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << "\n";
        }


    }
     std::string receiveMessage() {
        char buffer[256];
        int bytesReceived = recv(sock, buffer, 256, 0);
        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "Receive failed: " << WSAGetLastError() << "\n";
            return "";
        }
        buffer[bytesReceived] = '\0'; // Null-terminate the string
        return std::string(buffer);
    }

};

int main(){
  int N;
  int tobeBonded;
  std::vector<std::tuple<std::string, std::string, std::string>> hydrologs;
  Client client( "127.0.0.1", 8080);
  client.sendMessage("HYDRO");
  std::cout << "Integer for Hydrogen:";
  std::cin >> N;
  tobeBonded = N;
  client.sendMessage(std::to_string(N));
  std::cout << "sent " << N <<std::endl;
  std::string goSignal = client.receiveMessage();
  if (goSignal == "START") {

    for (int i = 1; i <= N; i++){
    
        std::string message="H";
        message.append(std::to_string(i));
        client.sendMessage(message);

        //print the request regardless
        time_t now = time(0); // get current dat/time with respect to system.
        char* dt = ctime(&now); 
        std::cout << message << ", request, "<< dt << std::endl;
        hydrologs.emplace_back(std::make_tuple(message, "request", dt));
      

        //listen for server message
        std::string serverMessage = client.receiveMessage(); //this can be go signal or bond
        now = time(0); // get current dat/time with respect to system.
        dt = ctime(&now); 
        if(serverMessage.at(0) == 'H'){
            std::string serverMessage1 = client.receiveMessage();
            
            std::cout << serverMessage << ", bonded, " << dt << std::endl;
             hydrologs.emplace_back(std::make_tuple(serverMessage, "bonded", dt));
            std::cout << serverMessage1 << ", bonded, " << dt << std::endl;
             hydrologs.emplace_back(std::make_tuple(serverMessage1, "bonded", dt));
            tobeBonded -= 2;
        }

    }

    while(tobeBonded >= 1){
        std::string serverResponse = client.receiveMessage();
        time_t now = time(0); // get current dat/time with respect to system.
        char* dt = ctime(&now); // convert it into string.
        if(serverResponse.at(0) == 'H'){
            std::cout << serverResponse << ", bonded, " << dt << std::endl;
            hydrologs.emplace_back(std::make_tuple(serverResponse, "bonded", dt));
            tobeBonded -= 2;
        
        }
  
    }
    std::cout<< "Performing Sanity Check..." << std::endl;    
    //perform sanity check
    std::unordered_map<std::string, int> moleculeStatus;
    int errorCount = 0;
    for (const auto& log : hydrologs) {
        std::string molecule = std::get<0>(log);
        std::string action = std::get<1>(log);

        // Encoding statuses as integers: 0 = unknown, 1 = requested, 2 = bonded
        int& status = moleculeStatus[molecule];

        if (action == "request") {
            if (status == 0) {
                status = 1; // Requested
            } else {
                // Error: Duplicate request or request after bonding
                ++errorCount;
            }
        } else if (action == "bonded") {
            if (status == 1) {
                status = 2; // Bonded
            } else {
                // Error: Bonding before request or duplicate bonding
                ++errorCount;
            }
        }
    }
    std::cout<< "Found " + std::to_string(errorCount) + " errors" << std::endl;
  }
}