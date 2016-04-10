#include <iostream>
#include <cstdio>
#include <map>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <pthread.h>
#include "../net.hpp"
#include "../netinfo.hpp"

using namespace std;

// maintain a list of clients
vector<net::client> clients;
void* client_listener(void*);

void send_all(const string& message, net::client sender = -1) {
	cout << message << endl;
	for (int i = 0; i < clients.size(); ++i)
		if (clients[i] != sender)
			clients[i] << message;
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

void* client_listener(void* args) {
	net::client client = *(net::client*) args;
	int pid = client.sock;
	printf("Server (thread-%d): acquiring name of client [%d]...\n", pid, client.sock);
	string name; client >> name;
	client << true; // send a ping to client that everything is ok
	send_all("(" + name + " has entered the room" + ")");
	try {
		string message;
		while (client >> message)
			if (message.length())
				send_all(name + ": " + message, client);
	} catch (net::client const*) {
		send_all(name + " has left the room.", client);
		clients.erase(find(clients.begin(), clients.end(), client));
	}
	client.close();
}