// Minimization: repro_lambda_shadow.cpp without `using enum` — the case
// labels are fully qualified instead. Tests whether the C++20 using-enum
// declaration inside the lambda participates in the crash.

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
		switch (sfs::status(agi::fs::path(path)).type()) {
			case sfs::file_type::not_found: return nullptr;
			case sfs::file_type::regular:   return "file";
			case sfs::file_type::directory: return "directory";
			default:                        return "other";
		}
	});
}
