/**
 * A lightweight socket wrapper for a network server (IPv4), using low-level
 * POSIX sockets. Extends the net::socket class. Internally calls bind()
 * and listen() on construction. You can receive client sockets by calling
 * net::server::accept() and wrapping it with a net::client.
 * 
 * To construct a net::server, you need to assign a port it will bind to,
 * and an optional argument maxconn which specifies the number of allowed
 * clients that can connect to the server. If the specified port has
 * already been bound by another server, a net::socket_exception will be
 * thrown.
 * 
 * When the server is destroyed or closed, all clients connected to it
 * will close as well. When a single client is closed, the specific
 * net::socket tied to that client will be the only object that will
 * close.
 * 
 * You cannot send data using the server's sockfd, ergo wrapping it with
 * a socket stream will be pointless (you can try and see for yourself).
 * 
 * @namespace  	net
 * @author 		Rico Tiongson
 * @package  	SocketNetworking
 */

namespace net {
	class server : public socket {
		static const int maxconn;
		server(): socket(-1) {}
		server(const socket& sock): socket(sock.sockfd) {}
		explicit server(unsigned short port, int maxconn = server::maxconn): socket() {
			// create sock address
			struct sockaddr_in sad;
			memset(&sad, 0, sizeof sad);
			sad.sin_family = AF_INET;
			sad.sin_addr.s_addr = INADDR_ANY;
			sad.sin_port = htons(port);
			{
				// unlink used port
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
			socklen_t length = sizeof(address);
			int clientsock = ::accept(sock, (sockaddr*) &address, &length);
			if (clientsock < 0) {
				perror("server::accept()");
				throw this;
			}
			return clientsock;
		}
	};
	// static variable declarations
	const int server::maxconn = SOMAXCONN;

}