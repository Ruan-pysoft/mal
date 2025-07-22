#include "env.h"

#include <ministd_memory.h>
#include <ministd_string.h>

#include "error.h"
#include "types.h"
#include "values.h"

struct Env_struct {
	/* TODO: turn this into a hashmap,
	 * rather than a simple associative array
	 */
	Env_own outer;
	String_own own vars;
	Value_own own vals;
	usz len;
	usz cap;

	usz ref_count;
	usz circular_refs;
};
/* do BEFORE removing value from environment:
 * find all functions pointing to this or parent environment,
 * and decrement the circular_refs count for each one
 */
void _env_circular_refs_remove(Env_ref start, Value_ref val);
/* do after adding value to environment:
 * find all functions pointing to this or parent environment,
 * and increment the circular_refs count for each one
 */
void _env_circular_refs_add(Env_ref start, Value_ref val);
MutEnv_own
env_new(Env_own outer, err_t ref err_out)
{
	err_t err = ERR_OK;
	MutEnv_own res;

	res = alloc(sizeof(*res), &err);
	TRY_WITH(err, NULL);
	res->ref_count = 1;
	res->circular_refs = 0;

	res->outer = outer;

	res->cap = 16;
	res->len = 0;
	res->vars = nalloc(sizeof(*res->vars), res->cap, &err);
	if (err != ERR_OK) {
		free(res);
		ERR_WITH(err, NULL);
	}
	res->vals = nalloc(sizeof(*res->vals), res->cap, &err);
	if (err != ERR_OK) {
		free(res->vars);
		free(res);
		ERR_WITH(err, NULL);
	}

	return res;
}
MutEnv_own
env_copy(MutEnv_ref this)
{
	++this->ref_count;

	return (MutEnv_own)this;
}
void
env_free(Env_own this)
{
	if (this == NULL) return;
	--((MutEnv_ref)this)->ref_count;

	if (this->ref_count == this->circular_refs) {
		env_free(this->outer);

		if (this->vars != NULL) {
			usz i;
			for (i = 0; i < this->len; ++i) {
				string_free(this->vars[i]);
			}
			free(this->vars);
		}
		if (this->vals != NULL) {
			usz i;
			for (i = 0; i < this->len; ++i) {
				_env_circular_refs_remove(
					this->outer,
					this->vals[i]
				);
				value_free(this->vals[i]);
			}
			free(this->vals);
		}
		free((own_ptr)this);
	}
}

usz
env_size(Env_ref this)
{
	return this->len;
}
bool
env_contains(Env_ref this, String_ref var)
{
	usz i;

	while (this != NULL) {
		for (i = 0; i < this->len; ++i) {
			if (string_iseq(var, this->vars[i])) return true;
		}

		this = this->outer;
	}

	return false;
}
Value_ref
env_get(Env_ref this, String_ref var, err_t ref err_out)
{
	usz i;

	while (this != NULL) {
		for (i = 0; i < this->len; ++i) {
			if (string_iseq(var, this->vars[i])) {
				return this->vals[i];
			}
		}

		this = this->outer;
	}

	ERR_WITH(ERR_INVAL, NULL);
}
/* conceptually operates on constant values:
 * the contents of the environment does not change.
 * returns ERR_INVAL if newcap < this->len
 */
/* WARN: if the reallocs fail,
 * the env is left in an invalid state,
 * and must be immediately free'd by the caller!
 */
