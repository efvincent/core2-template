# Changelog

All notable changes to the template are documented in this file: what a generated project gets, and how it is generated. Changes to this repo alone (its docs, `check.sh`, its Claude Code settings) are not.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the template adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html), measured from a generated project's side: a patch release fixes it, a minor release adds to it, and a major release is one where `copier update` would leave a project that needs hand work (a renamed question, a moved file the author owns, a build change the author must follow). Releases are git tags `vX.Y.Z`; `copier copy` takes the latest, and `copier update` moves a project between them.

## [Unreleased]

## [1.1.0] - 2026-11-17

### Added
- clang-tidy in core2's style: `.clang-tidy` (naming, trailing return types on functions, initialized locals, bug-prone patterns; nothing that wants the C++ standard library), run by clangd (`--clang-tidy` in `.vscode/settings.json`). `third-party/.clang-tidy` turns it off in the submodules. Claude cannot edit `.clang-tidy`.

### Fixed
- Shift+F5 (CMake Tools' *Debug*) debugs under CodeLLDB: `cmake.debugConfig` in `.vscode/settings.json`. Before, it stopped with "Configured debug type 'cppdbg' is not supported".

## [1.0.0] - 2026-11-17

The first release, and the first as a Copier template (decision 14). Before it, the template was generated with cargo-generate and was not tagged.

### Added
- Generating with Copier: `copier copy --trust gh:efvincent/core2-template <dir>`. The questions are `project_name`, `core2_version` (default `v2.1.8`), `core2_url`, `ui` and `imgui_backend`; `.copier-answers.yml` records them for `copier update`.
- `./dev`, the generated project's one command (decision 15): `build [preset]`, `run [preset] [-- args]`, `docs`, `clean [preset|all]`, `setup`, `help`. It asks for a missing command or preset only at a terminal, and `build`, `run` and `docs` check that the setup is done and offer it.
- `./dev setup`, run by Copier after the copy: the submodules at their pins (a rerun never moves one), core2's local Claude Code settings, the debug build and core2's reference.
- core2's HTML reference: the `core2_docs` target, into `docs/core2/html/`, linked from the README and, through a tag file, from the project's own reference.
- VSCodium setup: `.vscode/extensions.json` (clangd, CMake Tools, CodeLLDB), CMake Tools on the presets, tasks through `./dev`.

### Changed
- clangd reads `build/cmake/debug/compile_commands.json`; before, it looked at the root, where nothing was.
- The generated project's `CLAUDE.md`, `.claude/`, `src/`, the CMake build and `./dev` are denied to Claude's edit tools and Bash; changes to them come as patches, and to `CLAUDE.md` and the settings as patch files in `proposals/`.

### Removed
- `cargo-generate.toml`, `post.rhai` and the self-deleting `setup.sh`.

[Unreleased]: https://github.com/efvincent/core2-template/compare/v1.1.0...HEAD
[1.1.0]: https://github.com/efvincent/core2-template/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/efvincent/core2-template/releases/tag/v1.0.0
