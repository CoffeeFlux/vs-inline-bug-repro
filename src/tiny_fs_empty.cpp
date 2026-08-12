// Minimization: keeps the class derived from std::filesystem::path but drops
// the sfs::status() call — the temporary's own .empty() member is called in
// the if condition instead. Tests whether the status() free-function call is
// a required ingredient.

#include <filesystem>

namespace sfs = std::filesystem;

namespace agi::fs {
class path : public sfs::path {
public:
	path(const char *c_str) : sfs::path(reinterpret_cast<const char8_t *>(c_str)) {}
};
}

const char *get_mode(const char *path) {
	return [=]() -> const char * {
		if (agi::fs::path(path).empty())
			return nullptr;
		return "other";
	}();
}
