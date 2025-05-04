# Documentation

## Project Overview

This project is a simple chat application using the ENet library in C++ to demonstrate reliable peer-to-peer networking. It consists of two main components:

 - Client: Connects to the server, sends and receives chat messages, and displays them using an ncurses-based interface.
 - Server: Listens for client connections, manages connected peers, receives messages from clients, and broadcasts them to all other connected clients.

The project uses:
- ENet: a lightweight reliable UDP networking library.
- ncurses: for terminal-based UI in the client.

Directory structure:
```plaintext
client/
  client.cpp
  chat_screen.cpp
  chat_screen.hpp
server/
  server.cpp
readme.md: build instructions
```

Quick start:
```bash
g++ -lncurses -lpthread client/chat_screen.cpp client/client.cpp -o client -lenet
g++ server/server.cpp -o server -lenet
``` 

## ENet Library Introduction

ENet is a C library that provides a simple yet powerful way to build reliable communication on top of UDP. It combines the performance of UDP with features normally found in TCP, making it ideal for game networking and real-time applications.

### Why ENet?
- **Reliability**: Lost packets are automatically resent.
- **Ordering**: Messages can be sent in order on specific channels.
- **Low Latency**: Uses UDP under the hood, avoiding TCP head-of-line blocking.

### Core Concepts
- **ENetHost**: Represents a local host (server or client) that can manage multiple connections (peers).
- **ENetPeer**: A remote endpoint you're connected to or listening for.
- **ENetPacket**: A packet of data you send or receive (reliable or unreliable).
- **ENetEvent**: Represents events like connect, receive, or disconnect.
- **Channels**: Logical streams (numbered) on a peer for ordered delivery.

### Basic Workflow
1. **Initialize ENet**:
   ```cpp
   if (enet_initialize() != 0) {
       std::cerr << "Failed to initialize ENet" << std::endl;
       return;
   }
   ```
2. **Create a Host**:
   - **Server**:
     ```cpp
     ENetAddress address;
     enet_address_set_host(&address, "0.0.0.0");
     address.port = 1234;
     ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);
     ```
   - **Client**:
     ```cpp
     ENetHost* client = enet_host_create(NULL, 1, 2, 0, 0);
     ```
3. **Connect (Client-side)**:
   ```cpp
   ENetAddress address;
   enet_address_set_host(&address, "127.0.0.1");
   address.port = 1234;
   ENetPeer* peer = enet_host_connect(client, &address, 2, 0);
   ```
4. **Service Events**:
   ```cpp
   ENetEvent event;
   while (enet_host_service(host, &event, 1000) > 0) {
       switch (event.type) {
           case ENET_EVENT_TYPE_CONNECT:
               std::cout << "Connection established" << std::endl;
               break;
           case ENET_EVENT_TYPE_RECEIVE:
               std::cout << "Received packet: " << (char*)event.packet->data << std::endl;
               enet_packet_destroy(event.packet);
               break;
           case ENET_EVENT_TYPE_DISCONNECT:
               std::cout << "Peer disconnected" << std::endl;
               break;
       }
   }
   ```
5. **Send Packets**:
   ```cpp
   const char* message = "Hello, ENet!";
   ENetPacket* packet = enet_packet_create(message,
                             strlen(message) + 1,
                             ENET_PACKET_FLAG_RELIABLE);
   enet_peer_send(peer, 0, packet);
   enet_host_flush(host);
   ```
6. **Cleanup**:
   ```cpp
   enet_host_destroy(host);
   enet_deinitialize();
   ```

This should give you a solid foundation for using ENet in your C++ projects. Next, we will cover how to set up the library and compile the example chat application.

## Setup Instructions

### Prerequisites
- ENet library:
  - macOS: `brew install enet`
  - Ubuntu/Debian: `sudo apt-get install libenet-dev`
- ncurses:
  - macOS: `brew install ncurses`
  - Ubuntu/Debian: `sudo apt-get install libncurses-dev`
- POSIX Threads: included by default on most systems.
- C++ compiler (g++ or clang++).

### Building the Chat Application
1. **Compile the client**:
   ```bash
   g++ -std=c++11 client/chat_screen.cpp client/client.cpp -o client -lenet -lncurses -lpthread
   ```
2. **Compile the server**:
   ```bash
   g++ -std=c++11 server/server.cpp -o server -lenet
   ```

### Configuring the Connection
By default, the client connects to `192.168.1.83:7777`. Edit the `enet_address_set_host` and `address.port` lines in `client.cpp` to match your server's IP and port.

