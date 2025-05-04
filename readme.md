## client

g++ -lncurses -lpthread chat_screen.cpp client.cpp -o client -lenet


## server 

g++ server.cpp -o server -lenet`