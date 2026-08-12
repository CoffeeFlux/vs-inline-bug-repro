// Bisection variant A: the full agi::fs::path class from repro_full.cpp,
// called from a plain function — no wrap() template, no lambda, and the
// parameter is named `p` so it does not shadow the class name.
//
// Isolates whether the class's full member surface (extra constructors,
// string() wrappers, friend operators, WRAP_SFP members) is what triggers
// the ICE.

#include <cstdint>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iterator>
#include <memory>
#include <string>

namespace agi::fs {

class path : public std::filesystem::path {
public:
	path(std::string_view string) : std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t *>(string.data()), string.size())) {}
	path(std::string const& string) : std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t *>(string.data()), string.size())) {}
	path(const char *c_str) : std::filesystem::path(reinterpret_cast<const char8_t *>(c_str)) {}

	path() : std::filesystem::path() {}

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

const char *get_mode(const char *p) {
	using enum sfs::file_type;
	switch (sfs::status(agi::fs::path(p)).type()) {
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
