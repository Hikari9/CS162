/**
 * This header defines input/output stream objects for low-end
 * web sockets (POSIX) as "socket streams". The socket streams
 * use file descriptors (fd) to read and write into the stream
 * The fact that low-end sockets use file descriptors (fd) allow
 * read() and write() capabilities even for non-sockets.
 * 
 * Unlike the "stream-like" behavior of net::client, which sends
 * data as bytes, socket streams send non-character data types
 * (e.g. short, int, long) as strings, similar to the behavior
 * of std::cout. The performance of net::client is therefore
 * faster than socket streams, however socket streams are able
 * to extend the capabilities of std::istream and std::ostream.
 * Socket streams can use flags and iostream methods from STL,
 * and is capable of buffering unlike the standalone net::client.
 * 
 * This header file is standalone. You can wrap it to any file
 * descriptor, which is standard for any C++ compiler. However,
 * it is still recommended to patch this to the other net
 * namespaced headers.
 * 
 * @namespace  	net
 * @author 		Rico Tiongson
 * @package  	SocketNetworking
 * 
 * @example     {a simple server/client console chat implementation}
 * 
 * #include <iostream>				// std::cout, std::cin, std::getline(), std::endl, std::flush
 * #include <unistd.h>				// fork()
 * #incldue "net_server.hpp"		// net::server, net::socket
 * #include "net_client.hpp"		// net::client, net::socket
 * #include "net_socketstream.hpp" 	// net::isocketstream, net::osocketstream
 * 
 * using namespace std;
 * 
 * int main() {
 * #ifdef SERVER
 * 		cout << "Accepting client..." << endl;
 * 		net::server server(4000); 			  	// create a server at port 4000
 * 		net::socket socket = server.accept(); 	// accept a client socket
 * 		cout << "Client has joined the room." << endl;
 * #else
 * 		net::client socket("localhost", 4000);	// connect to a server
 * #endif
 * 		if (fork()) {
 * 			// message listener, parent process
 * 			net::isocketstream sockin(socket.sockfd);
 * 			string message;
 * 			while (getline(sockin, message))
 * 				if (!message.empty())
 * 					cout << "\rClient: " << message << endl << "Server: " << flush;
 * 		}
 * 		else {
 * 			// message sender, child process, terminate when socket is closed
 * 			net::osocketstream sockout(socket.sockfd);
 * 			string message;
 * 			while (getline(cin, message)) {
 * 				sockout << message << endl;
 * 				cout << "Server: " << flush;
 * 			}	
 * 		}
 * }
 * 
 */

#ifndef __INCLUDE_NET_SOCKETSTREAM__
#define __INCLUDE_NET_SOCKETSTREAM__

#include <streambuf>	// std::streambuf, std::streamsize, std::size_t, std::memmove()
#include <istream>		// std::istream
#include <ostream>		// std::ostream
#include <sys/socket.h>		// send(), recv()

namespace net {

	using namespace std;

	/**
	 * the output socket buffer object
	 * @extends  std::streambuf
	 */
	
	class osocketbuf : public streambuf {

	protected:

		/**
		 * socket file descriptor (sockfd)
		 */
		
		int sockfd;

	public:

		/**
		 * @brief      constructs an output socket buffer to wrap a socket file descriptor; can be used for regular files
		 * @param[in]  sockfd   a file descriptor describing the connecting socket
		 */
		
		osocketbuf(int sockfd): sockfd(sockfd) {}

		/**
		 * @brief      syncs then destructs the output socket buffer object
		 */
		
		~osocketbuf() {
			sync();
		}
		
		/**
		 * @brief      sends a single character to the connecting socket
		 * @param[in]  c     the character to send as a streambuf::int_type
		 * @return     a streambuf::int_type describing whether the character was sent or the end of file was reached
		 */
		
		virtual int_type overflow (int_type c) {
			return c != traits_type::eof() && send(sockfd, &c, 1, 0) <= 0 ? traits_type::eof() : c;
		}

		/**
		 * @brief      sends an array of characters to the connecting socket
		 * @param[in]  data   the data to send
		 * @param[in]  bytes  the number of bytes to send
		 * @return     a streambuf::streamsize describing the number of bytes successfully sent
		 */
		
		virtual streamsize xsputn (const char* data, streamsize bytes) {
			return send(sockfd, data, bytes, 0);
		}

	private:

		/**
		 * @brief      deleted copy constructor
		 * @param[in]  <unnamed>
		 */
		
		osocketbuf(const osocketbuf&);

		/**
		 * @brief      deleted assignment operator
		 * @param[in]  <unnamed>
		 */
		
