// src/tools/workspace.cpp — YOU IMPLEMENT (spec §4.4).
//
// Workspace::resolve: a purely lexical check, steps 1–6 of the spec.
// Do not touch the filesystem here (no exists(), canonical(), ...).
// The constructor, root() and check_symlinks() are provided in
// workspace_provided.cpp.

#include "tools/workspace.h"

#include <exception>
#include <string>

namespace fs = std::filesystem;

bool Workspace::resolve(std::string_view user_path, fs::path* out, std::string* reason) const {
    try {
        // 1. Empty path
        if (user_path.empty()) {
            *reason = "empty path";
            return false;
        }

        // 2. Embedded NUL: OS truncates the path
        if (user_path.find('\0') != std::string_view::npos) {
            *reason = "path contains a NUL byte";
            return false;
        }
        // 3. Only workspace relative paths
        const fs::path p{std::string(user_path)};
        if (p.is_absolute() || p.has_root_path()) {
            *reason = "absolute paths are not allowed: use a path relative to the workspace";
            return false;
        }

        // 4. Duplicate slashes, and "/.." pairs
        const fs::path n = p.lexically_normal();

        // 5. A leading ".." means the path goes above the root at some point
        if (n.empty() || *n.begin() == "..") {
            *reason = "path escapes the workspace";
            return false;
        }

        // 6. "." means the root itself
        *out = (n == ".") ? root() : root() / n;
        return true;
    } catch (const std::exception& e) {
        *reason = std::string("invalid path: ") + e.what();
        return false;
    } catch (...) {
        *reason = "invalid path";
        return false;
    }
}