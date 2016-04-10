#include <iostream>
#include <cstdio>
#include <map>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include "net.hpp"
#include "netinfo.hpp"

using namespace std;

vector<net::client> clients;

void* client_listener(void* args) {
	net::client client = *(net::client*) args;
	int pid = client.sock;
	printf("Server (thread-%d): acquiring name of client [%d]...\n", pid, client.sock);
	string name; client >> name;
	printf("Server (thread-%d): %s has entered the room.\n", pid, name.c_str());
	client.send(true); // send a ping to client that everything is ok
	string message;
	try {
		while (client >> message) {
			if (message.length()) {
				cout << name << ": " << message << endl;
				message = name + ": " + message;
				for (int i = 0; i < clients.size(); ++i)
					if (clients[i] != client)
						clients[i] << message;
			}
		}
	} catch (net::client const*) {
		message = name + " has left the room.";
		printf("Server (thread-%d): %s\n", pid, message.c_str());
		for (int i = 0; i < clients.size(); ++i) {
			if (clients[i] == client) {
				// kick the client
				swap(clients[i], clients.back());
				clients.pop_back();
				--i;
			} else {
				clients[i] << message;
			}
		}
	}
	client.close();
}

int main(int argc, char* argv[]) {
	// check validity of arguments
	if (argc == 1) {
		printf("Some missing arguments\n");
		printf("Format: %s <port>\n", argv[0]);
		return 0;
	}
	// create server using port
	net::server server(atoi(argv[1]));
	printf("Server (main): created server at %s (port %s) [%d]\n", net::ip_address().c_str(), argv[1], server.sock);
	// indefinitely accept clients
	while (true) {
		printf("Server (main): accepting clients...\n");
		net::client client = server.accept();
		printf("Server (main): connected to client socket [%d]\n", client.sock);
		clients.push_back(client);
		// create a listening thread for the client
		int error = 0;
		if (error = pthread_create(new pthread_t(), NULL, &client_listener, &clients.back())) {
			errno = error;
			perror("pthread_create()");
		}
		sleep(1);
	}
}