		osocketbuf& operator = (const osocketbuf&);
	};

	/**
	 * the output socket stream object
	 * @extends  std::ostream
	 */
	
	class osocketstream : public ostream {

	protected:

		/**
		 * The output socket buffer object wrapped by this stream
		 */

		osocketbuf buf;

	public:

		/**
		 * @brief      constructs an output socket stream to wrap a socket file descriptor; can be used for regular files
		 * @param[in]  sockfd  a file descriptor describing the connecting socket
		 */

		osocketstream(int sockfd): buf(sockfd), ostream(0) {rdbuf(&buf);}

	private:

		
		/**
		 * @brief      deleted copy constructor
		 * @param[in]  <unnamed>
		 */
		
		osocketstream(const osocketstream&);

		/**
		 * @brief      deleted assignment operator
		 * @param[in]  <unnamed>
		 */
		
		osocketstream& operator = (const osocketstream&);

	};
	
	/**
	 * the input socket buffer object
	 * @extends  std::streambuf
	 */
	
	class isocketbuf : public streambuf {

	protected:

		/**
		 * socket file descriptor (sockfd)
		 */
		
		int sockfd;

		/**
		 * putback size in bytes
		 * consumes first part of the buffer
		 * @see http://www.cplusplus.com/reference/istream/istream/putback/
		 */
		
		const size_t pback;

		/**
		 * total buffer size in bytes
		 * usable buffer length is (bsize - pback)
		 */

		const size_t bsize;

		/**
		 * input byte buffer used for putback and data input
		 */

		char* buffer;

	public:

		/**
		 * @brief      constructs an input socket buffer to wrap a socket file descriptor; can be used for regular files
		 * @param[in]  sockfd  a file descriptor describing the connecting socket
		 * @param[in]  pback   putback size in bytes
		 * @param[in]  bsize   total buffer size in bytes
		 */

		isocketbuf(int sockfd, size_t pback = 4, size_t bsize = 64):
			sockfd(sockfd),
			pback(pback),
			bsize(bsize),
			buffer(new char[bsize]) {
			char* start = buffer + pback;
			setg(start, start, start);
		}

		/**
		 * @brief      destroys the buffer and destructs the input socket buffer object
		 */

		~isocketbuf() {
			delete[] buffer;
		}

	protected:

		/**
		 * @brief      controls putback and how data is received from the connecting socket
		 * @return     a streambuf::int_type describing how much input was taken
		 */
		
		virtual int_type underflow() {

			// check if there's still input in the buffer so we can return immediately
			
			if (gptr() < egptr())
				return traits_type::to_int_type(*gptr());

			// if not, refill the buffer's reserved space
			// putback some characters if needed
			
			char* start = buffer + pback;
			
			int pbacks = gptr() - eback();
			if (pbacks > pback)
				pbacks = pback;

			if (pbacks > 0)
				memmove(start - pbacks, gptr() - pbacks, pbacks);

			// fill the buffer with new data
			// recv() will block itself until new data can be received
			
			int received = recv(sockfd, start, 1, 0);

			// error: received < 0
			// EOF:   received == 0 
			
			if (received <= 0) return traits_type::eof();

			// reupdate pointers
			setg(start - pbacks, start, start + received);
			return traits_type::to_int_type(*gptr());

		}

	private:

		
		/**
		 * @brief      deleted copy constructor
		 * @param[in]  <unnamed>
		 */
		
		isocketbuf(const isocketbuf&);

		/**
		 * @brief      deleted assignment operator
		 * @param[in]  <unnamed>
		 */
		
		isocketbuf& operator = (const isocketbuf&);

	};

	/**
	 * the input socket stream object
	 * @extends  std::istream
	 */
	
	class isocketstream : public istream {

	protected:

		/**
		 * The input socket buffer object wrapped by this stream
		 */

		isocketbuf buf;

	public:

		/**
		 * @brief      constructs an input socket stream to wrap a socket file descriptor; can be used for regular files
		 * @param[in]  sockfd  a file descriptor describing the connecting socket
		 */

		isocketstream(int sockfd): buf(sockfd), istream(0) {rdbuf(&buf);}

	private:

		
		/**
		 * @brief      deleted copy constructor
		 * @param[in]  <unnamed>
		 */
		
		isocketstream(const isocketstream&);

		/**
		 * @brief      deleted assignment operator
		 * @param[in]  <unnamed>
		 */
		
		isocketstream& operator = (const isocketstream&);

	};

}

#endif /* __INCLUDE_NET_SOCKETSTREAM__ */