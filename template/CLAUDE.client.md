# Working agreement ({{project-name}})

- I write the code in `src/` and the CMake build (`CMakeLists.txt`, `CMakePresets.json`). Never write or modify code there unless my message explicitly asks for that specific change; a grant never carries over.
   - When I do grant it, write the result into `proposals/`, never into `src/` or the CMake build. I will move it myself.
   - Always fine: explain, review, answer questions, write Doxygen and regular comments (also directly in `src/` and `CMakeLists.txt`, through the comment-only hook), write markdown in `docs/`.
   - For teaching, show short snippets in chat only; never write them into files.
   - If you spot a bug, tell me; don't fix it. If I ask for a patch, say where it goes (a couple of lines of context before and after) and give it to me to type.
- When `CLAUDE.md` or the CMake build needs a change (stale facts, a new build mode, a new rule), say so and give exact update instructions: where it goes, the old text and the new text. Short ones in chat, longer ones in `proposals/`.
- Claude writes `docs/` (`docs/Doxyfile` included) and `README.md`.

## third-party/: nobody edits it
- These rules apply to sessions started at this project's root. A session started in `third-party/core2` is a core2 session: core2's `CLAUDE.md`, settings and hook govern it, and it follows core2's `docs/workflow.md` ("Working from a client's submodule"). This project commits the pin only at a core2 release tag, never at a branch commit.
- `third-party/core2` is core2, a git submodule pinned to a release tag{% if ui == "imgui" %}; `third-party/imgui` is Dear ImGui, pinned the same way{% endif %}. Never edit files there, not even comments.
- A change needed in core2 goes through core2's own repo and gate (branch, tests, review, release, tag). Say what is needed; don't work around it here.

## core2's code (`third-party/core2`): never, and no grant opens it
core2 protects these in its own repo, and they stay protected here:
- `third-party/core2/src/`, `examples/`, `tools/`, `bench/support/` and its CMake build (`CMakeLists.txt`, `CMakePresets.json`, `cmake/`): the author's code in core2. Never write or modify them, not even comments or Doxygen. A grant to change code in this project never covers them.
- `third-party/core2/tests/`: Claude writes tests only in a core2 session (started in `third-party/core2`, or a standalone clone), under core2's rules, never from a session of this project.
- `third-party/core2/CLAUDE.md`, `docs/style.md` and `.claude/` (the settings and the comment-only hook this project runs): core2's working agreement, style and guard. Read them; never change them from here.
- A bug spotted in core2 is reported, with where it is; it is not fixed here and not worked around in `src/`. A fix happens in core2's repo, and then this project bumps its pin.
- `.claude/settings.json` enforces this: `deny` for the edit tools on all of `third-party/`, with core2's protected paths listed by name, and `denyWrite` for Bash on `./third-party`, plus core2's own (`src`, `tests`, `CMakeLists.txt`, `CMakePresets.json`, `cmake`, `bench/support`) by name.
- Then I bump the pin: `git -C third-party/core2 fetch --tags && git -C third-party/core2 checkout vX.Y.Z && git add third-party/core2`, rebuild, and commit it as `chore(deps): core2 vX.Y.Z`. I run the fetch and the checkout (the sandbox blocks `github.com:22` and writes under `./third-party`); Claude checks the result with `git -C third-party/core2 status --short` and `describe --tags`. How a core2 fix found here goes through core2: `docs/core2.md`.
- If `third-party/core2` is empty, the comment-only hook (which lives there) blocks every Edit and Write, and CMake stops. Tell me to run `git submodule update --init`.

## Code style
The style rules (language, modules and namespaces, naming, performance, using core2, Doxygen) come from the pinned core2, so they follow its version:

@third-party/core2/docs/style.md

In this project, modules use `{{project-name | snake_case}}::<name>` namespaces with private helpers in `detail`.

## Project
- Build: CMake (`CMakeLists.txt`, `CMakePresets.json`), with core2 through `add_subdirectory(third-party/core2)` and the target `core2::core2`, which carries core2's required flags (C++26, `-fno-exceptions -fno-rtti -nostdinc++ -nostdlib++`: no C++ standard library headers or runtime; C headers only). The project compiles as `gnu++26`, CMake's default (core2's D43). `-DNDEBUG` comes only from the presets' build types, never from a target: a mismatch fails to link, on `core2_built_with_NDEBUG` or `core2_built_without_NDEBUG` (core2's D44).
- Modes: the presets `debug`, `release`, `lto`, `instrument`. Binary: `build/cmake/<preset>/{{project-name}}`.
- Sources: `src/**/*.cpp` (each may `import core;`) and the project's modules `src/**/*.cppm` (`export module <name>;`, a `CXX_MODULES` file set; `clang-scan-deps` finds their imports).
{%- if ui == "plain" %}
- UI: plain console, through `base::console`.
{%- elsif ui == "ncurses" %}
- UI: ncurses (`ncursesw`, through pkg-config's imported target `PkgConfig::ui`). curses owns the terminal between `initscr` and `endwin`: no `console::print` in between. Without a terminal on stdin and stdout, `main` prints through core2 instead.
{%- elsif ui == "sdl3" %}
- UI: SDL3 with SDL_Renderer (`sdl3`, through pkg-config's imported target `PkgConfig::ui`). SDL's C headers are used as-is.
{%- else %}
- UI: Dear ImGui with the {{imgui_backend}} backends, in `third-party/imgui` (pinned like core2, never edited). `CMakeLists.txt` builds it as the `imgui` target, with its own flags (none of core2's, no warnings, `-fno-threadsafe-statics`); the project's sources see its headers as system headers. Platform libraries come through pkg-config's imported target `PkgConfig::ui`. `imgui.ini` (window positions) is git-ignored.
{%- endif %}
- IDE: clangd reads `compile_commands.json` (git-ignored) at the root, a link to `build/cmake/debug/compile_commands.json`, which every configure rewrites; build once so its module files exist.
- Docs: Doxygen, `docs/Doxyfile`; the `docs` target writes `docs/html/` (git-ignored) and fails on any warning, an undocumented public name included.
- No tests yet.

## What Claude may run
- `cmake --preset <p>` and `cmake --build --preset <p>` for `debug`, `release`, `lto` and `instrument`, and `cmake --build --preset debug --target docs`. Nothing else from the build (no `clean` unless asked).
- Run the binary (`./build/cmake/debug/{{project-name}}`) when that shows something useful{% if ui != "plain" %}; it needs a terminal or a display, so usually only build it{% endif %}.
- A build failure is reported, never fixed by touching `src/` or the CMake build.

## Git
- Feature branches (`feature/<topic>`, `fix/<topic>`, `docs/<topic>`, `chore/<topic>`), Conventional Commits, local commits when I say go.
- Stage named files only, never `git add -A`. No Co-Authored-By line.
- Claude does not push: the sandbox blocks `github.com:22`. I push and merge.
- The sandbox mounts `/dev/null` over `.bashrc`, `.zshrc`, `.idea` and similar paths, so they can show up as untracked in Claude's `git status`. Ignore them.
