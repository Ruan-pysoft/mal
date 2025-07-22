#ifndef ENV_H
#define ENV_H

#include "error.h"
#include "types.h"

/* new empty environment, outer can be NULL */
MutEnv_own env_new(Env_own outer, err_t ref err_out);
MutEnv_own env_copy(MutEnv_ref this);
void env_free(Env_own this);

/* no. of values in environment */
usz env_size(Env_ref this);
bool env_contains(Env_ref this, String_ref var);
/* error with ERR_INVAL for !env_contains(this, var) */
Value_ref env_get(Env_ref this, String_ref var, err_t ref err_out);
void env_set(MutEnv_ref this, String_own var, Value_own val, err_t ref err_out);
void env_extend(MutEnv_ref this, String_ref ref vars, Value_ref ref vals,
		usz n_items, err_t ref err_out);
void env_bind(MutEnv_ref this, String_ref ref arg_names, usz n_args,
	      bool variadic, List_own arg_vals, rerr_t ref err_out);

#endif /* ENV_H */
