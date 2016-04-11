namespace net {

	#include <streambuf>	// std::streambuf, traits_type::to_int_type
	#include <cstring>		// memmove
	#include <sys/types.h> 	// ssize_t
	#include <sys/socket.h>	// recv()
	using std::size_t;

	// @see http://www.mr-edd.co.uk/blog/beginners_guide_streambuf
	socketbuf::socketbuf(int sockfd, size_t size, size_t pb):
		sockfd(sockfd),									// the socket file descriptor
		pback(pb ? pb : (size_t) 1),					// the push back length in bytes
		bsize(pback + (size < pback ? pback : size)),	// the size of the buffer in bytes
		buffer(new char[size])							// the actual buffer
	{
		char* end = buffer + bsize;
		setg(end, end, end);
	}

	/**
	 * @brief      receive bytes from connected socket
	 * @return     a std::streambuf::int_type determining the state of the buffer, if it has an underflow or not
	 */
	std::streambuf::int_type socketbuf::underflow() {
		// check if buffer is not yet exhausted
		if (gptr() < egptr())
			return traits_type::to_int_type(*gptr());

		// buffer is exhausted, read data onto buffer
		char* start = buffer;
		if (eback() == base) {
			// make arrangements for putback characters
			std::memmove(buffer, egptr() - pback, pback);
			start += pback;
		}

		// start is now the start of the buffer
		// read from socket onto the buffer
		ssize_t received = recv(sockfd, start, bsize - (start - base), 0);

		// mark end of file when dialoge is closed
		if (received == 0)
			return traits_type::eof();

		// set buffer pointers
		setg(buffer, start, start + received);
		return traits_type::to_int_type(*gptr());
	}

	/**
	 * @brief      send bytes to connected socket
	 * @return     a std::streambuf::int_type determining the state of the buffer, if it has an overflow or not
	 */
	std::streambuf::int_type socketbuf::overflow() {
		
	}

}