#ifndef PRINTER_H
#define PRINTER_H

#include <ministd_error.h>
#include <ministd_types.h>

#include "values.h"

const char own pr_str(const value_t ref val, err_t ref err_out);

#endif /* PRINTER_H */