void
_env_grow(Env_ref this, usz newcap, err_t ref err_out)
{
	err_t err = ERR_OK;
	MutEnv_ref mut_this = (MutEnv_ref)this;

	if (newcap < this->len) {
		ERR_VOID(ERR_INVAL);
	}

	mut_this->cap = newcap;

	mut_this->vars = nrealloc(
		mut_this->vars,
		sizeof(*mut_this->vars),
		mut_this->cap,
		&err
	);
	if (err != ERR_OK) {
		mut_this->vars = NULL;
		/* NOTE: theoretically,
		 * we should call `free_string` on all the vars,
		 * but if something went wrong with realloc,
		 * we really don't know what happened to that memory
		 * (is it deallocated yet? presumably)
		 * so we'll just have to leak them
		 */
		ERR_VOID(err);
	}
	mut_this->vals = nrealloc(
		mut_this->vals,
		sizeof(*mut_this->vals),
		mut_this->cap,
		&err
	);
	if (err != ERR_OK) {
		mut_this->vals = NULL;
		/* NOTE: theoretically,
		 * we should call `free_value` on all the vals,
		 * but if something went wrong with realloc,
		 * we really don't know what happened to that memory
		 * (is it deallocated yet? presumably)
		 * so we'll just have to leak them
		 */
		ERR_VOID(err);
	}
}
void
env_set(MutEnv_ref this, String_own var, Value_own val, err_t ref err_out)
{
	err_t err = ERR_OK;
	usz i;

	for (i = 0; i < this->len; ++i) {
		if (string_iseq(var, this->vars[i])) {
			string_free(var);
			_env_circular_refs_remove(this, this->vals[i]);
			value_free(this->vals[i]);
			this->vals[i] = val;
			_env_circular_refs_add(this, this->vals[i]);

			return;
		}
	}

	if (this->len == this->cap) {
		_env_grow(this, this->cap*2, &err);
		TRY_VOID(err);
	}

	this->vars[this->len] = var;
	this->vals[this->len] = val;
	_env_circular_refs_add(this, this->vals[this->len]);
	++this->len;
}
void
env_extend(MutEnv_ref this, String_ref ref vars, Value_ref ref vals,
	   usz n_items, err_t ref err_out)
{
	/* WARN: currently time complexity is O(n^2),
	 * need to implement a proper hashmap structure
	 * to reduce time complexity down to O(n log n)
	 */

	err_t err = ERR_OK;
	usz i;

	for (i = 0; i < n_items; ++i) {
		env_set(this, vars[i], vals[i], &err);
		TRY_VOID(err);
	}
}
void
env_bind(MutEnv_ref this, String_ref ref arg_names, usz n_args, bool variadic,
	 List_own arg_vals, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	usz i;
	const usz required_args = variadic ? n_args-1 : n_args;

	if (variadic && list_len(arg_vals) < required_args) {
		err = rerr_arg_vararg_mismatch(
			required_args,
			list_len(arg_vals)
		);
		list_free(arg_vals);
		RERR_VOID(err);
	} else if (!variadic && list_len(arg_vals) != required_args) {
		err = rerr_arg_len_mismatch(required_args, list_len(arg_vals));
		list_free(arg_vals);
		RERR_VOID(err);
	}

	for (i = 0; i < required_args; ++i) {
		Value_ref val = list_nth(arg_vals, i, &err.e.errt);
		if (!rerr_is_ok(err)) {
			list_free(arg_vals);
			RERR_VOID(err);
		}
		env_set(
			this,
			string_copy(arg_names[i]),
			value_copy(val),
			&err.e.errt
		);
		if (!rerr_is_ok(err)) {
			list_free(arg_vals);
			RERR_VOID(err);
		}
	}

	if (variadic) {
		List_own list;
		Value_own val;
		if (n_args == required_args) {
			list = list_new(NULL, 0, &err.e.errt);
			if (!rerr_is_ok(err)) {
				list_free(arg_vals);
				RERR_VOID(err);
			}
		} else {
			list = list_tail(arg_vals, required_args, &err.e.errt);
			if (!rerr_is_ok(err)) {
				list_free(arg_vals);
				RERR_VOID(err);
			}
		}
		val = value_list(list, &err.e.errt);
		if (!rerr_is_ok(err)) {
			list_free(arg_vals);
			RERR_VOID(err);
		}
		env_set(
			this,
			string_copy(arg_names[n_args-1]),
			val,
			&err.e.errt
		);
		if (!rerr_is_ok(err)) {
			list_free(arg_vals);
			RERR_VOID(err);
		}
	}

	list_free(arg_vals);
}

bool
_env_isparent(Env_ref this, Env_ref other)
{
	if (this == NULL || other == NULL) return false;

	while (this != NULL) {
		if (this->outer == other) return true;
		this = this->outer;
	}

	return false;
}
void
_env_circular_refs_remove(Env_ref start, Value_ref val)
{
	if (start == NULL) return;
	if (value_fns(val) == 0) return;

	if (value_islist(val)) {
		struct List_iter at;
		List_ref list = value_getlist(val, NULL);

		for (at = list_iter(list); !list_isend(at); list_next(at)) {
			if (value_fns(list_at(at, NULL)) > 0) {
				_env_circular_refs_remove(start, val);
			}
		}
	} else if (value_isvector(val)) {
		struct List_iter at;
		List_ref list = value_getvector(val, NULL);

		for (at = list_iter(list); !list_isend(at); list_next(at)) {
			if (value_fns(list_at(at, NULL)) > 0) {
				_env_circular_refs_remove(start, val);
			}
		}
	} else if (value_ishashmap(val)) {
		struct HashMap_iter at;
		HashMap_ref hashmap = value_gethashmap(val, NULL);

		for (at = hashmap_iter(hashmap); !hashmap_isend(at);
				hashmap_next(at)) {
			if (value_fns(hashmap_valueat(at, NULL)) > 0) {
				_env_circular_refs_remove(start, val);
			}
		}
	} else if (value_isfn(val)) {
		Fn_ref fn = value_getfn(val, NULL);

		if (_env_isparent(start, fn_closure(fn))) {
			--((MutEnv_ref)fn_closure(fn))->circular_refs;
		}
	} else {
		/* something went wrong! */
		/* TODO: some error reporting here */
	}
}
void
_env_circular_refs_add(Env_ref start, Value_ref val)
{
	if (start == NULL) return;
	if (value_fns(val) == 0) return;


	if (value_islist(val)) {
		struct List_iter at;
		List_ref list = value_getlist(val, NULL);

		for (at = list_iter(list); !list_isend(at); list_next(at)) {
			if (value_fns(list_at(at, NULL)) > 0) {
				_env_circular_refs_add(start, val);
			}
		}
	} else if (value_isvector(val)) {
		struct List_iter at;
		List_ref list = value_getvector(val, NULL);

		for (at = list_iter(list); !list_isend(at); list_next(at)) {
			if (value_fns(list_at(at, NULL)) > 0) {
				_env_circular_refs_add(start, val);
			}
		}
	} else if (value_ishashmap(val)) {
		struct HashMap_iter at;
		HashMap_ref hashmap = value_gethashmap(val, NULL);

		for (at = hashmap_iter(hashmap); !hashmap_isend(at);
				hashmap_next(at)) {
			if (value_fns(hashmap_valueat(at, NULL)) > 0) {
				_env_circular_refs_add(start, val);
			}
		}
	} else if (value_isfn(val)) {
		Fn_ref fn = value_getfn(val, NULL);

		if (_env_isparent(start, fn_closure(fn))) {
			++((MutEnv_ref)fn_closure(fn))->circular_refs;
		}
	} else {
		/* something went wrong! */
		/* TODO: some error reporting here */
	}
}
