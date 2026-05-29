#include "yaml.hpp"

#include <charconv>
#include <cstring>
#include <sstream>

namespace torc::yaml {

const Node *Node::get(std::string_view key) const {
    if (!is_map())
        return nullptr;
    for (const auto &[k, v] : as_map()) {
        if (k == key)
            return &v;
    }
    return nullptr;
}

// ── Mini-YAML parser ───────────────────────────────────
// Supports:
//   key: value
//   key:
//     nested_key: value
//   key:
//     - list item
//     - list item
//   key: |
//     multi-line block scalar
//
// Does NOT support: anchors, aliases, flow style, tags, complex keys

namespace {

struct Line {
    int indent;
    std::string_view content; // trimmed of leading spaces
    int number;
};

std::vector<Line> tokenize(std::string_view input) {
    std::vector<Line> lines;
    int lineno = 0;
    while (!input.empty()) {
        ++lineno;
        auto nl = input.find('\n');
        auto raw = (nl == std::string_view::npos) ? input : input.substr(0, nl);
        input = (nl == std::string_view::npos) ? std::string_view{} : input.substr(nl + 1);

        // Count indent
        int indent = 0;
        while (indent < static_cast<int>(raw.size()) && raw[static_cast<size_t>(indent)] == ' ')
            ++indent;

        auto content = raw.substr(static_cast<size_t>(indent));
        // Skip blank lines and comments
        if (content.empty() || content[0] == '#')
            continue;

        lines.push_back({indent, content, lineno});
    }
    return lines;
}

class Parser {
  public:
    explicit Parser(std::vector<Line> lines) : lines_(std::move(lines)) {}

    ParseResult run() {
        ParseResult result;
        result.root = parse_node(0, result.errors);
        return result;
    }

  private:
    std::vector<Line> lines_;
    size_t pos_ = 0;

    bool at_end() const { return pos_ >= lines_.size(); }
    const Line &peek() const { return lines_[pos_]; }

    Node parse_node(int min_indent, std::vector<ParseError> &errors) {
        if (at_end())
            return Node{Scalar{}};

        const auto &line = peek();
        if (line.indent < min_indent)
            return Node{Scalar{}};

        // List?
        if (line.content.starts_with("- ")) {
            return parse_list(line.indent, errors);
        }

        // Map?
        auto colon = line.content.find(':');
        if (colon != std::string_view::npos) {
            return parse_map(line.indent, errors);
        }

        // Plain scalar
        ++pos_;
        return Node{Scalar{std::string(line.content)}};
    }

    Node parse_list(int base_indent, std::vector<ParseError> &errors) {
        List list;
        while (!at_end() && peek().indent == base_indent && peek().content.starts_with("- ")) {
            auto item_content = peek().content.substr(2);
            ++pos_;

            // Check if item_content is a key: value (inline map start)
            auto colon = item_content.find(':');
            if (colon != std::string_view::npos) {
                Map m;
                auto key = item_content.substr(0, colon);
                auto val = item_content.substr(colon + 1);
                while (!val.empty() && val[0] == ' ')
                    val = val.substr(1);

                if (val == "|") {
                    m.emplace_back(std::string(key), parse_block_scalar(base_indent + 4));
                } else if (val.empty() && !at_end() && peek().indent > base_indent) {
                    m.emplace_back(std::string(key), parse_node(peek().indent, errors));
                } else {
                    m.emplace_back(std::string(key), Node{Scalar{std::string(val)}});
                }

                // Collect remaining keys at deeper indent
                while (!at_end() && peek().indent > base_indent) {
                    auto &nl = peek();
                    auto nc = nl.content.find(':');
                    if (nc != std::string_view::npos) {
                        auto nk = nl.content.substr(0, nc);
                        auto nv = nl.content.substr(nc + 1);
                        while (!nv.empty() && nv[0] == ' ')
                            nv = nv.substr(1);

                        if (nv == "|") {
                            ++pos_;
                            m.emplace_back(std::string(nk), parse_block_scalar(nl.indent + 2));
                        } else if (nv.empty() && !at_end()) {
                            ++pos_;
                            if (!at_end() && peek().indent > nl.indent) {
                                m.emplace_back(std::string(nk), parse_node(peek().indent, errors));
                            } else {
                                m.emplace_back(std::string(nk), Node{Scalar{}});
                            }
                        } else {
                            m.emplace_back(std::string(nk), Node{Scalar{std::string(nv)}});
                            ++pos_;
                        }
                    } else {
                        ++pos_;
                    }
                }
                list.push_back(Node{std::move(m)});
            } else if (!at_end() && peek().indent > base_indent) {
                // Nested structure under plain list item
                list.push_back(parse_node(peek().indent, errors));
            } else {
                list.push_back(Node{Scalar{std::string(item_content)}});
            }
        }
        return Node{std::move(list)};
    }

    Node parse_map(int base_indent, std::vector<ParseError> &errors) {
        Map map;
        while (!at_end() && peek().indent == base_indent) {
            auto &line = peek();
            auto colon = line.content.find(':');
            if (colon == std::string_view::npos)
                break;

            auto key = line.content.substr(0, colon);
            auto rest = line.content.substr(colon + 1);
            while (!rest.empty() && rest[0] == ' ')
                rest = rest.substr(1);

            ++pos_;

            if (rest == "|") {
                // Block scalar
                map.emplace_back(std::string(key), parse_block_scalar(base_indent + 2));
            } else if (rest.empty()) {
                // Nested structure
                map.emplace_back(std::string(key), parse_node(base_indent + 2, errors));
            } else {
                map.emplace_back(std::string(key), Node{Scalar{std::string(rest)}});
            }
        }
        return Node{std::move(map)};
    }

    Node parse_block_scalar(int min_indent) {
        std::string result;
        while (!at_end() && peek().indent >= min_indent) {
            if (!result.empty())
                result += '\n';
            result += std::string(peek().content);
            ++pos_;
        }
        return Node{Scalar{std::move(result)}};
    }
};

} // namespace

ParseResult parse(std::string_view input) {
    auto lines = tokenize(input);
    Parser p(std::move(lines));
    return p.run();
}

} // namespace torc::yaml
