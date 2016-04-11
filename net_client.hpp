/**
 * A lightweight socket wrapper for a network client, using low-level
 * POSIX sockets. Extends the net::socket class.
 * 
 * The net::client object supports communication between the server and
 * the client using bytes. Although this class is named as a "client"
 * class, this does not in any way mean that the server cannot make use
 * of its functions. In a server-client model, a server can accept
 * multiple clients through the net::server::accept() method, which can
 * be wrapped into the net::client object. On the client side, one can
 * instantiate a net::client object with a host and port constructor
 * to call connect() to a server with such credentials.
 * 
 * Unlike net::isocketstream and net::osocketstream which stream data as
 * characters, net::client sends data in raw bytes, which buffer-free 
 * thus more efficient. It does so through implicit conversion using
 * C++ templates and the sizeof() compiler function, given that the data
 * has no pointers.
 * 
 * @namespace  	net
 * @author 		Rico Tiongson
 * @package  	SocketNetworking
 */


#ifndef __INCLUDE_NET_CLIENT__
#define __INCLUDE_NET_CLIENT__

#include <string>			// std::string
#include <sys/types.h>		// sockaddr, sockaddr_in
#include <sys/socket.h>		// connect()
#include <netdb.h>			// gethostbyname()
#include <arpa/inet.h>		// htons()
#include "net_socket.hpp"	// net::socket, net::socket_exception


namespace net {

	using namespace std;

	/**
	 * @brief      a lightweight wrapper class for a client socket
	 */

	class client : public socket {
	public:

		/**
		 * @brief      constructs and wraps a file descriptor as a client socket
		 * @details    does not call connect()
		 * @param[in]  sockfd	a socket file descriptor
		 */

		client(int sockfd = -1): socket(sockfd) {}

		/**
		 * @brief      constructs and copies another socket as a client socket
		 * @details    does not call connect()
		 * @param[in]  sock		another socket
		 */

		client(const socket& sock): socket(sock) {}

		/**
		 * @brief      constructs and connects a client socket to a server with a specific host and port
		 * @param[in]  host  
		 * @param[in]  port  { parameter_description }
		 */

		client(const string& host, int port): socket() throw(socket_exception) {
			// get host by name
			struct hostent *server = gethostbyname(host.c_str());
			if (!server) throw socket_exception("client::gethostbyname()");
			// create socket address
			struct sockaddr_in sad;
			memset(&sad, 0, sizeof sad);
			sad.sin_family = AF_INET;
			sad.sin_port = htons(port);
			// connect
			if (connect(sock, (sockaddr*) &sad, sizeof sad) < 0)
				throw socket_exception("client::connect()");
		}
		// operator overloads
		// template<class T> client& operator >> (T& data) {read(data); return *this;}
		// template<class T> client& operator << (T data) {send(data); return *this;}
		template<class T>
		void send(T* data, size_t bytes) const {
			for (char* buffer = (char*) data; bytes;) {
				ssize_t sent = ::write(sock, buffer, bytes);
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

}