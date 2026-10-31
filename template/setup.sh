#!/bin/sh
# One-time setup for {{project-name}}, generated from core2-template: adds the
# pinned submodules under third-party/, stages them, then deletes this script.
# Safe to rerun if it stops halfway (a network error, say).
set -eu
cd "$(dirname "$0")"

[ -d .git ] || git init -q

# add_pinned <url> <path> <tag>: the submodule at <path>, checked out at <tag>.
# The staged gitlink is the pin.
add_pinned() {
  if [ -z "$(git ls-files --stage -- "$2")" ]; then
    git submodule add -q -- "$1" "$2"
  else
    git submodule update -q --init -- "$2"
  fi
  git -C "$2" -c advice.detachedHead=false checkout -q "$3"
  git add .gitmodules "$2"
  echo "$2 pinned at $3"
}

add_pinned '{{core2_url}}' third-party/core2 '{{core2_version}}'
{%- if ui == "imgui" %}
add_pinned https://github.com/ocornut/imgui.git third-party/imgui v1.92.8
{%- endif %}

rm -f setup.sh
echo "Done. Build with: make"
