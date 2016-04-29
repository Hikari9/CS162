// multiple chat linux server, for CS 162 Lab 10 requirement
// compile: g++ server.cpp -pthread -o server
#include <iostream>
#include <sstream>
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
vector<net::client> clients;

// kick clients above this threshold
int max_connections;

// send a message to all clients
void send_all(string message, net::client sender = -1) {
	for (int i = 0; i < clients.size(); ++i)
		if (clients[i] != sender)
			clients[i].send(message);
	clog << message << endl; // have a log in the console
}

// thread that listens to clients
void* client_listener(void*);

// thread that listens to server input
void* server_listener(void*);

int main(int argc, char* argv[]) {
	// check validity of arguments
	if (argc == 1) {
		printf("Some missing arguments\n");
		printf("Format: %s <max_connections> <port>\n", argv[0]);
		return 0;
	}
	::max_connections = atoi(argv[1]);
	int port = atoi(argv[2]);
	// create server using port, listen up to max_connections
	net::server server(port, max_connections);
	int sockfd = (int) server;	// we can get sockfd of server
	printf("Server: created server at %s (port %d) [sockfd=%d]\n", server.ip(), port, sockfd);
	// maintain a a pthread vector that will be destructed on exit
	vector<pthread_t> pthreads;
	// have an input process that accepts input from the server
	{
		int error;
		pthreads.push_back(pthread_t());
		if (error = pthread_create(&pthreads.back(), NULL, &server_listener, NULL)) {
			errno = error;
			perror("pthread_create()");
		}
	}
	// indefinitely accept clients
	while (true) {
		printf("Server: accepting clients...\n");
		// accept a client from the server socket
		net::client client = server.accept();
		printf("connected to client socket [%d]\n", (int) client);
		// create a dedicated thread for client messages
		pthreads.push_back(pthread_t());
		int error;
		if (error = pthread_create(&pthreads.back(), NULL, &client_listener, reinterpret_cast<void*>((int) client))) {
			errno = error;
			perror("pthread_create()");
		}
		sleep(1);
	}
}

// thread that listens to server input
void* server_listener(void*) {
	string message;
	while (getline(cin, message)) {
		if (message == "@exit") {
			send_all("Server commenced shutdown");
			exit(EXIT_SUCCESS);
		}
	}
}

// thread that listens to client messages
void* client_listener(void* args) {
	net::client client = *(int*) &args;
	const char* ip = client.ip();
	printf("acquiring name of client [%s]...\n", ip);
	// get name of the client
	string name;
	client.read(name);
	if (client.good()) {
		if (clients.size() >= max_connections) {
			// server already full
			client.send(false);
			client.send("server is already full");
		} else {
			// client can join
			clients.push_back(client);
			string label;
			try {
				client.send(true);
				client.send((int) client); // send the socket number of this client as well
				{
					// prepare the label of this client: "(sockfd)[name]: "
					ostringstream oss;
					oss << "(" << (int) client << ")[" << name << "]";
					label = oss.str();
				}
				{
					// send a message to everyone that client has joined
					ostringstream oss;
					oss << name << " entered the room {{ " << label << " }}";
					send_all(oss.str());
				}
				// indefinitely listen to messages
				string message;
				while (client.read(message)) {
					if (message == "@exit") {
						client.close();
						break;
					}
					if (!message.empty()) {
						ostringstream oss;
						send_all(label + ": " + message, client);
					}
				}
			} catch (net::socket_exception ex) {
				cerr << ex.what() << endl;
			}
			// client has disconnected
			for (int i = 0; i < clients.size(); ++i) {
				if (!clients[i].good()) {
					clients[i--] = clients.back();
					clients.pop_back();
				}
			}
			ostringstream oss;
			oss << name << " has left the room {{ " << label << " }}";
			send_all(oss.str());
		}		
	}
}
