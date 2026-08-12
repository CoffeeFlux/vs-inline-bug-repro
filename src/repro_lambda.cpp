// Bisection variant B: minimal derived class, but keeps the full call
// machinery from repro_full.cpp — the wrap() error-trampoline template, the
// [=] lambda with an explicit return type, and `using enum` inside the
// lambda. The parameter is named `p` so it does not shadow the class name.
//
// Isolates whether the template/lambda machinery is what triggers the ICE.

#include <cstring>
#include <exception>
#include <filesystem>

#ifdef _MSC_VER
#define agi_strdup _strdup
#else
#define agi_strdup strdup
#endif

namespace sfs = std::filesystem;

class path : public sfs::path {
public:
	path(const char *c_str) : sfs::path(reinterpret_cast<const char8_t *>(c_str)) {}
};

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

const char *get_mode(const char *p, char **err) {
	return wrap(err, [=]() -> const char * {
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
	});
}
