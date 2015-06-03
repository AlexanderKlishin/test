#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>

#include <string>
#include <vector>
#include <iostream>
#include <memory>

#define PROG "test"

void usage() {
	std::cerr << PROG " [-h] [-f file_name|-] [-m words|checksum] [-v word]"
		<< "\nexamples:"
		<< "\n\tcat /usr/bin/ls | " PROG " -m checksum"
		<< "\n\t" PROG " -f /usr/bin/ls -m checksum"
		<< "\n\t" PROG " -f /usr/bin/ls -m words -v ls"
		<< "\n\techo \"small ssmall fix small\" | " PROG " -m words -v small"
		<< "\n";
}

struct file_buf {
	char *data; // must be aligned at least to int
	size_t size;
};

class file_reader {
public:
	file_reader() { }
	virtual ~file_reader() { }

	virtual const file_buf *get_next_buf() =0; // sets errno
	virtual bool operator!() const =0;
};

class mmap_file_reader : public file_reader {
public:
	mmap_file_reader(const std::string &fname)
		: fd_(-1), size_(0), blksize_(0), pos_(0), buf_{nullptr, 0} {

		fd_ = open(fname.c_str(), O_RDONLY);
		if (fd_ < 0) {
			perror("open");
			return;
		}

		if (posix_fadvise(fd_, 0, 0, POSIX_FADV_SEQUENTIAL)) {
			perror("posix_fadvice");
			cleanup();
			return;
		}

		struct stat fs;
		if (fstat(fd_, &fs) == -1) {
			perror("fstat");
			cleanup();
			return;
		}
		size_ = fs.st_size;
		blksize_ = fs.st_blksize * 10; // reduce the number of syscalls

		pos_ = -blksize_;
		buf_.size = blksize_;
	}

	bool operator!() const { return fd_ < 0; }

	const file_buf *get_next_buf() {
		if (buf_.data != MAP_FAILED) {
			munmap(buf_.data, buf_.size);
			buf_.data = static_cast<char *>(MAP_FAILED);
		}

		pos_ += blksize_;

		if (pos_ >= size_) {
			pos_ -= blksize_;
			return nullptr;
		}

		size_t rem = size_ - pos_;
		if (rem < blksize_)
			buf_.size = rem; // the last block

		buf_.data = static_cast<char *>(mmap(NULL, buf_.size,
		                                     PROT_READ, MAP_PRIVATE,
		                                     fd_, pos_));

		if (buf_.data == MAP_FAILED) {
			int err = errno;
			perror("mmap");
			errno = err;
			return nullptr;
		}

		return &buf_;
	}

	~mmap_file_reader() { cleanup(); }

private:
	void cleanup() {
		if (buf_.data != MAP_FAILED)
			munmap(buf_.data, buf_.size);

		buf_.data = static_cast<char *>(MAP_FAILED);

		if (fd_ != -1)
			close(fd_);

		fd_ = -1;
	}

private:
	int fd_;
	size_t size_;
	size_t blksize_;
	size_t pos_;

	file_buf buf_;
};

class stdin_file_reader : public file_reader {
public:
	stdin_file_reader() : data_{0}, buf_{data_, 0} { }

	bool operator!() const { return false; }

	const file_buf *get_next_buf() {
		std::cin.read(buf_.data, BUF_SIZE);
		buf_.size = std::cin.gcount();

		return (buf_.size)? &buf_ : nullptr;
	}

private:
	enum { BUF_SIZE = 256 };

	char data_[BUF_SIZE] __attribute__ ((aligned (8)));

	file_buf buf_;
};

struct cmd_opts {
	enum cmd_mode { HELP = 0x00, CHECKSUM, WORDS };

	cmd_opts(int argc, char **argv) : mode(CHECKSUM), fname("-"), word() {
		int opt;
		while ((opt = getopt(argc, argv, "hf:m:v:")) != -1) {
			switch (opt) {
				case 'h':
					usage();
					exit(0);
				case 'f':
					fname.assign(optarg);
					break;
				case 'v':
					word.assign(optarg);
					break;
				case 'm':
					if (strcmp("checksum", optarg) == 0) {
						mode = CHECKSUM;
					} else if (strcmp("words", optarg) == 0) {
						mode = WORDS;
					} else {
						std::cerr << "unsupported option " << optarg << "\n";
						exit(1);
					}
					break;
				default:
					std::cerr << "unsupported option " << opt << "\n";
					exit(1);
			}
		}

		// check parameters consistency
		if (mode == WORDS && word.empty()) {
			std::cerr << "mode 'words' require non empty '-v' option" << "\n";
			exit(1);
		}

		if (fname.empty()) {
			std::cerr << "empty file name" << "\n";
			exit(1);
		}

	}

