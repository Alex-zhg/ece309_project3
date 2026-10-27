// tests/p3/test_p3.cpp — YOUR P3 TEST SUITE.
//
// The five tests below are worked examples. Keep them, and add your own
// until you have AT LEAST 15 (spec §6 lists what must be covered).
// Name every test <area>_<what>: reg_ for the registry, ws_ for the
// workspace, calc_ for the calculator.
//
// Build & run:   cmake --build build && ./build/test_p3
// One area:      ./build/test_p3 reg_        (names starting with "reg_")
// One test:      ./build/test_p3 =reg_add_and_find

#include "../mini_test.h"
#include "../test_helpers.h"     // TempDir, write_text, args1, ...

#include "tools/calculator.h"
#include "tools/tool_registry.h"
#include "tools/workspace.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

namespace {

// A tool whose name we choose — used to fill the registry.
class NamedTool : public Tool {
public:
    explicit NamedTool(std::string n) : n_(std::move(n)), schema_(JsonValue::object()) {}
    std::string name() const override { return n_; }
    std::string description() const override { return "test tool"; }
    const JsonValue& schema() const override { return schema_; }
    ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
private:
    std::string n_;
    JsonValue schema_; // an empty JSON object: {}
};

bool eval(const std::string& expr, double* value, std::string* error) {
    std::vector<CalcToken> tokens;
    if (!calc_tokenize(expr, &tokens, error)) return false;
    return calc_evaluate(tokens, value, error);
}

//Evaluates an expression that must succeed and returns its value.
double calc(const std::string& expr) {
    double v = 0;
    std::string err;
    REQUIRE(eval(expr, &v, &err));
    return v;
}

std::string tool_name(int i) { return "tool_" + std::to_string(i); }

}

TEST(reg_add_and_find) {
    ToolRegistry r;
    auto t = std::make_unique<NamedTool>("alpha");
    Tool* raw = t.get(); // remember the address before moving
    REQUIRE(r.add(std::move(t)));
    CHECK(r.size() == 1);
    CHECK(r.find("alpha") == raw);
    CHECK(r.find("beta") == nullptr);
}

TEST(reg_duplicate_keeps_original) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("alpha");
    Tool* original = first.get();
    REQUIRE(r.add(std::move(first)));
    CHECK(!r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(r.find("alpha") == original);
    CHECK(r.size() == 1);
}

TEST(reg_rejects_nullptr) {
    ToolRegistry r;
    CHECK(!r.add(nullptr));
    CHECK(r.size() == 0);
}

TEST(reg_find_absent_name) {
    ToolRegistry r;
    CHECK(r.find("anything") == nullptr);
    r.add(std::make_unique<NamedTool>("alpha"));
    CHECK(r.find("alph") == nullptr);
    CHECK(r.find("alphaa") == nullptr);
    CHECK(r.size() == 1);
}

TEST(reg_for_each_visits_each_once) {
    ToolRegistry r;
    r.add(std::make_unique<NamedTool>("a"));
    r.add(std::make_unique<NamedTool>("b"));
    r.add(std::make_unique<NamedTool>("c"));

    std::vector<std::string> seen;
    r.for_each([&](const Tool& t) { seen.push_back(t.name()); });
    std::sort(seen.begin(), seen.end());
    CHECK(seen == std::vector<std::string>({"a", "b", "c"})); // each exactly once
}

TEST(reg_10000_tools) {
    const int n = 10000;
    ToolRegistry r;
    Tool* first_ptr = nullptr;
    int load_too_high = 0;

    for (int i = 0; i < n; ++i) {
        auto t = std::make_unique<NamedTool>(tool_name(i));
        Tool* raw = t.get();
        REQUIRE(r.add(std::move(t)));
        if (i == 0) first_ptr = raw;
        if (r.load_factor() > 1.0) ++load_too_high;
    }

    CHECK(load_too_high == 0); // load factor <= 1.0 after every add
    CHECK(r.size() == static_cast<std::size_t>(n));
    CHECK(r.find("tool_0") == first_ptr);
    CHECK(r.max_probe_length() <= 64);

    int not_found = 0;
    for (int i = 0; i < n; ++i)
        if (r.find(tool_name(i)) == nullptr) ++not_found;
    CHECK(not_found == 0); // every name can be found

    std::printf("    bucket_count=%zu  load_factor=%.4f  max_probe_length=%zu\n", r.bucket_count(), r.load_factor(), r.max_probe_length());
}

TEST(reg_alternative_hash) {
    ToolRegistry r;
    for (int i = 0; i < 10000; ++i) r.add(std::make_unique<NamedTool>(tool_name(i)));

    // Count how many names land in each bucket; the biggest count is the longest chain.
    std::vector<int> counts(r.bucket_count(), 0);
    for (int i = 0; i < 10000; ++i) {
        std::string s = tool_name(i);
        std::size_t weak = static_cast<unsigned char>(s[0]) + s.size();
        ++counts[weak % r.bucket_count()];
    }
    int weak_longest = *std::max_element(counts.begin(), counts.end());

    std::printf("    FNV-1a longest chain = %zu    weak hash longest chain = %d\n", r.max_probe_length(), weak_longest);
    CHECK(weak_longest > 1000);
    CHECK(r.max_probe_length() < 64);
}

