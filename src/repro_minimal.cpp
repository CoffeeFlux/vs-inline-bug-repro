// MSVC ICE reproducer — reduced version.
//
// Strips repro_full.cpp down to what looks like the essential ingredients:
// a class derived from std::filesystem::path with a char8_t-converting
// constructor, constructed inline inside a switch condition wrapping
// std::filesystem::status(...).type().
//
// No wrapper template, no lambda, minimal derived class. If repro_full.cpp
// crashes the compiler and this file does not, the trigger depends on the
// surrounding lambda/template machinery, which narrows future bisection.

#include <filesystem>

namespace sfs = std::filesystem;

class path : public sfs::path {
public:
	path(const char *c_str) : sfs::path(reinterpret_cast<const char8_t *>(c_str)) {}
};

const char *get_mode(const char *p) {
	using enum sfs::file_type;
	switch (sfs::status(path(p)).type()) {
		case not_found: return nullptr;
		case regular:   return "file";
		case directory: return "directory";
		case symlink:   return "link";
		case block:     return "block device";
		case character: return "char device";
		case fifo:      return "fifo";
		case socket:    return "socket";
		default:        return "other";
	}
}