### Running
1. Start the server:
   ```bash
   ./server
   ```
2. Start one or more clients:
   ```bash
   ./client
   ```
Enter a username and begin chatting!

## Client Code Structure

The client-side code is located in the `client/` directory and is responsible for the user interface, network communication, and message handling. It consists of three main files:

1. **chat_screen.hpp & chat_screen.cpp**
   - Implements the `ChatScreen` class using ncurses to display chat history and an input box.
   - `Init()`: Sets up ncurses windows for the main chat display and input field.
   - `PostMessage(const char* sender, const char* message)`: Adds a new line to the chat window with the username and message.
   - `CheckBoxInput()`: Captures user keystrokes in the input box and returns the entered string when Enter is pressed.

2. **client/client.cpp**
   - **Includes & Globals**: `<enet/enet.h>`, `<bits/stdc++.h>`, pthreads, and a global `ChatScreen` instance.
   - `sendPacket(ENetPeer* peer, const char* msg)`: Helper to create and send a reliable ENet packet on channel 0.
   - **ClientData class**: Stores each peer's ID and username for display.
   - `parseData(unsigned char* data)`: Parses incoming packets of the form `type|id|payload`:
     - `type=1`: Chat message—extracts sender ID and text, then calls `PostMessage`.
     - `type=2`: New user joined—registers the username in `clientMap`.
     - `type=3`: Server assignment of `CLIENT_ID`.
   - `msgLoop(void* arg)`: Runs in a separate thread, continuously polling `enet_host_service` and dispatching `ENET_EVENT_TYPE_RECEIVE` to `parseData`.
   - `main()`: High-level flow:
     1. Prompt for username.
     2. Initialize ENet with `enet_initialize()` and set `atexit(enet_deinitialize)`.
     3. Create a client host with `enet_host_create(NULL, 1, 1, 0, 0)`.
     4. Configure server address and call `enet_host_connect()`.
     5. Wait for a `ENET_EVENT_TYPE_CONNECT` event to confirm connection.
     6. Send a registration packet formatted as `"2|username"`.
     7. Initialize the chat UI with `chatScreen.Init()`.
     8. Spawn the `msgLoop` in a separate pthread for incoming messages.
     9. Enter the message loop: call `CheckBoxInput()` to get user messages, then send via `sendPacket()`.

This design cleanly separates UI logic (`ChatScreen`) from networking logic (`client.cpp`), making it easier to extend or replace components in your future projects. 

## Server Code Structure

The server-side logic is located in `server/server.cpp` and handles new connections, message parsing, and broadcasting. It consists of:

1. **Includes & Globals**
   - `<enet/enet.h>`, `<bits/stdc++.h>`
   - `ClientData` class: wraps client `id` and `username`.
   - `map<int, ClientData*> clientMap`: tracks active peers.
   - `int new_player_id = 0`: counter for assigning unique peer IDs.

2. **Helper Functions**
   - `sendPacket(ENetPeer* peer, const char* msg)`: Sends a reliable packet on channel 0 to a single peer.
   - `broadcastPacket(ENetHost* server, const char* data)`: Sends a reliable packet to all connected peers.

3. **parseData(ENetHost* server, int id, unsigned char* data)**
   - Parses `data_type` using `sscanf`:
     - `1`: Chat message — extracts text and broadcasts `"1|id|message"`.
     - `2`: New user — extracts `username`, updates `clientMap[id]`, broadcasts `"2|id|username"`.
   - Designed for easy extension to additional message types.

4. **main()** — Server Lifecycle
   1. Initialize ENet and register `atexit(enet_deinitialize)`.
   2. Create host:
      ```cpp
      ENetAddress address;
      address.host = ENET_HOST_ANY; address.port = 7777;
      ENetHost* server = enet_host_create(&address, 32, 1, 0, 0);
      ```
   3. Enter the event loop with `enet_host_service(server, &event, 5000)`:
      - **CONNECT**: Log the new peer, broadcast existing users, assign `new_player_id`, store `ClientData` in `peer->data`, send `"3|assignedID"` to the new peer.
      - **RECEIVE**: Call `parseData(server, peerID, event.packet->data)`, then `enet_packet_destroy`.
      - **DISCONNECT**: Log disconnect, broadcast `"4|peerID"` to remaining peers.
      - **NONE**: Optional handling or exit condition.
   4. Cleanup with `enet_host_destroy(server)` and ENet deinitialization.

