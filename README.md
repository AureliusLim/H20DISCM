# Distributed H2O Simulation

This project consist of a server, a hydrogen client, and oxygen client. The client send bond requests to the server to simulate the proper bonding of H2O through socket programming in C++.

## Contributors
- Caoile, Sean
- Lim, Aurelius
- Tan, Gavin
- Yongco, Denzel

## Features
- **Performance Measurement**: Records the performance of the program from the second, the client sends the request up to the completion of the simulation
- **Socket Programming**: Features a server, hydrogen client, and oxygen client for synchronization of particles

## Setup
- In the project folder, open 3 separate terminals.
- Execute `./server.exe`  `./hclient.exe`  `./oclient.exe`
- When compiling, include the options `-lwsock32` `-std=c++11` 
