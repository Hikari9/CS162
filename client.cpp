#include <iostream>
#include <unistd.h>
#include "net.hpp"
#include "socketstream.hpp"

using namespace std;

int main() {
	// connect to server
	net::client client("localhost", 4000);

	if (fork() == 0) {
		// message receiver
		net::isocketstream iss(client.sock);
		string message;
		while (getline(iss, message))
			if (!message.empty()) {
				cout << "\rServer: " << message << endl;
				cout << "Client: " << flush;
			}
	}

	else {
		// message sender
		net::osocketstream oss(client.sock);
		string message;
		cout << "Client: " << flush;
		while (getline(cin, message)) {
			oss << message << endl;
			cout << "Client: " << flush;
		}
	}
}