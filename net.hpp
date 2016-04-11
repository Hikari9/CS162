/**
 * The net-cpp API implements lightweight socket programming
 * using unix sockets (linux platform).
 *
 * @author Rico Tiongson
 */

#ifndef __INCLUDE_NET__
#define __INCLUDE_NET__ 1

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <map>
#include <algorithm>

#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace net {
	using namespace std;
	/**
	 * A lightweight socket wrapper for the UNIX socket.
	 * Performs automatic garbage collection for multiple copies of the same socket.
	 */
	class socket {
	private:
		static map<int, int> sfd;
		inline void open_sock() {if (sockfd >= 0) ++sfd[sockfd];}
	public:
		int sockfd;
		socket(): sockfd(::socket(AF_INET, SOCK_STREAM, 0)) {open_sock();}
		socket(int sockfd): sockfd(sockfd) {open_sock();}
		socket(const socket& s): sockfd(s.sockfd) {open_sock();}
		socket& operator = (const socket& s) {sockfd = s.sockfd; open_sock(); return *this;}
		~socket() {if (*this && --sfd[sockfd] == 0) close();}
		inline operator bool() const {return sockfd >= 0;}
		virtual void close() {
			if (*this) {
				sfd.erase(sockfd);
				if (::close(sockfd) < 0) {
					perror("socket::close()");
					throw this;
				}
				sockfd = -1;
			}
		}
		bool operator == (const socket& s) const {return sockfd == s.sockfd;}
		bool operator != (const socket& s) const {return sockfd != s.sockfd;}
		bool operator < (const socket& s) const {return sockfd < s.sockfd;}
	};
	/**
	 * A lightweight socket wrapper for a network client.
	 * Uses C++ stream-like operations in order to send (<<) or read (>>) data.
	 * Implicitly converts any data type into bytes on send.
	 */
	struct client : public socket {
		client(int sockfd = -1): socket(sockfd) {}
		client(const socket& s): socket(s) {}
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
			if (connect(sockfd, (sockaddr*) &sad, sizeof sad) < 0) {
				perror("client::connect()");
				throw this;
			}
		}
		// operator overloads
		template<class T> client& operator >> (T& data) {read(data); return *this;}
		template<class T> client& operator << (T data) {send(data); return *this;}
		template<class T>
		void send(T* data, size_t bytes) const {
			for (char* buffer = (char*) data; bytes;) {
				ssize_t sent = ::write(sockfd, buffer, bytes);
				if (sent <= 0) {
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
				ssize_t received = ::read(sockfd, buffer, bytes);
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
				ssize_t received = ::read(sockfd, buffer, 1);
				if (received <= 0) {
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
				ssize_t received = ::read(sockfd, buffer, 1);
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
	/**
	 * A lightweight socket wrapper for a network server.
	 * Internally calls bind() and listen() on construction.
	 * You can receive client sockets by calling server::accept().
	 */
	struct server : public socket {
		static const int maxconn;
		server(): socket(-1) {}
		server(const socket& s): socket(s) {}
		server(int port, int maxconn = server::maxconn): socket() {
			// create sockfd address
			struct sockaddr_in sad;
			memset(&sad, 0, sizeof sad);
			sad.sin_family = AF_INET;
			sad.sin_addr.s_addr = INADDR_ANY;
			sad.sin_port = htons(port);
			{ // unlink used port
				int enable = 1;
				if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
					perror("server::setsockopt()");
					throw this;
				}
			}
			// bind to port
			if (bind(sockfd, (sockaddr*) &sad, sizeof sad) < 0) {
				perror("server::bind()");
				throw this;
			}
			// listen to connections
			if (listen(sockfd, maxconn) < 0) {
				perror("server::listen()");
				throw this;
			}
		}
		net::socket accept() const {
			struct sockaddr_in address;
			socklen_t length = sizeof(address);
			int clientsock = ::accept(sockfd, (sockaddr*) &address, &length);
			if (clientsock < 0) {
				perror("server::accept()");
				throw this;
			}
			return clientsock;
		}
	};
	// static variable declarations
	const int server::maxconn = SOMAXCONN;
	map<int, int> socket::sfd;
};

#endif /* __INCLUDE_NET__ */