5. **Message Protocol**
   - `1|senderID|message` — Chat message broadcast
   - `2|senderID|username` — New user join broadcast
   - `3|assignedID` — Server assigns new peer ID
   - `4|senderID` — Peer disconnect notification

This clear, modular setup makes it easy to understand and extend the server for future real-time or game-specific functionality. 

## Code Walkthrough

Below is a step-by-step description of a typical client–server chat session using this code:

1. **Server Startup**
   - Calls `enet_initialize()` and `enet_host_create(&address, 32, 1, 0, 0)` to listen on port 7777.
   - Enters the event loop: `while (enet_host_service(server, &event, 5000) > 0)`.

2. **Client Startup and Connection**
   - Calls `enet_initialize()` and `enet_host_create(NULL, 1, 1, 0, 0)`.
   - Sets server address with `enet_address_set_host` and `address.port`.
   - Calls `enet_host_connect(client, &address, 1, 0)`.
   - Waits for `ENET_EVENT_TYPE_CONNECT`, then prints "Connection succeeded."

3. **Server Accepts New Connection**
   - Receives `ENET_EVENT_TYPE_CONNECT` in its loop.
   - Logs the event and broadcasts existing users to the newcomer.
   - Increments `new_player_id`, creates `ClientData(new_player_id)`, stores it in `peer->data` and `clientMap`.
   - Sends a packet `"3|<assignedID>"` directly to the new client using `sendPacket`.

4. **Client Receives Assigned ID**
   - In its initial event loop (or in the `msgLoop`), receives `ENET_EVENT_TYPE_RECEIVE` with data `"3|<id>"`.
   - `parseData` sees `case 3` and sets the global `CLIENT_ID` to `<id>`.

5. **Client Registers Username**
   - After connecting, the client sends `"2|<username>"` via `sendPacket` to inform the server of its name.
   - Server's `parseData` (`case 2`) extracts `<username>`, updates `clientMap[id]`, and broadcasts `"2|<id>|<username>"` to all connected peers.

6. **Clients Track New User**
   - Each client's `msgLoop` receives `2|id|username`, and `parseData` stores `ClientData` in the local `clientMap`.
   - Note: the UI will not show join messages by default, but you can extend `PostMessage` to announce joins.

7. **Chat Messaging**
   - User types a message in the ncurses input box (`CheckBoxInput()` returns a string).
   - Client wraps it as `"1|<message>"` and calls `sendPacket`.
   - Server's `parseData` (`case 1`) broadcasts `"1|<senderID>|<message>"` to all.
   - Client's `msgLoop` receives it and `parseData` calls `PostMessage(senderName, message)` to display it.

8. **Disconnect**
   - When a client disconnects or resets, server receives `ENET_EVENT_TYPE_DISCONNECT`, logs, and broadcasts `"4|<senderID>"`.
   - Clients receiving `4|id` can remove entries from their `clientMap` and optionally announce the departure.

This walkthrough illustrates how ENet events and the custom packet protocol drive the chat flow. It's easy to extend with more message types or integrate game state updates. 

## Example Usage

Below is a sample session illustrating how to run the server and connect two clients:

**1. Start the server**
```bash
./server
```
Sample output:
```
ENet server listening on port 7777...
```

**2. Client 1 (Alice)**
```bash
./client
```
```
enter the name: Alice
Connection to 192.168.1.83:7777 succeeded.
ENet client initialized successfully.
```
Type a message (e.g., `Hello, everyone!`) and press Enter.

**3. Client 2 (Bob)**
```bash
./client
```
```
enter the name: Bob
Connection to 192.168.1.83:7777 succeeded.
ENet client initialized successfully.
```

**4. Chat exchange**
- Alice's terminal shows:
```
Alice: Hello, everyone!
Bob: Hi Alice!
```
- Bob's terminal shows:
```
Alice: Hello, everyone!
Bob: Hi Alice!
```

You can open more clients similarly. Each new peer receives the list of existing users, and messages are reliably broadcast to all connected clients. 

## Additional Resources

- ENet Official Website & Docs: https://enet.bespin.org/
- ENet GitHub Repository: https://github.com/lsalzman/enet
- Comprehensive ENet Tutorial: https://gafferongames.com/post/introduction_to_enet/
- ncurses Programming Tutorial: https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/
- POSIX Threads Programming: https://computing.llnl.gov/tutorials/pthreads/
- C++ Networking Overview: https://www.boost.org/doc/libs/1_78_0/doc/html/boost_asio.html

With these resources, you can deepen your understanding of ENet, terminal UIs, and threading for building robust networked applications. 