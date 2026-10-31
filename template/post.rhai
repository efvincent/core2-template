// Post-generation hook. It runs in cargo-generate's temp dir, before the result
// is moved to the destination, so it only arranges files: the git work is in
// setup.sh (see docs/design.md in core2-template, "Why setup.sh, not the hook").

let ui = variable::get("ui");
let variant = if ui == "imgui" { "imgui/" + variable::get("imgui_backend") } else { ui };

// The chosen variant's directory becomes src/; the others go.
file::rename("ui/" + variant, "src");
file::delete("ui");

// Stored under another name so it is not loaded while working on the template.
file::rename("CLAUDE.client.md", "CLAUDE.md");

file::delete("post.rhai");

print("");
print("Next, once, in the new project: sh setup.sh   (adds the pinned submodules)");
print("Then: make");
