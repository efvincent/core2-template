#!/bin/sh
# Generates each variant of the template into a temp dir with Copier (from
# the working tree, uncommitted changes included), whose task runs the
# project's `./dev setup` (submodules, the debug build, core2's reference);
# checks the result, `./dev` itself, and that the setup can be rerun; builds
# every CMake preset through `./dev build` (warnings fail the check); checks
# the debug preset's compile_commands.json and both references; and runs the
# binaries that can run headless.
#
#   ./check.sh plain         the plain variant (the usual check)
#   ./check.sh               every variant (about five minutes)
#   ./check.sh sdl3 ...      only these
#
# Environment:
#   CORE2_DIR      a local core2 clone to use as core2_url, for an offline run or
#                  a tag not pushed yet (default: none, so core2 comes from
#                  core2_url's default in copier.yml, over SSH)
#   CORE2_VERSION  the tag to pin (default: core2_version's default in
#                  copier.yml, which is what gets checked)
#   KEEP=1         keep the temp dir after a pass (it is always kept after a failure)
#   WINDOWED=1     also run the ImGui variants that need a real display
#                  (sdl3_gpu, glfw_opengl3, glfw_vulkan), as run_window does
set -eu

here=$(cd "$(dirname "$0")" && pwd)
# default_of <question>: the `default:` of a question in copier.yml.
default_of() {
  sed -n "/^$1:/,/^\$/s/^  default: *//p" "$here/copier.yml"
}
default_url=$(default_of core2_url)
core2=${CORE2_DIR:-$default_url}
default_version=$(default_of core2_version)
version=${CORE2_VERSION:-$default_version}
all_variants="plain ncurses sdl3 imgui-sdl3_renderer imgui-sdl3_gpu imgui-glfw_opengl3 imgui-glfw_vulkan"
variants=${*:-$all_variants}
modes="debug release lto instrument"

# git refuses to clone a local path as a submodule unless this is allowed.
if [ -n "${CORE2_DIR:-}" ]; then
  export GIT_CONFIG_COUNT=1 GIT_CONFIG_KEY_0=protocol.file.allow GIT_CONFIG_VALUE_0=always
fi

out=$(mktemp -d "${TMPDIR:-/tmp}/core2-template-check.XXXXXX")
log=$out/log

fail() {
  echo "FAIL: $*" >&2
  echo "      output kept in $out" >&2
  exit 1
}

# run <what> <command...>: runs the command with no input and its output in
# $log; on failure shows the log and stops.
run() {
  what=$1
  shift
  if ! "$@" </dev/null >"$log" 2>&1; then
    cat "$log" >&2
    fail "$what"
  fi
}

# run_window [VAR=value...] <bin>: starts a windowed program (it runs until its
# window is closed) in the project's directory, where ImGui writes imgui.ini.
# Waits up to WINDOW_WAIT seconds for the line it prints once its window is set
# up, lets it draw for one more second, then stops it. Fails if it exits on its
# own or never prints the line. Setup can take seconds: under the sanitizers,
# loading the Vulkan drivers is slow.
WINDOW_WAIT=20
run_window() {
  (cd "$dir" && exec env "$@") </dev/null >"$log" 2>&1 &
  pid=$!
  tenths=0
  while kill -0 "$pid" 2>/dev/null && ! grep -q 'to quit$' "$log"; do
    if [ "$tenths" -ge $((WINDOW_WAIT * 10)) ]; then
      kill "$pid" 2>/dev/null; wait "$pid" 2>/dev/null || true
      cat "$log" >&2
      fail "$v: $bin did not get past its window setup in $WINDOW_WAIT seconds"
    fi
    sleep 0.1
    tenths=$((tenths + 1))
  done
  if ! kill -0 "$pid" 2>/dev/null; then
    rc=0; wait "$pid" || rc=$?
    cat "$log" >&2
    fail "$v: $bin exited on its own (status $rc)"
  fi
  sleep 1
  if ! kill -0 "$pid" 2>/dev/null; then
    rc=0; wait "$pid" || rc=$?
    cat "$log" >&2
    fail "$v: $bin exited on its own after its window setup (status $rc)"
  fi
  kill "$pid" 2>/dev/null; wait "$pid" 2>/dev/null || true
  window_note=" (window up after $((tenths / 10)).$((tenths % 10))s)"
}

