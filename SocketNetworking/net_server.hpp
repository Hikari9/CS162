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

#include "net_socket.hpp"

namespace net {

	/**
	 * @brief      a lightweight wrapper class for a server socket
	 */

	class server : public socket {
	public:

		/**
		 * the default maximum length to which the queue of pending connections for the socket may grow
		 */

		static const int DEFAULT_MAXCONN = SOMAXCONN;

		/**
		 * @brief      constructs an empty server socket
		 */

		server(): socket(-1) {}
		
		/**
		 * @brief      constructs and copies the file descriptor of another server socket
		 * @param[in]  sock  the server socket to copy; must already be bound in order to accept connections
		 */

		server(const server& sock): socket(sock) {}

		/**
		 * @brief      constructs a server socket and binds it to a specific port in the local host
		 * @details    calls socket(), bind(), and listen() in that order
		 * @param[in]  port     the port that this server socket will bind to
		 * @param[in]  maxconn  the backlog parameter on listen(); default value is server::DEFAULT_MAXCONN
		 * @param[in]  key		the key to the network card ip to bind to [default: NULL]
		 * @throw      a socket_exception if there server could not bind to the port or listen to connections
		 */

		explicit server(unsigned short port, int maxconn = server::DEFAULT_MAXCONN, const char* key = NULL): socket() {
			// create socket address
			struct sockaddr_in sad;
			memset(&sad, 0, sizeof sad);
			sad.sin_family = AF_INET; // IPv4 socket
			sad.sin_addr.s_addr = key == NULL ? INADDR_ANY : inet_addr(net::ip_address(sad.sin_family, key));
			sad.sin_port = htons(port);
			// unlink used port under a previous unclosed bind()
			int unbind = 1;
			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &unbind, sizeof(unbind)) < 0)
				throw socket_exception("server::setsockopt()");
			// bind to port
			if (bind(sockfd, (sockaddr*) &sad, sizeof sad) < 0)
				throw socket_exception("server::bind()");
			// listen to connections
			if (listen(sockfd, maxconn) < 0)
				throw socket_exception("server::listen()");
		}

		/**
		 * @brief      accepts a connecting socket
		 * @details    calls accept() and wraps a socket file descriptor to a socket object
		 * @throw      a socket_exception if there was a problem in accepting the client socket
		 * @return     a socket referring to the accepted client
		 */

		socket accept() const throw(socket_exception) {
			struct sockaddr_in address;
			socklen_t length = sizeof(address);
			int clientsock = ::accept(sockfd, (sockaddr*) &address, &length);
			if (clientsock < 0)
				throw socket_exception("server::accept()");
			return clientsock;
		}

		/**
		 * @brief      gets the ip address of the host socket
		 * @details    uses net::ip_address() by default instead of socket::ip(), because the latter usually gives a loopback ip
		 * @throw      a socket_exception if the socket cannot get the host's name
		 * @return     a const char pointer to the local ip address of the host socket
		 */

		virtual const char* ip() const throw(socket_exception) {
			return net::ip_address();
		}

	};

	// define the static constant so it can be used in the namespace
	const int server::DEFAULT_MAXCONN;

}