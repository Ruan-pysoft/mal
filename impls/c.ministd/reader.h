#ifndef READER_H
#define READER_H

#include <ministd_error.h>
#include <ministd_types.h>

#include "error.h"
#include "values.h"

const value_t own read_str(const char ref str, perr_t ref err_out);

#endif /* READER_H */
