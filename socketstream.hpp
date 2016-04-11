/**
 * C++ I/O stream classes that wrap a file descriptor (fd).
 * @author      Rico Tiongson 
 */

#include <streambuf>
#include <istream>
#include <ostream>
namespace net {
	using namespace std;

	// output file descriptor stream buffer
	class ofdbuf : public streambuf {
		protected:
			int fd;
		public:
			ofdbuf(int fd): fd(fd) {}
			~ofdbuf() {sync();}
			virtual int overflow(int c) {return c != EOF && write(fd, &c, 1) <= 0 ? EOF : c;}
			virtual streamsize xsputn (const char* s, streamsize num) {return write(fd, s, num);}

		private:
			ofdbuf(const ofdbuf&);
			ofdbuf& operator= (const ofdbuf&);
	};

	// input file descriptor stream buffer
	class ifdbuf : public streambuf {
		protected:
			int fd, pback, bsize;
			char* buffer;
		public:
			ifdbuf(int fd, int pback=4, int bsize=64): fd(fd), pback(pback), bsize(bsize), buffer(new char[bsize]) {
				char* start = buffer + pback;
				setg(start, start, start);
			}
			~ifdbuf() {
				delete[] buffer;
			}
		protected:
			virtual int underflow() {
				if (gptr() < egptr())
					return traits_type::to_int_type(*gptr());
				int pbacks = gptr() - eback();
				char* start = buffer + pback;
				if (pbacks > pback)
					pbacks = pback;
				if (pbacks)
					memmove(start - pbacks, gptr() - pbacks, pbacks);
				int received = read(fd, buffer + pback, 1);
				if (received <= 0) return EOF;
				setg(start - pbacks, start, start + received);
				return traits_type::to_int_type(*gptr());
			}
		private:
			ifdbuf(const ifdbuf&);
			ifdbuf& operator= (const ifdbuf&);
	};

	// output file descriptor stream
	class osocketstream : public ostream {
		protected:
			ofdbuf buf;
		public:
			osocketstream(int fd): buf(fd), ostream(0) {rdbuf(&buf);}
		private:
			osocketstream(const osocketstream&);
			osocketstream& operator= (const osocketstream&);
	};

	// input file descriptor stream
	class isocketstream : public istream {
		protected:
			ifdbuf buf;
		public:
			isocketstream(int fd): buf(fd), istream(0) {rdbuf(&buf);}
		private:
			isocketstream(const isocketstream&);
			isocketstream& operator= (const isocketstream&);
	};

}