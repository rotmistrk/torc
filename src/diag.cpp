#include "diag.hpp"
#include <unistd.h>

namespace torc::diag {

bool is_terminal() {
    return isatty(STDERR_FILENO) != 0;
}

} // namespace torc::diag
