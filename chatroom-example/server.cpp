// compile: g++ server.cpp -pthread -o server
#include <iostream>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <map>
#include <vector>
#include "../net_client.hpp"
#include "../net_server.hpp"

using namespace std;

// maintain a list of clients
map<string, net::client> clients;

// send a message to all clients
void send_all(string message, net::client sender = -1) {
	message = "[" + message + "]";
	cout << message << endl;
	for (map<string, net::client>::iterator it = clients.begin(); it != clients.end(); ++it) {
		net::client& subscriber = it->second;
		if (subscriber != sender)
			subscriber.send(message);
	}
}

// thread that listens to clients
void* client_listener(void* args);

int main(int argc, char* argv[]) {
	// check validity of arguments
	if (argc == 1) {
		printf("Some missing arguments\n");
		printf("Format: %s <port>\n", argv[0]);
		return 0;
	}
	int port = atoi(argv[1]);
	// create server using port, listen up to 4 connections
	net::server server(port, 4);
	int sockfd = (int) server;	// we can get sockfd of server
	printf("Server: created server at %s (port %s) [sockfd=%d]\n", server.ip(), argv[1], sockfd);
	// maintain a a pthread vector that will be destructed on exit
	vector<pthread_t> pthreads;
	// indefinitely accept clients
	while (true) {
		printf("Server: accepting clients...\n");
		// accept a client from the server socket
		net::client client = server.accept();
		printf("Server: connected to client socket [sockfd=%d]\n", (int) client);
		// create a dedicated thread for client messages
		pthreads.push_back(pthread_t());
		int clientfd = client, error = 0;
		if (error = pthread_create(&pthreads.back(), NULL, &client_listener, reinterpret_cast<void*>(clientfd))) {
			errno = error;
			perror("pthread_create()");
		}
		sleep(1);
	}
}

// thread that listens to client messages
void* client_listener(void* args) {
	net::client client = *(int*) &args;
	const char* ip = client.ip();
	printf("Server: acquiring name of client [%s]...\n", ip);
	string name = client.read<string>();
	if (client.good()) {
		// check if name already exists
		if (clients.count(name)) {
			// send an error ping
			client.send(false);
			client.send("that name already exists!");
		} else {
			clients[name] = client;
			// send an ok ping
			client.send(true);
			send_all(name + " entered the room");
			string message;
			while (client.read(message))
				if (!message.empty())
					send_all(name + ": " + message, client);
		}
	}
	clients.erase(name);
	send_all(name + " has left the room");
}