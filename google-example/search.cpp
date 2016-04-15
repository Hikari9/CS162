// a console google search using a client socket
#include <iostream>					// std::cin, std::cout
#include <sstream>					// std::ostringstream
#include <cctype>					// std::isspace 
#include <ctime>					// std::clock, std::CLOCKS_PER_SEC
#include "../net_client.hpp"		// net::client
#include "../net_socketstream.hpp"	// net::isocketstream, net::osocketstream

using namespace std;

int main() {
	// connect to google AJAX API
	string host = "ajax.googleapis.com";
	int http = 80;
	try {
		// try connecting to google
		cout << "Connecting to " << host << "..." << endl;
		long ping = clock();
		net::client google(host, http);
		long delta = clock() - ping;
		cout << "Connected to " << host << " (" << google.ip() << ")" << endl;
		cout << "Ping time: " << delta << "ms" << endl;
		net::isocketstream sockin(google, 0, 1024); // 0 bytes for putback, 1024 bytes for buffer
		net::osocketstream sockout(google);
		cout << "Search: ";
		string search;
		getline(cin, search);
		// separate spaces with pluses
		for (int i = 0; i < search.length(); ++i)
			if (isspace(search[i]))
				search[i] = '+';
		// make the main query to google through API
		sockout << "GET /ajax/services/search/web?v=1.1&q=" << search << " HTTP/1.1" << endl;
		sockout << "Host: " << host << endl;
		sockout << "Connection: Close" << endl << endl;
		// get properties of the connection as a map
		while (getline(sockin, search) && search.length() > 1);
		// pretty print the JSON content of the query
		string content;
		int tabs = 0, quotes = 0;
		double searchResultTime = -1;
		while (getline(sockin, content)) {
			for (int i = 0; i < content.length(); ++i) {
				char c = content[i];
				if (quotes) {
					cout << c;
					if (c == '"')
						quotes ^= 1;
				}
				else switch (c) {
					case '{': case '[':
						tabs++;
					case ',':
						cout << c << '\n' << string(tabs*2, ' ');
						break;
					case '}': case ']':
						tabs--;
						cout << '\n' << string(tabs*2, ' ') << c;
						break;
					case '"':
						cout << c;
						quotes ^= 1;
						break;
					case '\\':
						++i;
					default:
						cout << c;
				}
			}
			// get search result time, if any
			string query = "\"searchResultTime\":\"";
			int pos = content.find(query);
			if (pos != string::npos) {
				int spos = pos + query.length();
				int epos = content.find("\"", spos);
				istringstream(content.substr(spos, epos - spos)) >> searchResultTime;
			}
		}
		cout << endl << endl;
		cout << "Search time: " << searchResultTime << "s" << endl;

	} catch (net::socket_exception ex) {
		cout << "Could not resolve host " << host << endl;
		cerr << ex.what() << endl;
	}
}