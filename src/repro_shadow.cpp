// Bisection variant C: minimal derived class, plain function, but the
// function parameter is named `path` — the same name as the class — exactly
// as in the original Aegisub code, where the argument inside
// agi::fs::path(path) is the parameter shadowing the type name.
//
// Isolates whether that name reuse is what triggers the ICE.

#include <filesystem>

namespace sfs = std::filesystem;

namespace agi::fs {
class path : public sfs::path {
public:
	path(const char *c_str) : sfs::path(reinterpret_cast<const char8_t *>(c_str)) {}
};
}

const char *get_mode(const char *path) {
	using enum sfs::file_type;
	switch (sfs::status(agi::fs::path(path)).type()) {
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
