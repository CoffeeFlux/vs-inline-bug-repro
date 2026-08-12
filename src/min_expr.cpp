// Minimization: repro_lambda_shadow.cpp with the offending expression moved
// from the switch condition into a plain discarded expression statement. The
// shipped Aegisub workaround already shows the same subexpression compiles in
// a declaration statement; if this compiles too, the crash is specific to
// condition contexts.

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
		(void)sfs::status(agi::fs::path(path)).type();
		return "other";
	});
}
