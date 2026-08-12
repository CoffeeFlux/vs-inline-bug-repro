// Minimization: min_iife.cpp with by-reference capture instead of by-value.
// Tests whether the capture mode matters.

#include <filesystem>

namespace sfs = std::filesystem;

namespace agi::fs {
class path : public sfs::path {
public:
	path(const char *c_str) : sfs::path(reinterpret_cast<const char8_t *>(c_str)) {}
};
}

const char *get_mode(const char *path) {
	return [&]() -> const char * {
		using enum sfs::file_type;
		switch (sfs::status(agi::fs::path(path)).type()) {
			case not_found: return nullptr;
			case regular:   return "file";
			case directory: return "directory";
			default:        return "other";
		}
	}();
}
