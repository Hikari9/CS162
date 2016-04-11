#include <iostream>
#include <unistd.h>
#include "net.hpp"
#include "socketstream.hpp"

using namespace std;

int main() {
	// accept a client
	net::server server(4000);
	cout << "Accepting..." << endl;
	net::client client = server.accept();
	cout << "Accepted." << endl;
	
	if (fork()) {
		// message receiver
		net::isocketstream iss(client.sockfd);
		string message;
		while (getline(iss, message))
			if (!message.empty()) {
				cout << "\rClient: " << message << endl;
				cout << "Server: " << flush;
			}
	}

	else {
		// message sender
		net::osocketstream oss(client.sockfd);
		string message;
		cout << "Server: " << flush;
		while (getline(cin, message)) {
			oss << message << endl;
			cout << "Server: " << flush;
		}
	}
}