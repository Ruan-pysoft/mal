#ifndef TYPES_H
#define TYPES_H

#include <ministd_types.h>

#include "error.h"

enum CMP {
	C_EQ,
	C_LT,
	C_GT
};

typedef const struct Value_struct own Value_own;
typedef const struct Value_struct ref Value_ref;
/* value_new-type functions found in `values.h` */
Value_own value_copy(Value_ref this);
void value_free(Value_own this);
/* other functions associated with values found in `values.h` */

/* environments are mutable */
typedef struct Env_struct own MutEnv_own;
typedef struct Env_struct ref MutEnv_ref;
typedef const struct Env_struct own Env_own;
typedef const struct Env_struct ref Env_ref;
/* env_new function found in `env.h` */
MutEnv_own env_copy(MutEnv_ref this);
void env_free(Env_own this);
/* other functions associated with environments found in `env.h` */

typedef const struct String_struct own String_own;
typedef const struct String_struct ref String_ref;
String_own string_new(const char ref cstr, err_t ref err_out);
String_own string_copy(String_ref this);
void string_free(String_own this);

usz string_len(String_ref this);
const char ref string_cstr(String_ref this);
/* equality checking can implement some optimisations that comparison cannot */
bool string_iseq(String_ref this, String_ref other);
enum CMP string_cmp(String_ref this, String_ref other);
String_own _string_append(struct String_struct own this, const char ref cstr,
			  err_t ref err_out);

/* list of values */
typedef const struct List_struct own List_own;
typedef const struct List_struct ref List_ref;
List_own list_new(Value_ref ref values, usz n_values, err_t ref err_out);
List_own list_deepcopy(List_ref this, err_t ref err_out);
List_own list_tail(List_ref this, usz from_idx, err_t ref err_out);
List_own list_copy(List_ref this);
void list_free(List_own this);

usz list_len(List_ref this);
/* error with ERR_INVAL for n >= list_len(this) */
Value_ref list_nth(List_ref this, usz n, err_t ref err_out);
usz list_fns(List_ref this);
List_own _list_extend(struct List_struct own this, Value_ref ref values,
		      usz n_values, err_t ref err_out);
List_own _list_append(struct List_struct own this, Value_own value,
		      err_t ref err_out);
/* error with ERR_INVAL for idx >= list_len(this) */
List_own _list_set(struct List_struct own this, usz idx, Value_own value,
		   err_t ref err_out);

struct List_iter {
	List_ref list;
	usz pos;
};
typedef const struct List_iter List_iter;
List_iter list_iter(List_ref this);
List_iter list_next(List_iter this);
bool list_isend(List_iter this);
/* error with ERR_INVAL for list_isend(this) */
Value_ref list_at(List_iter this, err_t ref err_out);
/* error with ERR_INVAL for list_isend(iter),
 * or if iter is an iter to another list */
List_own _list_at_set(struct List_struct own this, List_iter at,
		      Value_own value, err_t ref err_out);

/* hash-map of key-value pairs; all keys are Strings */
typedef const struct HashMap_struct own HashMap_own;
typedef const struct HashMap_struct ref HashMap_ref;
HashMap_own hashmap_new(String_ref ref keys, Value_ref ref values, usz n_items,
			err_t ref err_out);
HashMap_own hashmap_deepcopy(HashMap_ref this, err_t ref err_out);
HashMap_own hashmap_copy(HashMap_ref this);
void hashmap_free(HashMap_own this);

/* no. of key/value pairs in hash-map */
usz hashmap_size(HashMap_ref this);
bool hashmap_contains(HashMap_ref this, String_ref key);
/* error with ERR_INVAL for !hashmap_contains(this, key) */
Value_ref hashmap_get(HashMap_ref this, String_ref key, err_t ref err_out);
HashMap_own hashmap_set(HashMap_ref this, String_own key, Value_own value,
			err_t ref err_out);
/* if key present in this, don't take from other */
HashMap_own hashmap_union(HashMap_ref this, HashMap_ref other,
			  err_t ref err_out);
usz hashmap_fns(HashMap_ref this);
HashMap_own _hashmap_assoc(struct HashMap_struct own this, String_own key,
			   Value_own value, err_t ref err_out);
/* if key present in this, don't take from keys&values */
HashMap_own _hashmap_extend(struct HashMap_struct own this, String_ref ref keys,
			    Value_ref ref values, usz n_items, err_t ref err_out);

struct HashMap_iter {
	HashMap_ref hashmap;
	usz pos;
};
typedef const struct HashMap_iter HashMap_iter;
HashMap_iter hashmap_iter(HashMap_ref this);
HashMap_iter hashmap_next(HashMap_iter this);
bool hashmap_isend(HashMap_iter this);
/* error with ERR_INVAL for hashmap_isend(this) */
String_ref hashmap_keyat(HashMap_iter this, err_t ref err_out);
/* error with ERR_INVAL for hashmap_isend(this) */
Value_ref hashmap_valueat(HashMap_iter this, err_t ref err_out);
/* error with ERR_INVAL for hashmap_isend(iter),
 * or if iter is an iter to another hash-map */
HashMap_own _hashmap_at_set(struct HashMap_struct own this, HashMap_iter at,
			    Value_own value, err_t ref err_out);

typedef const struct Fn_struct own Fn_own;
typedef const struct Fn_struct ref Fn_ref;
/* functions are either builtin functions or a mal function
 * the environment *can* be NULL, to create a non-closure function
 */
typedef Value_own (ref builtin_fn)(List_own args, MutEnv_own env,
				   rerr_t ref err_out);
Fn_own fn_from_builtin_fn(builtin_fn fn, Env_own env, err_t ref err_out);
Fn_own fn_new(Value_own body, String_own own args, usz n_args, bool variadic,
	      Env_own env, err_t ref err_out);
Fn_own fn_copy(Fn_ref this);
void fn_free(Fn_own this);

Value_own fn_call(Fn_ref this, List_own args, rerr_t ref err_out);
Env_ref fn_closure(Fn_ref this);

#endif /* TYPES_H */