TEST(reg_hash_is_a_real_string_hash) {
    CHECK(ToolRegistry::hash("ab") != ToolRegistry::hash("ba")); // order matters
    CHECK(ToolRegistry::hash("abc") == ToolRegistry::hash("abc")); // deterministic

    std::unordered_set<std::size_t> distinct;
    for (int i = 0; i < 10000; ++i) distinct.insert(ToolRegistry::hash(tool_name(i)));
    CHECK(distinct.size() >= 9990);
}

TEST(ws_rejects_parent_escape) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("notes/todo.txt", &out, &reason));
    CHECK(out == ws.root() / "notes" / "todo.txt");
    CHECK(!ws.resolve("../etc/passwd", &out, &reason));
    CHECK(!reason.empty());
}

TEST(ws_accepts_good_paths) {
    TempDir d;
    Workspace ws(d.path());
    const fs::path root = ws.root();
    fs::path out;
    std::string why;

    CHECK(ws.resolve("notes/todo.txt", &out, &why)); CHECK(out == root / "notes/todo.txt");
    CHECK(ws.resolve("./notes//todo.txt", &out, &why)); CHECK(out == root / "notes/todo.txt");
    CHECK(ws.resolve(".", &out, &why)); CHECK(out == root);
    CHECK(ws.resolve("a/../b", &out, &why)); CHECK(out == root / "b");
    CHECK(ws.resolve("a/..", &out, &why)); CHECK(out == root);
    CHECK(ws.resolve("new/dir/file.txt", &out, &why)); CHECK(out == root / "new/dir/file.txt");
    CHECK(ws.resolve("my..notes.txt", &out, &why)); CHECK(out == root / "my..notes.txt");
}

TEST(ws_rejects_bad_paths) {
    TempDir d;
    Workspace ws(d.path());
    // This one climbs out of the workspace and walks back in.
    std::string back_in = "notes/../../" + ws.root().filename().string() + "/notes/todo.txt";

    for (const std::string& bad : {std::string(""), std::string("/etc/passwd"), std::string(".."), std::string("../etc/passwd"), std::string("a/../../b"), back_in}) {
        fs::path out;
        std::string why;
        CHECK(!ws.resolve(bad, &out, &why));
        CHECK(!why.empty());
    }
}

TEST(ws_rejects_nul_byte) {
    TempDir d;
    Workspace ws(d.path());
    std::string with_nul = std::string("notes") + '\0' + "/x";
    fs::path out;
    std::string why;
    CHECK(!ws.resolve(with_nul, &out, &why));
    CHECK(!why.empty());
}

TEST(ws_does_not_touch_the_filesystem) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string why;
    REQUIRE(ws.resolve("brand/new/file.txt", &out, &why));
    CHECK(!fs::exists(ws.root() / "brand")); // resolve created nothing
}

TEST(calc_precedence) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2 + 3 * 4", &v, &err));
    CHECK(v == 14);
}

TEST(calc_division_by_zero) {
    double v = 0;
    std::string err;
    CHECK(!eval("1 / 0", &v, &err));
    CHECK(err == "division by zero");
    CHECK(!eval("1 / (2 - 2)", &v, &err));
    CHECK(err == "division by zero");
}

TEST(calc_left_to_right) {
    CHECK(calc("10 - 4 - 3") == 3); // (10-4)-3, not 10-(4-3)
    CHECK(calc("100 / 10 / 5") == 2);
    CHECK(calc("10 - 4") == 6); // operands in the right order
}

TEST(calc_parentheses) {
    CHECK(calc("(2 + 3) * 4") == 20);
    CHECK(calc("2 * (3 + 4) - 5") == 9);
    CHECK(calc("2 * (3 + (4 - 1) * 2)") == 18); // nested
}

TEST(calc_negatives_and_decimals) {
    CHECK(calc("-3 * -3") == 9);
    CHECK(calc("7 / 2") == 3.5);
    CHECK(calc("1.5 * 4") == 6);
    CHECK(calc("-2 + -3") == -5);
}

TEST(calc_very_long_expressions) {
    std::string many_terms = "1";
    for (int i = 1; i < 20000; ++i) many_terms += "+1";
    CHECK(calc(many_terms) == 20000); // 20,000 terms

    std::string nested = std::string(5000, '(') + "1" + std::string(5000, ')');
    CHECK(calc(nested) == 1); // 5,000 nested parentheses
}

int main(int argc, char** argv) { return mt::run_all(argc, argv); }