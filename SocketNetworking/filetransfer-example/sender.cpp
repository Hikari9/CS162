/**
 * Send a file to a server through network by sending the number of bytes first.
 */
#include <iostream>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "../net_client.hpp"

using namespace std;

int main(int argc, char* argv[]) {
	// get arguments
	if (argc < 4) {
		printf("Some missing arguments\n");
		printf("Format: %s <host> <port> <filename>\n", argv[0]);
		return 0;
	}
	char* host = argv[1];
	char* port = argv[2];
	char* filename = argv[3];
	// retrieve file
	ifstream file(filename);
	if (!file) {
		printf("File not found\n");
		return EXIT_FAILURE;
	}
	// connect to the server
	net::client client;
	while (!client) {
		try {client = net::client(host, atoi(port));}
		catch (net::socket_exception) {
			printf("Cannot connect to \"%s:%s\". Attempting to reconnect...\n", host, port);
			sleep(3);
		}
	}
	printf("Waiting for upload to finish...\n");
	// get size of file
	file.seekg(0, file.end);
	int bytes = file.tellg();
	file.seekg(0);
	// read file to memory
	char* buffer = new char[bytes];
	file.read(buffer, bytes);
	file.close();
	client.send(bytes);			// send file number of bytes first
	client.send(buffer, bytes); // then send the actual file
	delete[] buffer;
	// receive a ping back that all is ok
	if (client.read<bool>()) printf("Uploaded %s (size=%.3fKB)\n", filename, bytes / 1000.0f);
	else printf("An error occured in uploading %s (size=%dB)\n", filename, bytes);
}