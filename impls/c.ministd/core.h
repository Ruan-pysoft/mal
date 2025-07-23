#ifndef CORE_H
#define CORE_H

#include "types.h"

#define CORE_LEN 14

extern const char ref core_names[CORE_LEN];
extern builtin_fn core_funcs[CORE_LEN];

void core_load(MutEnv_ref env, err_t ref err_out);

#endif /* CORE_H */