check_variant() {
  v=$1
  backend=
  case $v in
    plain | ncurses | sdl3) ui=$v ;;
    imgui-sdl3_renderer | imgui-sdl3_gpu | imgui-glfw_opengl3 | imgui-glfw_vulkan)
      ui=imgui
      backend=${v#imgui-}
      ;;
    *) fail "unknown variant $v (known: $all_variants)" ;;
  esac
  name=check-$(printf '%s' "$v" | tr '_' '-')
  dir=$out/$name
  echo "== $v ($name)"

  set -- --data project_name="$name" --data ui="$ui" --data core2_url="$core2" --data core2_version="$version"
  if [ -n "$backend" ]; then set -- "$@" --data imgui_backend="$backend"; fi
  run "$v: copier copy" copier copy --trust --defaults --vcs-ref=HEAD "$@" "$here" "$dir"
  if grep -n -e ': warning:' -e 'CMake Warning' "$log"; then
    fail "$v: copier copy (./dev setup) printed warnings"
  fi
  # ./dev setup (Copier's task) built debug, so the editor works on first open
  [ -x "$dir/dev" ] || fail "$v: ./dev is not executable"
  [ -f "$dir/build/cmake/debug/compile_commands.json" ] || fail "$v: ./dev setup left no build/cmake/debug/compile_commands.json"
  [ -x "$dir/build/cmake/debug/$name" ] || fail "$v: ./dev setup did not build build/cmake/debug/$name"
  grep -q 'compile-commands-dir=${workspaceFolder}/build/cmake/debug' "$dir/.vscode/settings.json" \
    || fail "$v: .vscode/settings.json does not point clangd at build/cmake/debug"

  # The generated tree
  for f in copier.yml ui; do
    [ ! -e "$dir/$f" ] || fail "$v: $f is left over"
  done
  if find "$dir" -path "$dir/third-party" -prune -o -name '*.jinja' -print | grep .; then
    fail "$v: .jinja files left over"
  fi
  [ "$(find "$dir" -maxdepth 1 -name '*src*')" = "$dir/src" ] || fail "$v: not exactly one src/"
  for f in CLAUDE.md README.md CMakeLists.txt CMakePresets.json .gitignore docs/core2.md docs/Doxyfile src/main.cpp \
           dev .copier-answers.yml .claude/settings.json \
           .vscode/settings.json .vscode/launch.json .vscode/tasks.json .vscode/extensions.json; do
    [ -f "$dir/$f" ] || fail "$v: $f is missing"
  done
  if grep -rIn -e '{{' -e '{%' --exclude-dir=.git --exclude-dir=third-party --exclude-dir=src --exclude-dir=build --exclude-dir=core2 "$dir"; then
    fail "$v: unrendered Jinja tags"
  fi
  run "$v: JSON files" python3 -c 'import json, sys; [json.load(open(p)) for p in sys.argv[1:]]' \
    "$dir/CMakePresets.json" "$dir/.claude/settings.json" "$dir/.vscode/settings.json" "$dir/.vscode/launch.json" "$dir/.vscode/tasks.json" \
    "$dir/.vscode/extensions.json"

  # The submodule, its pin, and what CLAUDE.md and the settings take from it
  pinned=$(git -C "$dir/third-party/core2" describe --tags --exact-match 2>/dev/null || true)
  [ "$pinned" = "$version" ] || fail "$v: third-party/core2 is at '$pinned', not $version"
  staged=$(git -C "$dir" diff --cached --name-only)
  for f in .gitmodules third-party/core2; do
    printf '%s\n' "$staged" | grep -qx "$f" || fail "$v: $f is not staged"
  done
  [ -f "$dir/third-party/core2/docs/style.md" ] || fail "$v: third-party/core2/docs/style.md (imported by CLAUDE.md) is missing"
  run "$v: comment-only hook self-test" python3 "$dir/third-party/core2/.claude/hooks/comment_only_guard.py" --selftest
  gitdir=$(git -C "$dir/third-party/core2" rev-parse --absolute-git-dir)
  run "$v: third-party/core2/.claude/settings.local.json" python3 -c '
