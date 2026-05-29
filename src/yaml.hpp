#pragma once
// torc — mini-YAML parser (subset: maps, lists, scalars, no anchors/flow)

#include <string>
#include <variant>
#include <vector>

namespace torc::yaml {

struct Node;

using Scalar = std::string;
using List = std::vector<Node>;
using Map = std::vector<std::pair<std::string, Node>>;

struct Node {
    std::variant<Scalar, List, Map> value;

    bool is_scalar() const { return std::holds_alternative<Scalar>(value); }
    bool is_list() const { return std::holds_alternative<List>(value); }
    bool is_map() const { return std::holds_alternative<Map>(value); }

    const Scalar &as_scalar() const { return std::get<Scalar>(value); }
    const List &as_list() const { return std::get<List>(value); }
    const Map &as_map() const { return std::get<Map>(value); }

    // Lookup key in map node (returns nullptr if not found or not a map)
    const Node *get(std::string_view key) const;
};

struct ParseError {
    int line;
    std::string message;
};

// Parse mini-YAML from string. Returns root node or error.
struct ParseResult {
    Node root;
    std::vector<ParseError> errors;
    bool ok() const { return errors.empty(); }
};

ParseResult parse(std::string_view input);

} // namespace torc::yaml
