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

## Confirmed results (run 1, 2026-08-11)

The ICE **reproduces in CI, in every lane, at both optimization levels** —
including the newest VS 2026 toolset, so this is not fixed as of MSVC 19.51:

| Lane | Compiler | `workaround.cpp` | `repro_minimal.cpp` | `repro_full.cpp` |
| --- | --- | --- | --- | --- |
| `vs2022-toolset-14.44` | cl 19.44.35228 (toolset 14.44.35207) | ✅ | ✅ | 💥 C1001 |
| `vs2026-toolset-14.44` | toolset 14.44.35207 on the VS 2026 image | ✅ | ✅ | 💥 C1001 |
| `vs2026-toolset-latest` | cl 19.51.36252 (toolset 14.51.36231) | ✅ | ✅ | 💥 C1001 |

The diagnostic in every failing lane:

```text
src/repro_full.cpp(122): fatal error C1001: Internal compiler error.
(compiler file 'msc1.cpp', line 1589)
```

`msc1.cpp` is the compiler front end, matching the observation that `/Od` vs
`/O2 /Zi` makes no difference. cl.exe exits with -1073741819 (0xC0000005,
access violation). Line 122 is the inline switch condition.

Since `repro_minimal.cpp` compiles everywhere, the trigger lives in something
the reduction dropped. The `src/repro_fullclass.cpp` (A: full class surface),
`src/repro_lambda.cpp` (B: wrap template + lambda), `src/repro_shadow.cpp`
(C: parameter named `path` shadowing the class name), and
`src/repro_lambda_shadow.cpp` (B+C) variants each isolate one suspect
ingredient.

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
