# vs-inline-bug-repro

Standalone reproduction attempt for an MSVC internal compiler error hit while
fixing a Unicode path bug in Aegisub:
[TypesettingTools/Aegisub#666](https://github.com/TypesettingTools/Aegisub/pull/666).

## The bug

With a class derived from `std::filesystem::path` (Aegisub's UTF-8-converting
`agi::fs::path`), MSVC 19.44 (Visual Studio 2022 17.14, toolset 14.44)
reportedly crashes with an internal compiler error when the conversion,
`std::filesystem::status()` call, and `.type()` call are written inline in a
switch condition:

```cpp
switch (sfs::status(agi::fs::path(path)).type()) { // ICE in MSVC 19.44
```

Hoisting the call into a local variable avoids the crash:

```cpp
auto st = sfs::status(agi::fs::path(path));
switch (st.type()) { // fine
```

That hoist is the workaround that shipped in the Aegisub PR (see the comment in
[`libaegisub/lua/modules/lfs.cpp`](https://github.com/TypesettingTools/Aegisub/blob/master/libaegisub/lua/modules/lfs.cpp)).

## Layout

| File | Contents | Expectation on affected toolsets |
| --- | --- | --- |
| `src/repro_full.cpp` | Near-verbatim copy of the Aegisub code shape: the real `agi::fs::path` class, the `wrap()` error-trampoline template, the `[=]` lambda, `using enum`, and the inline switch condition | 💥 fatal error C1001 |
| `src/repro_minimal.cpp` | Reduced version: minimal derived class, plain function, inline switch condition | Unknown — tells us how small the trigger is |
| `src/workaround.cpp` | Identical to `repro_full.cpp` except the `status()` call is hoisted into a local (the merged fix) | ✅ compiles (control) |

## CI matrix

`.github/workflows/repro.yml` compiles each file with
`cl /std:c++20 /EHsc /W4 /c`, at both default optimization and `/O2 /Zi`
(Aegisub builds as meson `debugoptimized`), across:

| Lane | Runner | Toolset |
| --- | --- | --- |
| `vs2022-toolset-14.44` | `windows-2022` (VS 2022 17.14) | 14.44 — the reported ICE toolset |
| `vs2026-toolset-14.44` | `windows-2025-vs2026` | 14.44 as serviced on the VS 2026 image |
| `vs2026-toolset-latest` | `windows-2025-vs2026` | newest installed toolset |

A red compile step means `cl.exe` failed on that file in that lane; the step
log and the job summary table state whether the failure was an internal
compiler error. `workaround.cpp` is the control and must stay green everywhere.
Full compiler output is uploaded as a `logs-<lane>` artifact.

Note that a green run does **not** prove the original report wrong: MSVC
servicing releases within the 19.44 family differ, and the ICE may depend on
compiler state that this reduction fails to retain.

## Reproducing locally

From an *x64 Native Tools Command Prompt for VS 2022*:

```bat
cl /nologo /std:c++20 /EHsc /W4 /c src\repro_full.cpp
cl /nologo /std:c++20 /EHsc /W4 /c src\workaround.cpp
```