import json, sys
fs = json.load(open(sys.argv[1]))["sandbox"]["filesystem"]
g = sys.argv[2]
assert fs == { "allowWrite": [g], "denyWrite": [g + "/hooks", g + "/config"] }, fs
' "$dir/third-party/core2/.claude/settings.local.json" "$gitdir"
  [ -z "$(git -C "$dir/third-party/core2" status --porcelain)" ] || fail "$v: third-party/core2 has changes (settings.local.json not ignored?)"

  # core2's reference, from ./dev setup: built, git-ignored, with its tag file
  [ -f "$dir/docs/core2/html/index.html" ] || fail "$v: ./dev setup did not build docs/core2/html/index.html"
  [ -f "$dir/docs/core2/core2.tag" ] || fail "$v: ./dev setup did not write docs/core2/core2.tag"
  git -C "$dir" check-ignore -q docs/core2 || fail "$v: docs/core2 is not git-ignored"

  # ./dev setup again (after a clone, or to repair): succeeds, keeps the pin
  run "$v: ./dev setup rerun" "$dir/dev" setup
  pinned=$(git -C "$dir/third-party/core2" describe --tags --exact-match 2>/dev/null || true)
  [ "$pinned" = "$version" ] || fail "$v: ./dev setup rerun moved third-party/core2 to '$pinned'"

  # ./dev away from a terminal: no command prints the help, never asks
  run "$v: ./dev" "$dir/dev"
  grep -q '^./dev build \[preset\]' "$log" || { cat "$log" >&2; fail "$v: ./dev without a command did not print the help"; }
  if "$dir/dev" build nosuchpreset </dev/null >"$log" 2>&1; then fail "$v: ./dev build nosuchpreset succeeded"; fi
  grep -q "unknown preset 'nosuchpreset'" "$log" || { cat "$log" >&2; fail "$v: no message for an unknown preset"; }

  # Without the submodule, configuring stops with the fix in the message.
  bare=$out/$name-no-submodule
  mkdir "$bare"
  cp -R "$dir/CMakeLists.txt" "$dir/CMakePresets.json" "$dir/src" "$dir/dev" "$bare/"
  if (cd "$bare" && cmake --preset debug) </dev/null >"$log" 2>&1; then
    fail "$v: cmake --preset debug without third-party/core2 succeeded"
  fi
  grep -q 'git submodule update --init' "$log" ||
    { cat "$log" >&2; fail "$v: no submodule message from cmake without third-party/core2"; }
  # and ./dev build stops there (no terminal, so it does not offer), naming the fix
  if "$bare/dev" build </dev/null >"$log" 2>&1; then
    fail "$v: ./dev build without the setup succeeded"
  fi
  grep -q 'third-party/core2 is not checked out' "$log" && grep -q 'run ./dev setup first' "$log" ||
    { cat "$log" >&2; fail "$v: ./dev build without the setup gave no setup message"; }
  rm -rf "$bare"

  # Every preset through ./dev build (configures the first time), warning-free
  # (the compiler's and CMake's)
  for m in $modes; do
    run "$v: ./dev build $m" "$dir/dev" build "$m"
    if grep -n -e 'warning:' -e 'CMake Warning' "$log"; then
      fail "$v: ./dev build $m printed warnings"
    fi
    bin=$dir/build/cmake/$m/$name
    [ -x "$bin" ] || fail "$v: $bin was not built"
    case $ui in
      plain | ncurses)
        # ncurses: no terminal here, so it prints through core2 instead
        run "$v: ./dev run $m" "$dir/dev" run "$m" -- one "two words"
        grep -q ': 2 argument(s)' "$log" && grep -q '^  2: two words$' "$log" ||
          { cat "$log" >&2; fail "$v: unexpected output from $bin"; }
        if [ "$ui" = ncurses ] && command -v script >/dev/null; then
          # and the curses screen, on a pseudo-terminal: a key press ends it
          { sleep 1; printf x; } | TERM=xterm timeout 10 script -qec "'$bin' one 'two words'" /dev/null >"$log" 2>&1 ||
            { cat "$log" >&2; fail "$v: $bin on a pseudo-terminal"; }
          grep -q 'Press any key' "$log" && grep -q 'two words' "$log" ||
            { cat "$log" >&2; fail "$v: unexpected screen from $bin"; }
        fi
        ;;
      sdl3)
        # SDL's offscreen video driver: no window on the desktop.
        run_window SDL_VIDEO_DRIVER=offscreen "$bin"
        ;;
      imgui)
        # SDL_GPU, OpenGL and Vulkan need a surface that can present, which
        # SDL's offscreen driver does not have: those are only built, unless
        # WINDOWED=1 runs them on the real display.
        if [ "$backend" = sdl3_renderer ]; then
          run_window SDL_VIDEO_DRIVER=offscreen "$bin"
        elif [ "${WINDOWED:-}" = 1 ]; then
          run_window "$bin"
        fi
        ;;
    esac
    echo "   $m: ok${window_note:-}"
    window_note=
  done

  run "$v: compile_commands.json" python3 -c '
