# Repository Guidelines

## Project Structure & Module Organization
OrcaSlicer’s C++17 sources live in `src/`, split by feature modules and platform adapters. User assets, icons, and printer presets are in `resources/`; translations stay in `localization/`. Tests sit in `tests/`, grouped by domain (`libslic3r/`, `sla_print/`, etc.) with fixtures under `tests/data/`. CMake helpers reside in `cmake/`, and longer references in `doc/` and `SoftFever_doc/`. Automation scripts belong in `scripts/` and `tools/`. Treat everything in `deps/` and `deps_src/` as vendored snapshots—do not modify without mirroring upstream tags.

## Build, Test, and Development Commands
Use out-of-source builds:
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` configures dependencies and generates build files.
- `cmake --build build --target OrcaSlicer --config Release` compiles the app; add `--parallel` to speed up.
- `cmake --build build --target tests` then `ctest --test-dir build --output-on-failure` runs automated suites.
Platform helpers such as `build_linux.sh`, `build_release_macos.sh`, and `build_release_vs2022.bat` wrap the same flow with toolchain flags. Use `build_release_macos.sh -sx` when reproducing macOS build issues, and `scripts/DockerBuild.sh` for reproducible container builds.

## Coding Style & Naming Conventions
`.clang-format` enforces 4-space indents, a 140-column limit, aligned initializers, and brace wrapping for classes and functions. Run `clang-format -i <file>` before committing; the CMake `clang-format` target is available when LLVM tools are on your PATH. Prefer `CamelCase` for classes, `snake_case` for functions and locals, and `SCREAMING_CASE` for constants, matching conventions in `src/`. Keep headers self-contained and align include order with the IWYU pragmas.

## Testing Guidelines
Unit tests rely on Catch2 (`tests/catch2/`). Name specs after the component under test—for example `tests/libslic3r/TestPlanarHole.cpp`—and tag long-running cases so `ctest -L fast` remains useful. Cover new algorithms with deterministic fixtures or sample G-code stored in `tests/data/`. Document manual printer validation or regression slicer checks in your PR when automated coverage is insufficient.

## Commit & Pull Request Guidelines
The history favors concise, sentence-style subject lines with optional issue references, e.g., `Fix grid lines origin for multiple plates (#10724)`. Squash fixups locally before opening a PR. Complete `.github/pull_request_template.md`, include reproduction steps or screenshots for UI changes, and mention impacted presets or translations. Link issues via `Closes #NNNN` when applicable, and call out dependency bumps or profile migrations for maintainer review.

## Security & Configuration Tips
Follow `SECURITY.md` for vulnerability reporting. Keep API tokens and printer credentials out of tracked configs; use `sandboxes/` for experimental settings. When touching third-party code in `deps_src/`, record the upstream commit or release in your PR description and run the relevant platform build script to confirm integration.

## Cursor Cloud specific instructions
This is a single C++/wxWidgets desktop app (`orca-slicer`); there is no backend/web service. The VM snapshot already has system packages installed, the vendored deps prebuilt in `deps/build/`, and the app prebuilt in `build/` — so a normal startup needs no rebuild.

- **Default compiler must be GCC.** This Ubuntu 24.04 image ships with `update-alternatives` pointing `c++`/`cc` at Clang, but Clang here can't find `-lstdc++`, so any CMake configure (deps or app) fails with `cannot find -lstdc++`. The repo build defaults to GCC, so `c++`/`cc` must resolve to `g++`/`gcc` (`sudo update-alternatives --set c++ /usr/bin/g++`, `--set cc /usr/bin/gcc`). This is asserted by the startup update script.
- **Build commands** are the standard ones in `build_linux.sh` / README: `./build_linux.sh -d` (vendored deps, ~30 min), `./build_linux.sh -s` (app), `./build_linux.sh -st` (app + tests). A full from-scratch deps+app build is ~2 hours. Incremental app rebuilds after code edits: `cmake --build build --config Release --target OrcaSlicer`.
- **Built binary:** `build/src/Release/orca-slicer` (also `build/orca-slicer`). It locates resources relative to the executable via the `build/resources -> /workspace/resources` symlink, so run it from the repo without copying resources.
- **Tests:** `ctest --test-dir build -C Release --output-on-failure` (Catch2; 146 cases, ~24 s).
- **Lint:** `clang-format` against `.clang-format` (e.g. `clang-format --dry-run --Werror <file>`). Note much of the existing tree is not clang-format-clean, so only lint files you touch.
- **Running the GUI is headless.** There is no real display, so start a virtual X server and force software OpenGL: `Xvfb :99 -screen 0 1920x1080x24 &` then run with `DISPLAY=:99 LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe MESA_GL_VERSION_OVERRIDE=3.3 ./build/src/Release/orca-slicer --datadir /tmp/orca_data`. For `computerUse`-driven GUI testing, launch the app on display `:1` instead (that is the display the computerUse tooling drives).
- **First-run prompts:** the app shows a "use system SSL certificate" Yes/No dialog and a Setup Wizard. Export `SSL_CERT_FILE=/etc/ssl/certs/ca-certificates.crt` to suppress the SSL prompt; the Setup Wizard still requires picking a printer once (it is saved into `--datadir`).
- **CLI slicing is currently broken on this branch.** `orca-slicer --slice ... --outputdir ...` segfaults inside `Slic3r::CLI::run` immediately, even with a valid display. This is an app bug, not an environment problem (the GUI uses the same init path and works). Use the GUI to slice/validate end-to-end.
