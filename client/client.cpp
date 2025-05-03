// g++ main.cpp -o main -lenet

#include <enet/enet.h>
#include <bits/stdc++.h>
#include<pthread.h>
#include "chat_screen.hpp"
using namespace std;

static ChatScreen chatScreen;
static int CLIENT_ID = -1;
void sendPacket(ENetPeer * peer, const char * msg) {
    ENetPacket * packet = enet_packet_create(msg, strlen(msg)+1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}

class ClientData
{
    int m_id;
    string m_username;

public:
    ClientData(int id) { m_id = id; };
    void setUserName(string username) { m_username = username; };
    int getID() { return m_id; };
    string getUserName() { return m_username; };
};

map<int, ClientData *> clientMap;

void parseData( unsigned char *data)
{
    int data_type;
    int id;
    sscanf((const char *)data, "%d|%d", &data_type, &id);

    switch (data_type)
    {
    case 1:
        /* code */
        if(id != CLIENT_ID) {
            char msg[80];
            sscanf((const char *)data,"%*d|%*d|%[^|]", &msg);
            chatScreen.PostMessage(clientMap[id]->getUserName().c_str(),msg);
        }
        break;
    case 2:
    {
        if(id != CLIENT_ID){
            char username[80];
            sscanf((const char*)data, "%*d|%*d|%[^|]",  &username);
            clientMap[id] = new ClientData(id);
            clientMap[id]->setUserName(username);
        }
        break;
    }
    case 3:
    {
        CLIENT_ID = id;
        break;
    }

    default:
        break;
    }
}

void * msgLoop(void *arg) {
    ENetHost *client = (ENetHost *)arg;
    while(true) {
        ENetEvent event;
        while(enet_host_service(client, &event, 1000 )>0) {
            switch(event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                    cout << "A packet of length" << event.packet->dataLength << "containing" << event.packet->data << " was received from" << event.peer->data << " at " << event.peer->address.host << " : " << event.peer->address.port << " on channel " << event.channelID << endl;
                    enet_packet_destroy(event.packet);
                    break;
            }
        }
    }
}

int main(int argc, char **argv)
{

    cout<<"enter the name: ";
    char username[80];
    scanf("%s", username);
    if (enet_initialize() != 0)
    {
        cerr << "An error occurred while initializing ENet." << endl;
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);
    ENetHost *client;
    client = enet_host_create(NULL, 1, 1, 0, 0);
    if (client == NULL)
    {
        cerr << "could not create a enet clinet" << endl;
        return EXIT_FAILURE;
    }

    ENetAddress address;
    ENetEvent event;
    ENetPeer *peer;

    enet_address_set_host(&address, "192.168.1.83");
    address.port = 7777;
    peer = enet_host_connect(client, &address, 1, 0);

    if (peer == NULL)
    {
        cerr << ("No available peers for initiating an ENet connection.\n");
        return (EXIT_FAILURE);
    }

    if (enet_host_service(client, &event, 1000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        puts("Connection to 192.168.1.83:7777 succeeded.");
    }
    else
    {
        enet_peer_reset(peer);
        puts("Connection to 192.168.1.83:7777 failed.");
        return EXIT_SUCCESS;
    }
    cout << "ENet client initialized successfully." << endl;

    while (enet_host_service(client, &event, 1000) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            cout << "A new client connected from " << event.peer->address.host << " : " << event.peer->address.port << endl;
            // event.peer->data = "Client information";
            break;

        case ENET_EVENT_TYPE_RECEIVE:
            cout << "A packet of length " << event.packet->dataLength << " containing " << event.packet->data << " was received from " << event.peer->data << " at " << event.peer->address.host << " : " << event.peer->address.port << " on channel " << event.channelID << endl;
            enet_packet_destroy(event.packet);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            cout << event.peer->data << " disconnected. at " << event.peer->address.host << " : " << event.peer->address.port << endl;
            event.peer->data = NULL;

        case ENET_EVENT_TYPE_NONE:
            cout << "event type ENET_EVENT_TYPE_NONE occured, now exiting." << endl;
            break;
        }
    }

    // game loop start

    // enet_peer_disconnect(peer, 0);
    // while (enet_host_service(client, &event, 3000) > 0)
    // {
    //     switch (event.type)
    //     {
    //     case ENET_EVENT_TYPE_CONNECT:
    //         cout << "A new client connected from " << event.peer->address.host << " : " << event.peer->address.port << endl;
    //         // event.peer->data = "Client information";
    //         break;
    //     case ENET_EVENT_TYPE_RECEIVE:
    //         enet_packet_destroy(event.packet);
    //         break;

    //     case ENET_EVENT_TYPE_DISCONNECT:
    //         puts("Disconnection succeeded.");
    //         break;

    //     case ENET_EVENT_TYPE_NONE:
    //         cout << "event type ENET_EVENT_TYPE_NONE occured, now exiting." << endl;
    //         break;
    //     }
    // }

    // enet_peer_reset(peer);

    // enet_host_destroy(client);


    char str_data[80] = "2|";
    strcat(str_data, username);
    sendPacket(peer, str_data);
    
    // game loop end

    chatScreen.Init();

    pthread_t thread;
    pthread_create(&thread, NULL, msgLoop, client);

    bool running = true;
    while(running) {
        string message = chatScreen.CheckBoxInput();
        char* st = new char[message.size()+1];
        strcpy(st, message.c_str());

        char msg[80] = "1|";
        strcat(msg, message.c_str());
        chatScreen.PostMessage(username,st);
        sendPacket(peer, msg);
        if( message == "/exit") running = false; 
    }

    pthread_join(thread, NULL);

    return EXIT_SUCCESS;
}