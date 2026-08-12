# vs-inline-bug-repro

Standalone reproduction of an MSVC internal compiler error hit while fixing a
Unicode path bug in Aegisub:
[TypesettingTools/Aegisub#666](https://github.com/TypesettingTools/Aegisub/pull/666).

**Status: reproduced and minimized.** The ICE fires in every tested toolset,
including the newest VS 2026 compiler — it is not fixed as of MSVC 19.51.

## The bug

With a class derived from `std::filesystem::path` (Aegisub's UTF-8-converting
`agi::fs::path`), MSVC crashes with `fatal error C1001` when a
`std::filesystem::status()` call on an inline-constructed temporary appears in
an `if`/`switch` **condition** inside a **lambda** whose enclosing function has
a **parameter named `path`** — the same name as the class, which the captured
variable shadows inside `agi::fs::path(path)`:

```cpp
switch (sfs::status(agi::fs::path(path)).type()) { // C1001
```

The diagnostic, identical on every affected toolset:

```text
fatal error C1001: Internal compiler error.
(compiler file 'msc1.cpp', line 1589)
```

`msc1.cpp` is the compiler front end; cl.exe dies with exit code -1073741819
(0xC0000005, access violation). Optimization level is irrelevant.

Hoisting the call into a local variable — the fix that shipped in the Aegisub
PR — avoids the crash:

```cpp
auto st = sfs::status(agi::fs::path(path));
switch (st.type()) { // fine
```

## Minimal reproducer

`src/min_iife.cpp`, 26 lines, crashes cl 19.44 through 19.51:

```cpp
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
		switch (sfs::status(agi::fs::path(path)).type()) {
			case not_found: return nullptr;
			case regular:   return "file";
			case directory: return "directory";
			default:        return "other";
		}
	}();
}
```

## Confirmed results

Three CI lanes, all agreeing on every variant across four runs:

| Lane | Compiler |
| --- | --- |
| `vs2022-toolset-14.44` | cl 19.44.35228, toolset 14.44.35207 (VS 2022 17.14 image) |
| `vs2026-toolset-14.44` | toolset 14.44.35207 as installed on the VS 2026 image |
| `vs2026-toolset-latest` | cl 19.51.36252, toolset 14.51.36231 (VS 2026) |

Ingredient analysis — each variant differs from the crashing shape in exactly
the listed way:

| Variant | Isolates | Result |
| --- | --- | --- |
| `repro_full.cpp` | near-verbatim Aegisub shape (line 122 = the switch condition) | 💥 C1001 |
| `workaround.cpp` | same, but `status()` hoisted into a local (the shipped fix) | ✅ |
| `repro_minimal.cpp` | no lambda, no shadowing, minimal class | ✅ |
| `repro_fullclass.cpp` | full class surface alone (plain function, no shadow) | ✅ |
| `repro_lambda.cpp` | wrap template + lambda alone (no shadow) | ✅ |
| `repro_shadow.cpp` | `path` parameter shadowing the class name alone (no lambda) | ✅ |
| `repro_lambda_shadow.cpp` | lambda + shadowing combined | 💥 C1001 |
| `min_iife.cpp` | drops the `wrap()` template — lambda invoked directly | 💥 C1001 |
| `min_if.cpp` | `if` condition instead of `switch` | 💥 C1001 |
| `min_expr.cpp` | same expression as a discarded expression statement | ✅ |
| `min_noenum.cpp` | qualified case labels, no `using enum` | 💥 C1001 |
| `tiny_ref.cpp` | `[&]` capture instead of `[=]` | 💥 C1001 |
| `tiny_fs_empty.cpp` | `.empty()` on the temporary — no `status()` call | ✅ |
| `tiny_nofs.cpp` | no `<filesystem>` — structurally identical custom class | ✅ |

Every ingredient below is required; removing any one of them makes the file
compile:

1. **A lambda** (capture mode irrelevant; a plain function is fine).
2. **A condition context** — `if` or `switch`. The same full expression as a
   plain expression statement or a declaration's initializer is fine.
3. **The nested call shape** `sfs::status(agi::fs::path(path)).type()` with
   the class derived from `std::filesystem::path` and its name shadowed by the
   captured variable. Calling a member directly on the temporary instead of
   passing it through `status()` is fine, and a structurally identical custom
   class with no `<filesystem>` involvement is fine.

## CI

`.github/workflows/repro.yml` compiles every variant with
`cl /std:c++20 /EHsc /W4 /c` (the original repro/workaround pair additionally
at `/O2 /Zi`; Aegisub builds as meson `debugoptimized`). A red step means
cl.exe failed on that file; the job summary table on each run classifies each
failure as ICE or ordinary error, and full compiler output is uploaded as a
`logs-<lane>` artifact.

## Reproducing locally

From an *x64 Native Tools Command Prompt for VS 2022*:

```bat
cl /nologo /std:c++20 /EHsc /W4 /c src\min_iife.cpp     &:: crashes
cl /nologo /std:c++20 /EHsc /W4 /c src\workaround.cpp   &:: compiles
```
