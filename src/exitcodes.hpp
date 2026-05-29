#pragma once
// torc — C++ package manager
// Exit codes per <sysexits.h>

namespace torc {

enum ExitCode : int {
    EX_OK = 0,
    EX_USAGE = 64,       // bad CLI usage
    EX_DATAERR = 65,     // manifest parse error
    EX_NOINPUT = 66,     // file not found
    EX_UNAVAILABLE = 69, // network/download failure
    EX_CANTCREAT = 73,   // cannot create directory
    EX_IOERR = 74,       // I/O error
    EX_CONFIG = 78,      // bad configuration
};

} // namespace torc
