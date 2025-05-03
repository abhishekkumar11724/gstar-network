// g++ main.cpp -o main -lenet

#include <enet/enet.h>
#include <bits/stdc++.h>
using namespace std;

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

void sendPacket(ENetPeer *peer, const char *msg)
{
    ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}

void broadcastPacket(ENetHost* server, const char * data) {
    ENetPacket * packet = enet_packet_create(data, strlen(data)+1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 0, packet);
}

map<int, ClientData *> clientMap;

void parseData(ENetHost *server, int id, unsigned char *data)
{
    cout << "PARSED :" << data << endl;
    int data_type;
    sscanf((char *)data, "%d|", &data_type);

    switch (data_type)
    {
    case 1:
       {
        char msg[80];
        sscanf(( const char *)data, "%*d|%[^\n]", &msg );
        char send_data[1024];
        sprintf(send_data, "1|%d|%s", id, msg);

        // broadcast packet

        break;
       }
    case 2:
    {
        char username[80];
        sscanf((const char *)data, "2|%[^\n]", &username);
        char send_data[1024] = {'\0'};

        sprintf(send_data, "2|%d|%s", id, username);
        cout << "SEND : " << send_data << endl;
        clientMap[id]->setUserName(username);
        break;
    }

    default:
        break;
    }
}

int main(int argc, char **argv)
{
    if (enet_initialize() != 0)
    {
        cerr << ("An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetAddress address;
    ENetHost *server;
    ENetEvent event;
    address.host = ENET_HOST_ANY;
    address.port = 7777;

    server = enet_host_create(&address /* the address to bind the server host to */,
                              32 /* allow up to 32 clients and/or outgoing connections */,
                              1 /* allow up to 2 channels to be used, 0 and 1 */,
                              0 /* assume any amount of incoming bandwidth */,
                              0 /* assume any amount of outgoing bandwidth */);
    if (server == NULL)
    {
        cerr << ("An error occurred while trying to create an ENet server host.\n");
        return (EXIT_FAILURE);
    }

    int new_player_id = 0;
    // game loop
    while (true)
    {
        while (enet_host_service(server, &event, 5000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                cout << "new peer connected at :" << event.peer->address.host << " : " << event.peer->address.host << endl;
                for (auto const &x : clientMap)
                {
                    char send_data = {'\0'};
                    sprintf((char *)send_data, "2|%d|%s", x.first, x.second->getUserName().c_str());
                }
                new_player_id++;
                clientMap[new_player_id] = new ClientData(new_player_id);
                event.peer->data = clientMap[new_player_id];
                char data_to_send[126] = {'\0'};
                sprintf(data_to_send, "3|%d", new_player_id);
                //broad cast
                sendPacket(event.peer, data_to_send);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                cout << "A packet of length " << event.packet->dataLength << " containing " << event.packet->data << " was received from " << event.peer->data << " at " << event.peer->address.host << " : " << event.peer->address.port << " on channel " << event.channelID << endl;

                parseData(server, static_cast<ClientData*> (event.peer->data)->getID(), event.packet->data);
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
    }

    // game loop

    enet_host_destroy(server);

    cout << "ENet server initialized successfully." << endl;

    return EXIT_SUCCESS;
}