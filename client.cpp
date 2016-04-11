#include <iostream>
#include <cstdio>
#include <unistd.h>
#include "net.hpp"
#include "socketstream.hpp"

using namespace std;

int main() {
	// connect to server
	net::client client("localhost", 4000);

	if (fork()) {
		// message receiver
		net::isocketstream iss(client.sockfd);
		string message;
		while (getline(iss, message))
			if (!message.empty()) {
				cout << "\rServer: " << message << endl;
				cout << "Client: " << flush;
			}
	}

	else {
		// message sender
		net::osocketstream oss(client.sockfd);
		string message;
		cout << "Client: " << flush;
		while (getline(cin, message)) {
			oss << message << endl;
			cout << "Client: " << flush;
		}
	}
}