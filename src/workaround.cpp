// MSVC ICE reproducer — workaround version (control).
//
// Byte-for-byte identical to repro_full.cpp except for one change inside
// get_mode(): the sfs::status(...) call is hoisted into a local variable
// instead of being written inline in the switch condition. This is the fix
// that was merged in https://github.com/TypesettingTools/Aegisub/pull/666.
//
// Expected: compiles cleanly on every toolset, including ones where
// repro_full.cpp produces an internal compiler error.

#include <cstdint>
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iterator>
#include <memory>
#include <string>

#ifdef _MSC_VER
#define agi_strdup _strdup
#else
#define agi_strdup strdup
#endif

#undef CreateDirectory

namespace agi::fs {

/// @class agi::fs::path
/// @brief Wrapper class around std::filesystem::path that properly handles
/// charset conversions.
///
/// Takes UTF-8 strings in its constructor and returns UTF-8 strings
/// in string(). Should be used everywhere instead of std::filesystem::path.
///
class path : public std::filesystem::path {
public:
	path(std::string_view string) : std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t *>(string.data()), string.size())) {}
	path(std::string const& string) : std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t *>(string.data()), string.size())) {}
	path(const char *c_str) : std::filesystem::path(reinterpret_cast<const char8_t *>(c_str)) {}

	path() : std::filesystem::path() {}

	// These are marked as explicit so that there is no way to accidentally go
	// from string to std::filesystem::path to agi::fs::path by implicit conversions
	explicit path(std::filesystem::path const& inner) : std::filesystem::path(inner) {}
	explicit path(std::filesystem::path &&inner) : std::filesystem::path(std::move(inner)) {}

	inline std::string string() const {
		const auto result = std::filesystem::path::u8string();
		return std::string(reinterpret_cast<const char *>(result.c_str()), result.size());
	}

	inline std::string generic_string() const {
		const auto result = std::filesystem::path::generic_u8string();
		return std::string(reinterpret_cast<const char *>(result.c_str()), result.size());
	}

	inline friend path operator/(path const& lhs, path const& rhs) {
		const std::filesystem::path &lhs_ = lhs;
		const std::filesystem::path &rhs_ = rhs;
		return path(lhs_ / rhs_);
	}

	template <typename C, typename T>
	inline friend std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T> &ostr, path const& rhs) {
		ostr << std::quoted(rhs.string());
		return ostr;
	}

#define WRAP_SFP(name) \
	inline path name() const { \
		return path(std::filesystem::path::name()); \
	}

	WRAP_SFP(root_name);
	WRAP_SFP(root_directory);
	WRAP_SFP(root_path);
	WRAP_SFP(relative_path);
	WRAP_SFP(parent_path);
	WRAP_SFP(filename);
	WRAP_SFP(stem);
	WRAP_SFP(extension);

	inline path& make_preferred() {
		std::filesystem::path::make_preferred();
		return *this;
	};
};

} // namespace agi::fs

namespace sfs = std::filesystem;

// Mirrors the error-trampoline wrapper from lfs.cpp (agi::Exception catch
// clause dropped; it is not needed to exercise the compiler).
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
		// Not inlined into the switch because it causes an ICE in MSVC 19.44.
		auto st = sfs::status(agi::fs::path(path));
		switch (st.type()) {
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
