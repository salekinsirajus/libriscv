#pragma once
#include <map>
#include "types.hpp"

namespace riscv {

struct FileDescriptors
{
	int assign(int);
	int get(int);
	int translate(int);
	int close(int);

	~FileDescriptors();

	std::map<int, int> translation;
	int counter = 0x1000;
};

inline FileDescriptors::~FileDescriptors()
{
	// Close all the real FDs
	for (const auto& it : translation) {
		close(it.second);
	}
}

inline int FileDescriptors::assign(int real_fd)
{
	const int virtfd = counter++;
	translation[virtfd] = real_fd;
	return virtfd;
}
inline int FileDescriptors::get(int virtfd)
{
	auto it = translation.find(virtfd);
	if (it != translation.end()) return it->second;
	return -EBADF;
}
inline int FileDescriptors::translate(int virtfd)
{
	auto it = translation.find(virtfd);
	if (it != translation.end()) return it->second;
	return virtfd;
}
inline int FileDescriptors::close(int virtfd)
{
	auto it = translation.find(virtfd);
	if (it != translation.end()) {
		// Remove the virt FD
		translation.erase(it);
		// Close the real FD
		close(it->second);
		return it->second;
	}
	return -EBADF;
}

} // riscv