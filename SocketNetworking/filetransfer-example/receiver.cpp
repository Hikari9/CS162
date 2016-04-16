/**
 * Receive a file from a client through the network by receiving the number of bytes first.
 */
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "../net_server.hpp"
#include "../net_client.hpp"

using namespace std;

int main(int argc, char* argv[]) {
	// get arguments
	if (argc < 3) {
		printf("Some missing arguments\n");
		printf("Format: %s <port> <filename>\n", argv[0]);
		return 0;
	}
	char* port = argv[1];
	char* filename = argv[2];
	// open server for one connection
	net::server server(atoi(port), 1);
	printf("Server is at %s:%s\n", server.ip(), port);
	printf("Waiting for client...\n");
	net::client client = server.accept();
	// get the number of bytes to receive first
	int bytes = client.read<int>();
	printf("Receiving file (size=%.3fKB)...\n", bytes / 1000.0f);
	char* buffer = new char[bytes];
	// write into the buffer
	try {client.read(buffer, bytes);}
	catch (net::socket_exception ex) {
		printf("Receive failed %s\n", ex.what());
		delete[] buffer;
		return EXIT_FAILURE;
	}
	printf("Writing to file...\n");
	// write to file
	try {
		ofstream file(filename);
		file.write(buffer, bytes);
		client.send(true);
		printf("Downloaded file to %s\n", filename);
	} catch (...) {
		printf("Could not write to file\n");
		client.send(false);
	}
	delete[] buffer;
}