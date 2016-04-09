#ifndef __INCLUDE_NET__
#define __INCLUDE_NET__ 1

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <algorithm>

#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace net {
	using namespace std;
	// lightweight socket class
	struct socket {
		int sock;
		socket(): sock(::socket(AF_INET, SOCK_STREAM, 0)) {
			if (sock < 0) {
				perror("socket::sock()");
				throw this;
			}
		}
		socket(int sock): sock(sock) {}
		~socket() {close();}
		inline operator bool() const {return sock >= 0;}
		virtual void close() {
			if (*this) {
				if (::close(sock) < 0) {
					perror("socket::close()");
					throw this;
				}
				sock = -1;
			}
		}
	};
	// lightweight client class
	struct client : public socket {
		client(int sock = -1): socket(sock) {}
		client(const string& host, int port): socket() {
			// get host by name
			struct hostent *server = gethostbyname(host.c_str());
			if (!server) {
				herror("client::gethostbyname()");
				throw this;
			}
			// create socket address
			struct sockaddr_in sad;
			memset(&sad, 0, sizeof sad);
			sad.sin_family = AF_INET;
			memcpy(server->h_addr, &sad.sin_addr.s_addr, server->h_length);
			sad.sin_port = htons(port);
			// connect
			if (connect(sock, (sockaddr*) &sad, sizeof sad) < 0) {
				perror("client::connect()");
				throw this;
			}
		}
		// operator overloads
		template<class T> client& operator >> (T& data) const {read(data);}
		template<class T> client& operator << (T data) const {send(data);}
		template<class T>
		void send(T* data, size_t bytes) const {
			for (char* buffer = (char*) data; bytes;) {
				ssize_t sent = ::write(sock, buffer, bytes);
				if (sent < 0) {
					perror("client::send()");
					throw this;
				}
				buffer += sent;
				bytes -= sent;
			}
		}
		template<class T> inline void send(T data) const {send(&data, sizeof(data));}
		inline void send(char data[]) const {send(data, strlen(data) + 1);}
		inline void send(const char data[]) const {send(data, strlen(data) + 1);}
		inline void send(string data) const {send(data.c_str());}
		template<class T>
		void read(T* data, size_t bytes) const {
			for (char* buffer = (char*) data; bytes;) {
				ssize_t received = ::read(sock, buffer, bytes);
				if (received < 0) {
					perror("client::read()");
					throw this;
				}
				buffer += received;
				bytes -= received;
			}
		}
		template<class T> inline void read(T& data) const {read(&data, sizeof(data));}
		template<class T> inline T read() const {
			char* buffer = new char[sizeof(T)];
			read(buffer, sizeof(T));
			return *(T*) buffer;
		}
		inline void read(string& data) const {
			data.clear();
			char *buffer = new char[1];
			while (true) {
				ssize_t received = ::read(sock, buffer, 1);
				if (received < 0) {
					perror("client::read(string&)");
					throw this;
				}
				if (*buffer == '\0') break;
				data.push_back(*buffer);
			}
			delete[] buffer;
		}
		inline void read(char buffer[]) const {
			while (true) {
				ssize_t received = ::read(sock, buffer, 1);
				if (received < 0) {
					perror("client::read(char[])");
					throw this;
				}
				if (*(buffer++) == '\0')
					break;
			}
		}
	};
	template<> inline string client::read<string>() const {
		string stream;
		read(stream);
		return stream;
	}
	template<> inline char* client::read<char*>() const {
		string stream;
		read(stream);
		char *buffer = new char[stream.length() + 1];
		strcpy(buffer, stream.c_str());
		return buffer;
	}
	// lightweight server class
	struct server : public socket {
		static const int maxconn;
		server(): socket(-1) {}
		server(int port, int maxconn = server::maxconn): socket() {
			// create sock address
			struct sockaddr_in sad;
			memset(&sad, 0, sizeof sad);
			sad.sin_family = AF_INET;
			sad.sin_addr.s_addr = INADDR_ANY;
			sad.sin_port = htons(port);
			{ // unlink used port
				int enable = 1;
				if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
					perror("server::setsockopt()");
					throw this;
				}
			}
			// bind to port
			if (bind(sock, (sockaddr*) &sad, sizeof sad) < 0) {
				perror("server::bind()");
				throw this;
			}
			// listen to connections
			if (listen(sock, maxconn) < 0) {
				perror("server::listen()");
				throw this;
			}
		}
		int accept() const {
			struct sockaddr_in address;
			socklen_t length;
			int clientsock = ::accept(sock, (sockaddr*) &address, &length);
			if (clientsock < 0) {
				perror("server::accept");
				throw this;
			}
			return clientsock;
		}
		void close() {
			if (sock >= 0) {
				if (::close(sock) == -1) {
					perror("server::close()");
					throw this;
				}
				sock = -1;
			}
		}
		~server() {
			close();
		}
	};
	const int server::maxconn = SOMAXCONN;
};

#endif /* __INCLUDE_NET__ */