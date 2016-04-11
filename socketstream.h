namespace net {

	#include <streambuf>
	using std::size_t;

	class socketbuf : public std::streambuf {
	public:
		// constructor
		explicit socketbuf(int sockfd, size_t size = 128, size_t pback = 8);
	private:
		// members
		const int sockfd;
		const size_t pback;
		const size_t bsize;
		const char* buffer;

		// overrides base class underflow()
		int_type underflow();

		// copy not allowed
		socketbuf (const socketbuf&);
		socketbuf& operator = (const socketbuf&);
	};
}