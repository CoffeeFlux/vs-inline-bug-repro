# vs-inline-bug-repro

Minimal reproduction of an MSVC front-end internal compiler error, found while
fixing a Unicode path bug in Aegisub:
[TypesettingTools/Aegisub#666](https://github.com/TypesettingTools/Aegisub/pull/666).

## The bug

[`repro.cpp`](repro.cpp) crashes cl 19.44.35228 (VS 2022 17.14, toolset 14.44)
through cl 19.51.36252 (VS 2026, toolset 14.51), at any optimization level:

```text
repro.cpp(22): fatal error C1001: Internal compiler error.
(compiler file 'msc1.cpp', line 1589)
```

cl.exe exits with -1073741819 (0xC0000005, access violation). From an
*x64 Native Tools Command Prompt*:

```bat
cl /nologo /std:c++20 /EHsc /W4 /c repro.cpp        &:: C1001
cl /nologo /std:c++20 /EHsc /W4 /c workaround.cpp   &:: compiles
```

[`workaround.cpp`](workaround.cpp) is identical except the `status()` call is
hoisted out of the switch condition into a local variable — the fix Aegisub
shipped.

## Required ingredients

Bisected on CI across toolsets 14.44 and 14.51 (the variant matrix lives in
this repo's git history); removing any one of these makes the file compile:

1. **A lambda.** Capture mode is irrelevant; a plain function is fine.
2. **A condition context** — `if` or `switch`. The identical expression
   compiles as a plain expression statement or a declaration's initializer,
   which is why the workaround works.
3. **The nested call** `sfs::status(agi::fs::path(path)).type()` with a class
   derived from `std::filesystem::path` whose name is shadowed by the captured
   parameter `path`. Renaming the parameter compiles; a structurally identical
   custom class without `<filesystem>` compiles; calling a member directly on
   the temporary instead of passing it through `status()` compiles.

## CI

Two lanes: `windows-2022` pinned to toolset 14.44 (as originally reported) and
`windows-2025-vs2026` on its newest toolset. The `repro.cpp` step is expected
to fail — a red ✗ there means the bug is still present in that toolset, so the
`msvc-latest` lane doubles as a fix tracker.
