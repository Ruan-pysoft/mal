#ifndef ENV_H
#define ENV_H

#include <ministd_error.h>

#include "values.h"

typedef struct mal_string key_t;

void key_free(key_t own this);
bool key_cmp(const key_t ref a, const key_t ref b);

typedef struct env_t env_t;
env_t own env_new(err_t ref err_out);
void env_free(env_t own this);
void env_add(env_t ref this, const key_t own key, const value_t own val,
	     err_t ref err_out);
const value_t ref env_get(env_t ref this, const key_t ref key);

#endif /* ENV_H */
