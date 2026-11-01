# Working agreement ({{project-name}})

- I write the code in `src/` and the `Makefile`. Never write or modify code there unless my message explicitly asks for that specific change; a grant never carries over.
   - When I do grant it, write the result into `proposals/`, never into `src/` or the `Makefile`. I will move it myself.
   - Always fine: explain, review, answer questions, write Doxygen and regular comments (also directly in `src/` and the `Makefile`, through the comment-only hook), write markdown in `docs/`.
   - For teaching, show short snippets in chat only; never write them into files.
   - If you spot a bug, tell me; don't fix it. If I ask for a patch, say where it goes (a couple of lines of context before and after) and give it to me to type.
- When `CLAUDE.md` or the `Makefile` needs a change (stale facts, a new build mode, a new rule), say so and give exact update instructions: where it goes, the old text and the new text. Short ones in chat, longer ones in `proposals/`.
- Claude writes `docs/` and `README.md`.

## third-party/: nobody edits it
- These rules apply to sessions started at this project's root. A session started in `third-party/core2` is a core2 session: core2's `CLAUDE.md`, settings and hook govern it, and it follows core2's `docs/workflow.md` ("Working from a client's submodule"). This project commits the pin only at a core2 release tag, never at a branch commit.
- `third-party/core2` is core2, a git submodule pinned to a release tag{% if ui == "imgui" %}; `third-party/imgui` is Dear ImGui, pinned the same way{% endif %}. Never edit files there, not even comments.
- A change needed in core2 goes through core2's own repo and gate (branch, tests, review, release, tag). Say what is needed; don't work around it here.

## core2's code (`third-party/core2`): never, and no grant opens it
core2 protects these in its own repo, and they stay protected here:
- `third-party/core2/src/`, `examples/`, `Makefile` and `core.mk`: the author's code in core2. Never write or modify them, not even comments or Doxygen. A grant to change code in this project never covers them.
- `third-party/core2/tests/`: Claude writes tests only in a core2 session (started in `third-party/core2`, or a standalone clone), under core2's rules, never from a session of this project.
- `third-party/core2/CLAUDE.md`, `docs/style.md` and `.claude/` (the settings and the comment-only hook this project runs): core2's working agreement, style and guard. Read them; never change them from here.
- A bug spotted in core2 is reported, with where it is; it is not fixed here and not worked around in `src/`. A fix happens in core2's repo, and then this project bumps its pin.
- `.claude/settings.json` enforces this: `deny` for the edit tools on all of `third-party/`, with core2's protected paths listed by name, and `denyWrite` for Bash on `./third-party`, plus core2's own three (`src`, `tests`, `Makefile`) by name.
- Then I bump the pin: `git -C third-party/core2 fetch --tags && git -C third-party/core2 checkout vX.Y.Z && git add third-party/core2`, rebuild, and commit it as `chore(deps): core2 vX.Y.Z`. How a core2 fix found here goes through core2: `docs/core2.md`.
- If `third-party/core2` is empty, the comment-only hook (which lives there) blocks every Edit and Write, and `make` stops. Tell me to run `git submodule update --init`.

## Code style
The style rules (language, modules and namespaces, naming, performance, using core2, Doxygen) come from the pinned core2, so they follow its version:

@third-party/core2/docs/style.md

In this project, modules use `{{project-name | snake_case}}::<name>` namespaces with private helpers in `detail`.

## Project
- Build: `Makefile` on core2's `third-party/core2/core.mk`. `clang++ -std=c++26 -fno-exceptions -fno-rtti -nostdinc++ -nostdlib++` (no C++ standard library headers or runtime; C headers only). core.mk compiles core2 with the project's flags.
- Modes: `debug` (default), `release`, `lto`, `instrument`. Binary: `build/{{project-name}}_<mode>`.
- Sources: `src/**/*.cpp` (each may `import core;`) and the project's modules `src/**/*.cppm` (`export module <name>;`, scanned by core.mk like core2's own).
{%- if ui == "plain" %}
- UI: plain console, through `base::console`.
{%- elsif ui == "ncurses" %}
- UI: ncurses (`ncursesw`, through pkg-config). curses owns the terminal between `initscr` and `endwin`: no `console::print` in between. Without a terminal on stdin and stdout, `main` prints through core2 instead.
{%- elsif ui == "sdl3" %}
- UI: SDL3 with SDL_Renderer (`sdl3`, through pkg-config). SDL's C headers are used as-is.
{%- else %}
- UI: Dear ImGui with the {{imgui_backend}} backends, in `third-party/imgui` (pinned like core2, never edited). The Makefile compiles ImGui with its own flags (no `-nostdinc++`, no warnings, `-fno-threadsafe-statics`), and the project's sources see its headers as system headers. Platform libraries come through pkg-config. `imgui.ini` (window positions) is git-ignored.
{%- endif %}
- IDE: clangd reads `compile_commands.json` (git-ignored), made by `make bear`; regenerate it when files are added, renamed or removed.
- No tests yet.

## What Claude may run
- `make`, `make debug`, `make release`, `make lto`, `make instrument`, `make bear`. Nothing else from the Makefile (no `clean` unless asked).
- Run the binary (`./build/{{project-name}}_debug`) when that shows something useful{% if ui != "plain" %}; it needs a terminal or a display, so usually only build it{% endif %}.
- A build failure is reported, never fixed by touching `src/` or the `Makefile`.

## Git
- Feature branches (`feature/<topic>`, `fix/<topic>`, `docs/<topic>`, `chore/<topic>`), Conventional Commits, local commits when I say go.
- Stage named files only, never `git add -A`. No Co-Authored-By line.
- Claude does not push. I push and merge.
- The sandbox mounts `/dev/null` over `.bashrc`, `.zshrc`, `.idea` and similar paths, so they can show up as untracked in Claude's `git status`. Ignore them.
