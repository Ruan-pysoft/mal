#ifndef ENV_H
#define ENV_H

#include <ministd_error.h>

#include "values.h"

typedef struct mal_string key_t;

typedef struct env_t env_t;
#ifndef FULL_ENV
env_t own env_new(err_t ref err_out);
#else
env_t own env_new(const env_t own outer, err_t ref err_out);
#endif
env_t own env_copy(env_t ref this);
void env_free(env_t own this);
#ifndef FULL_ENV
void env_add(env_t ref this, const key_t own key, const value_t own val,
	     err_t ref err_out);
#else
void env_set(env_t ref this, const key_t own key, const value_t own val,
	     err_t ref err_out);
#endif
const value_t ref env_get(const env_t ref this, const key_t ref key);

#endif /* ENV_H */
