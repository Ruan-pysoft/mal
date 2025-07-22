#ifndef READER_H
#define READER_H

#include <ministd_error.h>
#include <ministd_types.h>

#include "error.h"
#include "values.h"

Value_own read_str(const char ref str, perr_t ref err_out);

#endif /* READER_H */
