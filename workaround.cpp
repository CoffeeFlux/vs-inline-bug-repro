// Identical to repro.cpp except the status() call is hoisted out of the
// switch condition into a local variable — the workaround Aegisub shipped.
// Compiles cleanly on every toolset that crashes on repro.cpp.

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
		using enum sfs::file_type;
		auto st = sfs::status(agi::fs::path(path));
		switch (st.type()) {
			case not_found: return nullptr;
			case regular:   return "file";
			case directory: return "directory";
			default:        return "other";
		}
	}();
}
