#!/bin/sh
# Generates each variant of the template into a temp dir, runs its setup.sh,
# checks the result, builds it in every mode (warnings fail the check) plus
# `make bear`, and runs the binaries that can run headless.
#
#   ./check.sh               every variant
#   ./check.sh plain ...     only these
#
# Environment:
#   CORE2_DIR      the core2 clone to use as core2_url (default ~/code/cpp/core2);
#                  a local clone, so core2 needs no network
#   CORE2_VERSION  the tag to pin (default: core2_version's default in
#                  template/cargo-generate.toml, which is what gets checked)
#   KEEP=1         keep the temp dir after a pass (it is always kept after a failure)
#   WINDOWED=1     also run the ImGui variants that need a real display
#                  (sdl3_gpu, glfw_opengl3, glfw_vulkan) for 2 seconds each
set -eu

here=$(cd "$(dirname "$0")" && pwd)
core2=${CORE2_DIR:-$HOME/code/cpp/core2}
default_version=$(sed -n 's/^core2_version = .*default = "\([^"]*\)".*/\1/p' "$here/template/cargo-generate.toml")
version=${CORE2_VERSION:-$default_version}
all_variants="plain ncurses sdl3 imgui-sdl3_renderer imgui-sdl3_gpu imgui-glfw_opengl3 imgui-glfw_vulkan"
variants=${*:-$all_variants}
modes="debug release lto instrument"
jobs=$(nproc 2>/dev/null || echo 4)

# git refuses to clone a local path as a submodule unless this is allowed.
export GIT_CONFIG_COUNT=1 GIT_CONFIG_KEY_0=protocol.file.allow GIT_CONFIG_VALUE_0=always

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

  set -- -d ui="$ui" -d core2_url="$core2" -d core2_version="$version"
  if [ -n "$backend" ]; then set -- "$@" -d imgui_backend="$backend"; fi
  run "$v: cargo generate" cargo generate --path "$here" template \
    --name "$name" --destination "$out" --silent "$@"
  run "$v: setup.sh" sh "$dir/setup.sh"

  # The generated tree
  for f in setup.sh post.rhai CLAUDE.client.md cargo-generate.toml ui; do
    [ ! -e "$dir/$f" ] || fail "$v: $f is left over"
  done
  for f in CLAUDE.md README.md Makefile .gitignore docs/core2.md src/main.cpp \
           .claude/settings.json .vscode/settings.json .vscode/launch.json .vscode/tasks.json; do
    [ -f "$dir/$f" ] || fail "$v: $f is missing"
  done
  if grep -rIn -e '{{' -e '{%' --exclude-dir=.git --exclude-dir=third-party --exclude-dir=src "$dir"; then
    fail "$v: unrendered Liquid tags"
  fi
  run "$v: JSON files" python3 -c 'import json, sys; [json.load(open(p)) for p in sys.argv[1:]]' \
    "$dir/.claude/settings.json" "$dir/.vscode/settings.json" "$dir/.vscode/launch.json" "$dir/.vscode/tasks.json"

  # The submodule, its pin, and what CLAUDE.md and the settings take from it
  pinned=$(git -C "$dir/third-party/core2" describe --tags --exact-match 2>/dev/null || true)
  [ "$pinned" = "$version" ] || fail "$v: third-party/core2 is at '$pinned', not $version"
  staged=$(git -C "$dir" diff --cached --name-only)
  for f in .gitmodules third-party/core2; do
    printf '%s\n' "$staged" | grep -qx "$f" || fail "$v: $f is not staged"
  done
  [ -f "$dir/third-party/core2/docs/style.md" ] || fail "$v: third-party/core2/docs/style.md (imported by CLAUDE.md) is missing"
  run "$v: comment-only hook self-test" python3 "$dir/third-party/core2/.claude/hooks/comment_only_guard.py" --selftest

  # Every mode, warning-free
  for m in $modes; do
    run "$v: make MODE=$m" make -C "$dir" -j"$jobs" MODE="$m"
    if grep -n 'warning:' "$log"; then
      fail "$v: make MODE=$m printed warnings"
    fi
    bin=$dir/build/${name}_$m
    [ -x "$bin" ] || fail "$v: $bin was not built"
    case $ui in
      plain | ncurses)
        # ncurses: no terminal here, so it prints through core2 instead
        run "$v: run $m" "$bin" one "two words"
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
      imgui)
        # SDL_GPU, OpenGL and Vulkan need a surface that can present, which
        # SDL's offscreen driver does not have: those are only built, unless
        # WINDOWED=1 runs them on the real display.
        if [ "$backend" = sdl3_renderer ]; then
          headless=1
        else
          headless=
        fi
        if [ -n "$headless" ] || [ "${WINDOWED:-}" = 1 ]; then
          rc=0
          if [ -n "$headless" ]; then
            (cd "$dir" && SDL_VIDEO_DRIVER=offscreen timeout 2 "$bin") </dev/null >"$log" 2>&1 || rc=$?
          else
            (cd "$dir" && timeout 2 "$bin") </dev/null >"$log" 2>&1 || rc=$?
          fi
          [ "$rc" = 124 ] || { cat "$log" >&2; fail "$v: $bin exited with $rc before 2 seconds"; }
          grep -q 'to quit$' "$log" || { cat "$log" >&2; fail "$v: $bin did not get past its window setup"; }
        fi
        ;;
      sdl3)
        # SDL's offscreen video driver: no window on the desktop. The program
        # runs until it is closed, so still running after 2 seconds passes.
        rc=0
        (cd "$dir" && SDL_VIDEO_DRIVER=offscreen timeout 2 "$bin") </dev/null >"$log" 2>&1 || rc=$?
        [ "$rc" = 124 ] || { cat "$log" >&2; fail "$v: $bin exited with $rc before 2 seconds"; }
        grep -q 'to quit$' "$log" || { cat "$log" >&2; fail "$v: $bin did not get past its window setup"; }
        ;;
    esac
    echo "   $m: ok"
  done

  run "$v: make bear" make -C "$dir" bear
  run "$v: compile_commands.json" python3 -c '
import json, sys
db = json.load(open(sys.argv[1]))
files = [e["file"] for e in db]
assert any(f.endswith("src/main.cpp") for f in files), "no entry for src/main.cpp"
assert any("third-party/core2/src/" in f for f in files), "no entries for core2"
' "$dir/compile_commands.json"
  echo "   bear: ok"
}

for v in $variants; do
  check_variant "$v"
done

echo "all passed: $variants (core2 $version from $core2)"
if [ "${KEEP:-}" = 1 ]; then
  echo "kept: $out"
else
  rm -rf "$out"
fi
