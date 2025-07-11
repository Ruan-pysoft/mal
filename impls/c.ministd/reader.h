#ifndef READER_H
#define READER_H

#include <ministd_error.h>
#include <ministd_types.h>

#include "values.h"

const value_t own read_str(const char ref str, err_t ref err_out);

#endif /* READER_H */
