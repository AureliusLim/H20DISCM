#include <iostream>
#include <ctime>
#include <string>
#include <WinSock2.h>
#include <windows.h>
#include <typeinfo>
#include <sstream>
#include <chrono>
#include <cmath>
#include <thread>
#include <mutex>
#include <vector>
#include <utility>
#include <unordered_map>
class Server {
private:
    SOCKET server_fd, new_socket;

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    

public:
    Server(int port) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            return;
        }

        // Creating socket file descriptor
        if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
            std::cerr << "Socket creation failed\n";
            WSACleanup();
            return;
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        // Binding socket to port
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
            std::cerr << "Bind failed\n";
            closesocket(server_fd);
            WSACleanup();
            return;
        }

        // Listening for connections
        if (listen(server_fd, 3) == SOCKET_ERROR) {
            std::cerr << "Listen failed\n";
            closesocket(server_fd);
            WSACleanup();
            return;
        }

        std::cout << "Server listening on port " << port << std::endl;


        std::cout << "Connection accepted\n";
    }

    ~Server() {
        closesocket(server_fd);
        WSACleanup();
    }


     SOCKET acceptConnection() {
            // Accepting incoming connection
            SOCKET new_socket;
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) == INVALID_SOCKET) {
                std::cerr << "Accept failed\n";
                return INVALID_SOCKET;
            }
            return new_socket;
        }
       std::pair<std::string, SOCKET> receiveMessageAny(const std::vector<SOCKET>& sockets) {
          char buffer[1024];
          fd_set readfds;
          FD_ZERO(&readfds);

          SOCKET max_sd = 0;
          for (const auto& socket : sockets) {
              FD_SET(socket, &readfds);
              if (socket > max_sd) {
                  max_sd = socket;
              }
          }

          // Wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
          int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

          if ((activity < 0) && (errno != EINTR)) {
              printf("select error");
          }

          for (const auto& socket : sockets) {
              if (FD_ISSET(socket, &readfds)) {
                  int valread = recv(socket, buffer, 1023, 0); // Adjusted buffer size for null terminator
                  if (valread > 0) {
                      buffer[valread] = '\0'; // Ensuring the string is null-terminated
                      return std::make_pair(std::string(buffer), socket);
                  }
              }
          }

          // No message received from any of the sockets, return "null" message and an invalid socket
          return std::make_pair(std::string("null"), INVALID_SOCKET);
        }
        std::string receiveMessage(SOCKET socket) {
            char buffer[1024] = {0};
            int valread = recv(socket, buffer, 1024, 0);
            return std::string(buffer);
        }


        void sendMessage(SOCKET socket, const std::string& message) {
            if (send(socket, message.c_str(), message.length(), 0) == SOCKET_ERROR) {
                std::cerr << "Send failed: " << WSAGetLastError() << "\n";
            }

        }
    std::tuple<int, int, int> isH2OReady(std::vector<std::tuple<std::string, int, bool>> H_vector, std::vector<std::tuple<std::string, int, bool>> O_vector) {
      int h1_index = -1;
      int h2_index = -1;
      int o1_index = -1;


      for (int i = 0; i < H_vector.size(); ++i) {
          if (!std::get<2>(H_vector[i])) {
              if(h1_index == -1){
                h1_index = i;
              } else{
                h2_index = i;
                break;
              }
          }
      }

      for(int i=0; i<O_vector.size(); ++i){
        if (!std::get<2>(O_vector[i])) {
          o1_index = i;
          break;
        }
      }
      return std::make_tuple(h1_index, h2_index, o1_index);
    }

};
void printVector(const std::vector<std::tuple<std::string, int, bool>>& vec, const std::string& label) {
    std::cout << "Contents of " << label << " vector:" << std::endl;
    for (const auto& elem : vec) {
        // Accessing elements of the tuple
        const std::string& str = std::get<0>(elem);
        int num = std::get<1>(elem);
        bool flag = std::get<2>(elem);

        std::cout << "String: " << str << ", Number: " << num << ", Boolean: " << (flag ? "true" : "false") << std::endl;
    }
}
int main(){
  // fix this to accept any socket connection and assign to respective socket?
  //maybe pwede just save socket without caring from who
  Server server(8080);
  std::string identifier;
  SOCKET hSocket = INVALID_SOCKET;
  SOCKET oSocket = INVALID_SOCKET;
  std::vector<SOCKET> sockets;

  SOCKET new_socket = server.acceptConnection();
  identifier = server.receiveMessage(new_socket);
  if(identifier == "HYDRO"){
    std::cout << "Hydro Client Connected" << std::endl;
    hSocket = new_socket;
    sockets.push_back(hSocket);
  }
  else{
    std::cout << "Oxy Client Connected" << std::endl;
    oSocket = new_socket;
    sockets.push_back(oSocket);
  }

  new_socket = server.acceptConnection();
  identifier = server.receiveMessage(new_socket);
  if(identifier == "HYDRO"){
    std::cout << "Hydro Client Connected";
    hSocket = new_socket;
    sockets.push_back(hSocket);
  }
  else{
    std::cout << "Oxy Client Connected";
    oSocket = new_socket;
    sockets.push_back(oSocket);
  }


  std::pair<std::string, SOCKET> message;
  bool hdone = false, odone = false;
  int bond_ctr = 0;

  std::vector<std::tuple<std::string, int, bool>> H_vector;
  std::vector<std::tuple<std::string, int, bool>> O_vector;

  int h1_index;
  int h2_index;
  int o1_index;
  bool hydroFirst = true;
  int received = 0;

  std::string endH = "H";
  std::string endO = "O";
  message = server.receiveMessageAny(sockets);
  int Hbonds, Obonds;
  //std::cout << "MESSAGE = " << message.first<< std::endl;
  if(message.second == hSocket){
    endH.append(message.first);
    Hbonds = stoi(message.first);
    std::cout << "Last Element = " << endH << std::endl;
    
  }
  else{
    hydroFirst = false;
    Obonds = stoi(message.first);
    endO.append(message.first);
    std::cout << "Last Element = " << endO << std::endl;
    
  }
 

  message = server.receiveMessageAny(sockets);
  //std::cout << "MESSAGE = " << message.first << std::endl;
  if(message.second == hSocket){
    endH.append(message.first);
    Hbonds = stoi(message.first);
    std::cout << "Last Element = " << endH << std::endl;
  }
  else{
    endO.append(message.first);
    Obonds = stoi(message.first);
    std::cout << "Last Element = " << endO << std::endl;
  }
 
  auto start_time = std::chrono::high_resolution_clock::now();
  server.sendMessage(hSocket, "START");
  server.sendMessage(oSocket, "START");


  
  std::vector<std::tuple<std::string, std::string, std::string>> serverlogs;

  bool bond = false;

  
  while(!hdone || !odone){
    //std::cout << hydroFirst << std::endl;
    bond = false;
    if(hydroFirst || odone){
      message = std::make_pair(server.receiveMessage(hSocket), hSocket);
      hydroFirst = !hydroFirst;
    
    }
    else if(!hydroFirst || hdone){
      message =  std::make_pair(server.receiveMessage(oSocket), oSocket);
      hydroFirst = !hydroFirst;
    
    }
    time_t now = time(0); // get current dat/time with respect to system.
    char* dt = ctime(&now); 
    
    //std::string logitem = message.first + ",request," + dt;
    serverlogs.emplace_back(std::make_tuple(message.first, "request", dt));

    
    if(message.first.substr(0, 1) == "H"){
     
      if(message.first == endH){
        hdone = true;
     
      }
      else {
        received = 1;
      }
      H_vector.push_back(std::make_tuple(message.first, stoi(message.first.substr(1,message.first.length()-1)), false));
      
    }
    else if(message.first.substr(0, 1) == "O"){
      if (message.first == endO){
        odone = true;
      
      }
      else {
        received = 2;
      }
      O_vector.push_back(std::make_tuple(message.first, stoi(message.first.substr(1,message.first.length()-1)), false));
      
    }
    if (H_vector.size() > 1 && O_vector.size() > 0) {
      std::tuple<int, int, int> indices = server.isH2OReady(H_vector, O_vector);
      h1_index = std::get<0>(indices);
      h2_index = std::get<1>(indices);
      o1_index = std::get<2>(indices);


      if(h1_index != -1 && h2_index != -1 && o1_index !=-1){
        std::get<2>(H_vector[h1_index]) = true;
        std::get<2>(H_vector[h2_index]) = true;
        std::get<2>(O_vector[o1_index]) = true;
        bond = true;
        time_t now = time(0); // get current dat/time with respect to system.
        char* dt = ctime(&now); 

        server.sendMessage(hSocket, std::get<0>(H_vector[h1_index]));
      
        Sleep(1);
        serverlogs.emplace_back(std::make_tuple(std::get<0>(H_vector[h1_index]), "bonded", dt));
        serverlogs.emplace_back(std::make_tuple(std::get<0>(H_vector[h2_index]), "bonded", dt));

        std::cout << std::get<0>(H_vector[h1_index]) << ", bonded, " << dt << std::endl;
        std::cout << std::get<0>(H_vector[h2_index]) << ", bonded, " << dt << std::endl;

        server.sendMessage(hSocket, std::get<0>(H_vector[h2_index]));
        
       
        serverlogs.emplace_back(std::make_tuple(std::get<0>(O_vector[o1_index]), "bonded", dt));
        server.sendMessage(oSocket, std::get<0>(O_vector[o1_index]));
        std::cout <<  std::get<0>(O_vector[o1_index]) << ", bonded, " << dt << std::endl;
        
        bond_ctr++;
      }

     }
   
    if (received == 1) {
      if(!bond){
            server.sendMessage(hSocket, " ");   
      }
      
    }
    else if (received == 2) {
       if(!bond){
            server.sendMessage(oSocket, " ");
      }
    }

   
    received = 0;
    // std::cout << "log" << std::endl;

  }
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);


  std::cout<< "Connection Closed" << std::endl;
  std::cout<< duration.count() << " Milliseconds" << std::endl;

  std::cout<< "Performing Sanity Check..." << std::endl;
  //sanity check
  std::unordered_map<std::string, int> moleculeStatus;
    int hydrogenRequestCount = 0;
    int oxygenRequestCount = 0;
    int errorCount = 0;
    bool h1Bonded, h2Bonded, oBonded;
    for (const auto& log : serverlogs) {
        std::string molecule = std::get<0>(log);
        std::string action = std::get<1>(log);

        int& status = moleculeStatus[molecule];

        if (action == "request") {
            if (status != 0) {
                // Error: Duplicate request
                std::cout << molecule << action << std::endl;
                ++errorCount;
                continue; // Skip further processing for this log entry
            }
            status = 1; // Mark as requested

            // Count the requests for hydrogen and oxygen separately
            if (molecule[0] == 'H') {
                ++hydrogenRequestCount;
            } else if (molecule[0] == 'O') {
                ++oxygenRequestCount;
            }
        } else if (action == "bonded") {
            if (status != 1) {
                // Error: Bonding before request or duplicate bonding
                ++errorCount;
                std::cout << molecule << action << std::endl;
                continue; // Skip further processing for this log entry
            }
            // Ensure there are enough molecules of each type for bonding
            if (hydrogenRequestCount >= 2 && oxygenRequestCount >= 1) {
                // A valid bond can occur, so proceed
                status = 2; // Mark as bonded
                // Deduct the used molecules from the count
                if(h1Bonded && h2Bonded && oBonded){
                  hydrogenRequestCount -= 2;
                  oxygenRequestCount -= 1;

                  h1Bonded = false;
                  h2Bonded = false;
                  oBonded = false;
                }
                else{
                  if(molecule[0] == 'H' && !h1Bonded){
                    h1Bonded = true;
                  }
                  else if(molecule[0] == 'H' && !h2Bonded){
                    h2Bonded = true;
                  }
                  else if(molecule[0] == 'O' && !oBonded){
                    oBonded = true;
                  }
                }
                
            } else {
                // Error: Insufficient molecules for a bond
                ++errorCount;
                std::cout << molecule << action << std::endl;
            }
        }
    }

    std::cout << "Number of errors: " << errorCount << std::endl;
  // printVector(H_vector, "H");
  // printVector(O_vector, "O");
 

}