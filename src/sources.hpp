#pragma once
// torc — source file discovery with visitor callback

#include <functional>
#include <string>

namespace torc {

// Calls visitor for each .cpp file found, in sorted order.
// Returns count of files visited.
using SourceVisitor = std::function<void(const std::string &path)>;

int for_each_source(const std::string &src_dir, bool recursive, const SourceVisitor &visitor);

} // namespace torc
