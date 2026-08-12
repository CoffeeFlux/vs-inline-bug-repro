// Crashes the MSVC front end: fatal error C1001 (compiler file 'msc1.cpp',
// line 1589), cl.exe exiting with 0xC0000005. Reproduced on cl 19.44.35228
// (VS 2022 17.14, toolset 14.44) and on the latest cl 19.51.36252 (VS 2026,
// toolset 14.51) with `cl /std:c++20 /EHsc /c repro.cpp` at any optimization
// level.
//
// Found in https://github.com/TypesettingTools/Aegisub/pull/666.

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
		switch (sfs::status(agi::fs::path(path)).type()) { // C1001 on this line
			case not_found: return nullptr;
			case regular:   return "file";
			case directory: return "directory";
			default:        return "other";
		}
	}();
}
