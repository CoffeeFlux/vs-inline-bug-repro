// Bisection variant B+C: minimal derived class with the full call machinery
// (wrap() template, [=] lambda, using enum) AND the parameter named `path`,
// shadowing the class name inside the lambda — repro_full.cpp with the class
// stripped to a single constructor.
//
// If this crashes while A, B, and C individually compile, the trigger is the
// combination of the lambda machinery and the name shadowing, and this file
// is the small reproducer to report upstream.

#include <cstring>
#include <exception>
#include <filesystem>

#ifdef _MSC_VER
#define agi_strdup _strdup
#else
#define agi_strdup strdup
#endif

namespace sfs = std::filesystem;

namespace agi::fs {
class path : public sfs::path {
public:
	path(const char *c_str) : sfs::path(reinterpret_cast<const char8_t *>(c_str)) {}
};
}

template<typename Func>
auto wrap(char **err, Func f) -> decltype(f()) {
	try {
		return f();
	}
	catch (std::exception const& e) {
		*err = agi_strdup(e.what());
	}
	catch (...) {
		*err = agi_strdup("Unknown error");
	}
	return 0;
}

const char *get_mode(const char *path, char **err) {
	return wrap(err, [=]() -> const char * {
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
	});
}