	int mode = CHECKSUM;
	std::string fname;
	std::string word;
};

uint32_t get_crc(file_reader &f) {
	uint32_t res = 0;

	while (const auto buf = f.get_next_buf()) {
		int r = buf->size % sizeof(res);
		int max = buf->size - r;

		for (int i = 0; i < max; i += sizeof(res))
			res += *(uint32_t*)(buf->data + i);

		// tail
		if (r) {
			uint32_t rem = 0;
			for (int i = 0; i < r; i++)
				((char*)&rem)[i] = buf->data[buf->size - r + i];

			res += rem;
		}
	}

	return res;
}

// Knuth–Morris–Pratt
class look_for_word_kmp {
	class circle_buf {
		unsigned round(unsigned i) {
			i = i - 1;
			i = i | (i >> 1);
			i = i | (i >> 2);
			i = i | (i >> 4);
			i = i | (i >> 8);
			i = i | (i >> 16);
			return i + 1;
		}
	public:
		circle_buf(size_t size)
			: buf_(round(size), 0), head_(0), mask_(buf_.size() - 1) { }

		void put(char c) {
			buf_[head_++] = c;
			head_ &= mask_;
		}
		char get(unsigned i) const {
			size_t p = (head_ - i) & mask_;
			return buf_[p];
		}
	private:
		std::vector<char> buf_;
		unsigned head_;
		const unsigned mask_;
	};

public:
	look_for_word_kmp(const std::string &w)
		: count_(0), word_(w),
		pf_(word_.size(), 0), k_(0), hist_(word_.size() + 1) {

		for (size_t k = 0, i = 1; i < word_.size(); ++i) {
			while(k > 0 && word_[i] != word_[k])
				k = pf_[k-1];

			if (word_[i] == word_[k])
				k++;

			pf_[i] = k;
		}
	}

	void step(char c) {
		if (k_ == word_.size()) {
			if (is_space(hist_.get(word_.size() + 1)) && is_space(c))
				count_++;

			k_ = pf_[k_-1];
		}

		while (k_ > 0 && word_[k_] != c)
			k_ = pf_[k_-1];

		if (word_[k_] == c)
			k_++;

		hist_.put(c);
		return;
	}

	unsigned long get_count() const { return count_; }

private:
	bool is_space(char c) {
		const char wd[] = " \t\n\r()[]{}<>/\\|\"'`~!@#$%^&?*-+=.,;:";

		// c == 0 is also checked
		for (const auto d : wd)
			if (c == d)
				return true;

		return false;
	}

private:
	unsigned long count_;

	const std::string &word_;

	// KMP
	std::vector<int> pf_;
	size_t k_;

	circle_buf hist_;
};

#if 0
class look_for_word {
public:
	look_for_word(const std::string &w) :
		count_(0), word_(w), pos_(0), c_prev_(' ') { }

	void step(char c) {
		if (pos_ == 0) {
			if (word_[pos_] == c && is_space(c_prev_))
				pos_++;

		} else if (pos_ == word_.size()) {
			if (is_space(c))
				count_++;
			pos_ = 0;

		} else if (word_[pos_++] != c) {
			pos_ = 0;
		}

		c_prev_ = c;
	}

	unsigned long get_count() const { return count_; }

private:
	bool is_space(char c) {
		return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0;
	}

private:
	unsigned long count_;

	const std::string &word_;
	size_t pos_;
	char c_prev_;
};
#endif

uint32_t count_words(file_reader &f, const std::string &word) {
	look_for_word_kmp l(word);

	while (const auto buf = f.get_next_buf()) {
		for (size_t i = 0; i < buf->size; i++)
			l.step(buf->data[i]);
	}

	l.step(0); // end of file == space

	return l.get_count();
}

int main(int argc, char **argv) {
	cmd_opts opts(argc, argv);

	std::unique_ptr<file_reader> f;

	if (opts.fname == "-")
		f.reset(new stdin_file_reader());
	else
		f.reset(new mmap_file_reader(opts.fname));

	if (!*f) {
		std::cerr << "cannot open file \"" << opts.fname << "\"\n";
		return 1;
	}

	switch (opts.mode) {
		case cmd_opts::CHECKSUM:
			std::cout << get_crc(*f) << "\n";
			break;
		case cmd_opts::WORDS:
			std::cout << count_words(*f, opts.word) << "\n";
			break;
	}

	return (errno)? 1 : 0;
}

