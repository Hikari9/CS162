#include <iostream>					// std::cout, std::cin, std::getline(), std::endl, std::flush
#include <unistd.h>					// fork()
#include "../net_server.hpp"		// net::server, net::socket
#include "../net_client.hpp"		// net::client, net::socket
#include "../net_socketstream.hpp" 	// net::isocketstream, net::osocketstream

using namespace std;

int main() {
#ifdef SERVER // compile with -DSERVER option
	cout << "Accepting client..." << endl;
	net::server server(4000);
	net::socket socket = server.accept();
	cout << "Client has joined." << endl;
	cout << "Server: " << flush;
	if (fork()) {
		net::isocketstream sockin(socket);
		string message;
		while (getline(sockin, message))
			if (!message.empty())
				cout << "\rClient: " << message << "\nServer: " << flush;
	} else {
		net::osocketstream sockout(socket);
		string message;
		while (getline(cin, message)) {
			sockout << message << endl;
			cout << "Server: " << flush;
		}
	}
#else // CLIENT
	cout << "Connecting to server..." << endl;
	net::client socket("localhost", 4000);
	cout << "Server has joined." << endl;
	cout << "Client: " << flush;
	if (fork()) {
		net::isocketstream sockin(socket);
		string message;
		while (getline(sockin, message))
			if (!message.empty())
				cout << "\rServer: " << message << "\nClient: " << flush;
	} else {
		net::osocketstream sockout(socket);
		string message;
		while (getline(cin, message)) {
			sockout << message << endl;
			cout << "Client: " << flush;
		}
	}
#endif
}