import json, sys
db = json.load(open(sys.argv[1]))
files = [e["file"] for e in db]
assert any(f.endswith("src/main.cpp") for f in files), "no entry for src/main.cpp"
assert any("third-party/core2/src/" in f for f in files), "no entries for core2"
' "$dir/build/cmake/debug/compile_commands.json"
  echo "   compile_commands.json: ok"

  # The reference: warning-free (the Doxyfile makes warnings fatal), git-ignored,
  # and README.md's link to core2's reference works from it too
  run "$v: ./dev docs" "$dir/dev" docs
  if grep -n -e 'warning:' -e 'error:' "$log"; then
    fail "$v: the docs target printed warnings"
  fi
  [ -f "$dir/docs/html/index.html" ] || fail "$v: docs/html/index.html was not built"
  git -C "$dir" check-ignore -q docs/html || fail "$v: docs/html is not git-ignored"
  grep -q 'href="docs/core2/html/index.html"' "$dir/docs/html/index.html" || fail "$v: docs/html/index.html has no link to core2's reference"
  [ -f "$dir/docs/html/docs/core2/html/index.html" ] || fail "$v: the link to core2's reference does not resolve from docs/html/"
  grep -q 'href="docs/core2/html/index.html"' "$dir/README.md" || grep -q '](docs/core2/html/index.html)' "$dir/README.md" || fail "$v: README.md has no link to core2's reference"
  echo "   docs: ok"
  [ -z "$(git -C "$dir/third-party/core2" status --porcelain)" ] || fail "$v: third-party/core2 has changes after the builds"

  # ./dev clean: one preset's output goes, the others stay
  run "$v: ./dev clean release" "$dir/dev" clean release
  [ ! -e "$dir/build/cmake/release/$name" ] || fail "$v: ./dev clean release left build/cmake/release/$name"
  [ -x "$dir/build/cmake/debug/$name" ] || fail "$v: ./dev clean release removed the debug build"
  echo "   ./dev: ok"
}

# Every name first, so a typo stops the run before anything is built.
for v in $variants; do
  case " $all_variants " in
    *" $v "*) ;;
    *) fail "unknown variant $v (known: $all_variants)" ;;
  esac
done

for v in $variants; do
  check_variant "$v"
done

echo "all passed: $variants (core2 $version from $core2)"
if [ "${KEEP:-}" = 1 ]; then
  echo "kept: $out"
else
  rm -rf "$out"
fi
