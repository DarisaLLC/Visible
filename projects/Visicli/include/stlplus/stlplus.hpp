#ifndef STLPLUS_HPP
#define STLPLUS_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  Header to include the whole of STLplus in one go

------------------------------------------------------------------------------*/
#define STLPLUS_VERSION "2.7"

// fix operating system variations
#include "os_fixes.hpp"

#include <string>

// report on the build characteristics
// e.g. "Generic Unix, gcc v3.3, release"
//      this means that this version of STLplus is a Generic Unix (i.e. Posix) build,
//      built using the Gnu compiler gcc version 3.3 and this is a release build
std::string stlplus_build(void);

// Arithmetic
#include "inf.hpp"
#include "string_arithmetic.hpp"

// Containers
#include "triple.hpp"
#include "foursome.hpp"
#include "smart_ptr.hpp"
#include "digraph.hpp"
#include "hash.hpp"
#include "matrix.hpp"
#include "ntree.hpp"

// TextIO
#include "textio.hpp"
#include "fileio.hpp"
#include "stringio.hpp"
#include "string_vectorio.hpp"
#include "iostreamio.hpp"
#include "multiio.hpp"

// String Formatting Utilities
#include "dprintf.hpp"
#include "string_utilities.hpp"
#include "time.hpp"

// Subsystems
#include "cli_parser.hpp"
#include "file_system.hpp"
#include "error_handler.hpp"
#include "ini_manager.hpp"
#include "library_manager.hpp"
#include "subprocesses.hpp"
#include "tcp.hpp"

// Persistent Data Types
#include "persistent.hpp"

// Others
#include "debug.hpp"
#include "timer.hpp"

#endif
