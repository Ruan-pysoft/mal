#include "types.h"

#include <ministd.h>
#include <ministd_fmt.h>
#include <ministd_memory.h>
#include <ministd_string.h>

#include "error.h"
#include "values.h"

/* implementation of value_* functions in `values.h` */
Value_own value_copy(Value_ref this);
void value_free(Value_own this);

/* implementation of env_* functions in `env.h` */
MutEnv_own env_copy(MutEnv_ref this);
void env_free(Env_own this);

#define FREE_PRELUDE(Typename) do { \
		if (this == NULL) return; \
		--((struct Typename ## _struct ref)this)->ref_count; \
	} while (0)
#define COPY(Typename) do { \
		++((struct Typename ## _struct ref)this)->ref_count; \
		return (Typename ## _own)this; \
	} while (0)
#define ALLOC() do { \
		res = alloc(sizeof(*res), &err); \
		TRY_WITH(err, NULL); \
		res->ref_count = 1; \
	} while (0)

struct String_struct {
	char own content;
	usz len;

	usz ref_count;
};
String_own
string_new(const char ref cstr, err_t ref err_out)
{
	return string_newn(cstr, strlen(cstr), err_out);
}
String_own
string_newn(const char ref cstr, usz len, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct String_struct own res;

ALLOC();

	res->content = nalloc(sizeof(*res->content), len+1, &err);
	if (err != ERR_OK) {
		free(res);
		ERR_WITH(err, NULL);
	}
	memmove(res->content, cstr, sizeof(*res->content) * len);
	res->content[len] = '\0';

	res->len = len;

	return res;
}
String_own
string_copy(String_ref this)
{ COPY(String); }
void
string_free(String_own this)
{
	FREE_PRELUDE(String);

	if (this->ref_count == 0) {
		free(this->content);
		free((own_ptr)this);
	}
}

void
string_print(String_ref this, bool repr, FILE ref file, err_t ref err_out)
{
	(void)repr; /* TODO: implement repr-printing */

	fprints(this->content, file, err_out);
}
usz
string_len(String_ref this)
{
	return this->len;
}
bool
string_iseq(String_ref this, String_ref other)
{
	usz i;

	if (this == other) return true;

	if (this->len != other->len) return false;

	for (i = 0; i < this->len; ++i) {
		if (this->content[i] != other->content[i]) return false;
	}

	return true;
}
enum CMP
string_cmp(String_ref this, String_ref other)
{
	usz i;
	const usz min_len = this->len < other->len ? this->len : other->len;

	if (this == other) return C_EQ;

	for (i = 0; i < min_len+1; ++i) {
		if (this->content[i] < other->content[i]) return C_LT;
		else if (this->content[i] > other->content[i]) return C_GT;
	}

	return C_EQ;
}
enum CMP
string_match(String_ref this, const char ref to)
{
	usz i;
	const usz len = strnlen(to, this->len + 1);

	for (i = 0; i < len; ++i) {
		if (this->content[i] < to[i]) return C_LT;
		else if (this->content[i] > to[i]) return C_GT;
	}

	return C_EQ;
}
const char ref
string_cstr(String_ref this)
{
	return this->content;
}
String_own
_string_append(struct String_struct own this, const char ref cstr,
	       err_t ref err_out)
{
	err_t err = ERR_OK;
	const usz len = strlen(cstr);

	if (this->ref_count != 1) {
		string_free(this);

		ERR_WITH(ERR_PERM, NULL);
	}

	if (len == 0) return this;

	this->content = nrealloc(
		this->content,
		sizeof(*this->content),
		this->len + len + 1,
		&err
	);
	if (err != ERR_OK) {
		free(this);
		ERR_WITH(err, NULL);
	}

	memmove(
		this->content + this->len,
		cstr,
		sizeof(*this->content) * (len + 1)
	);

	this->len += len;

	return this;
}

struct List_struct {
	Value_own own values;
	usz len;
	usz cap;

	usz fns;
	usz ref_count;
};
List_own
list_new(Value_ref ref values, usz n_values, err_t ref err_out)
{
	err_t err = ERR_OK;
	usz i;
	struct List_struct own res;

	ALLOC();

	res->fns = 0;

	res->cap = n_values == 0 ? 16 : n_values;

	res->values = nalloc(sizeof(*res->values), res->cap, &err);
	if (err != ERR_OK) {
		free(res);
		ERR_WITH(err, NULL);
	}
	res->len = n_values;

	for (i = 0; i < n_values; ++i) {
		res->values[i] = value_copy(values[i]);
		res->fns += value_fns(res->values[i]);
	}

	return res;
}
List_own
list_deepcopy(List_ref this, err_t ref err_out)
{ return list_new(this->values, this->len, err_out); }
List_own
list_tail(List_ref this, usz from_idx, err_t ref err_out)
{
	if (from_idx >= this->len) {
		return list_new(NULL, 0, err_out);
	} else {
		return list_new(
			this->values + from_idx,
			this->len - from_idx,
			err_out
		);
	}
}
List_own
list_copy(List_ref this)
{
	/* to make environment cycle counts accurate */
	if (this->fns > 0) {
		usz i;
		for (i = 0; i < this->len; ++i) {
			value_copy(this->values[i]);
		}
	}

	COPY(List);
}
void
list_free(List_own this)
{
	FREE_PRELUDE(List);

	/* to make environment cycle counts accurate */
	if (this->fns > 0) {
		usz i;
		for (i = 0; i < this->len; ++i) {
			value_free(this->values[i]);
		}
	}

	if (this->ref_count == 0) {
		if (this->fns == 0 && this->values != NULL) {
			usz i;
			for (i = 0; i < this->len; ++i) {
				value_free(this->values[i]);
			}
			free(this->values);
		}
		free((own_ptr)this);
	}
}

bool
list_iseq(List_ref this, List_ref other)
{
	usz i;

	if (this == other) return true;
	if (this->len != other->len) return false;

	for (i = 0; i < this->len; ++i) {
		if (!value_iseq(this->values[i], other->values[i])) {
			return false;
		}
	}

	return true;
}
void
list_print(List_ref this, char open, char close, FILE ref file,
	   err_t ref err_out)
{
	err_t err = ERR_OK;
	usz i;

	fprintc(open, file, &err);
	TRY_VOID(err);

	for (i = 0; i < this->len; ++i) {
		if (i != 0) {
			fprintc(' ', file, &err);
			TRY_VOID(err);
		}

		value_print(this->values[i], true, file, &err);
		TRY_VOID(err);
	}

	fprintc(close, file, &err);
	TRY_VOID(err);
}
usz
list_len(List_ref this)
{
	return this->len;
}
Value_ref
list_nth(List_ref this, usz n, err_t ref err_out)
{
	if (n >= this->len) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this->values[n];
}
usz
list_fns(List_ref this)
{
	return this->fns;
}
/* conceptually operates on constant values:
 * the contents of the list does not change.
 * returns ERR_INVAL if newcap < this->len
 */
/* WARN: if the reallocs fail,
 * the list is left in an invalid state,
 * and must be immediately free'd by the caller!
 */
void
_list_grow(List_ref this, usz newcap, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct List_struct ref mut_this = (struct List_struct ref)this;

	if (newcap < this->len) {
		ERR_VOID(ERR_INVAL);
	}

	mut_this->cap = newcap;

	mut_this->values = nrealloc(
		mut_this->values,
		sizeof(*mut_this->values),
		mut_this->cap,
		&err
	);
	if (err != ERR_OK) {
		mut_this->values = NULL;
		/* NOTE: theoretically,
		 * we should call `free_value` on all the values,
		 * but if something went wrong with realloc,
		 * we really don't know what happened to that memory
		 * (is it deallocated yet? presumably)
		 * so we'll just have to leak them
		 */
		ERR_VOID(err);
	}
}
List_own
_list_extend(struct List_struct own this, Value_ref ref values, usz n_values,
	     err_t ref err_out)
{
	err_t err = ERR_OK;
	usz i;

	if (this->ref_count != 1) {
		list_free(this);

		ERR_WITH(ERR_PERM, NULL);
	}

	if (n_values == 0) return this;

	if (this->len + n_values > this->cap) {
		while (this->cap < this->len + n_values) {
			this->cap *= 2;
		}
		_list_grow(this, this->cap, &err);
		if (err != ERR_OK) {
			free(this);
			ERR_WITH(err, NULL);
		}
	}

	for (i = 0; i < n_values; ++i) {
		this->values[this->len + i] = value_copy(values[i]);
		this->fns += value_fns(this->values[this->len + i]);
	}
	this->len += n_values;

	return this;
}
List_own
_list_append(struct List_struct own this, Value_own value, err_t ref err_out)
{
	err_t err = ERR_OK;

	if (this->ref_count != 1) {
		list_free(this);

		ERR_WITH(ERR_PERM, NULL);
	}

	if (this->len+1 > this->cap) {
		this->cap *= 2;
		_list_grow(this, this->cap, &err);
		if (err != ERR_OK) {
			free(this);
			ERR_WITH(err, NULL);
		}
	}

	this->values[this->len] = value;
	this->fns += value_fns(this->values[this->len]);
	++this->len;

	return this;
}
List_own
_list_set(struct List_struct own this, usz idx, Value_own value,
	  err_t ref err_out)
{
	if (idx >= this->len) {
		list_free(this);
		value_free(value);
		ERR_WITH(ERR_INVAL, NULL);
	}

	this->fns -= value_fns(this->values[idx]);
	value_free(this->values[idx]);
	this->values[idx] = value;
	this->fns += value_fns(this->values[idx]);

	return this;
}

List_iter
list_iter(List_ref this)
{
	struct List_iter res;

	res.list = this;
	res.pos = 0;

	return res;
}
List_iter
list_next(List_iter this)
{
	struct List_iter res = this;

	++res.pos;
	if (res.pos > res.list->len) {
		res.pos = res.list->len;
	}

	return res;
}
bool
list_isend(List_iter this)
{
	return this.pos >= this.list->len;
}
Value_ref
list_at(List_iter this, err_t ref err_out)
{
	if (this.pos >= this.list->len) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this.list->values[this.pos];
}
List_own
_list_at_set(struct List_struct own this, List_iter at, Value_own value,
	     err_t ref err_out)
{
	if (at.list != this) {
		list_free(this);
		value_free(value);
		ERR_WITH(ERR_INVAL, NULL);
	}

	if (at.pos >= this->len) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	this->fns -= value_fns(this->values[at.pos]);
	value_free(this->values[at.pos]);
	this->values[at.pos] = value;
	this->fns += value_fns(this->values[at.pos]);

	return this;
}

struct HashMap_struct {
	/* TODO: turn this into an actual hashmap,
	 * rather than a simple associative array
	 */
	String_own own keys;
	Value_own own values;
	usz len;
	usz cap;

	usz fns;
	usz ref_count;
};
HashMap_own
hashmap_new(String_ref ref keys, Value_ref ref values, usz n_items,
	    err_t ref err_out)
{
	err_t err = ERR_OK;
	usz i;
	struct HashMap_struct own res;

	ALLOC();

	res->cap = n_items == 0 ? 16 : n_items;

	res->keys = nalloc(sizeof(*res->keys), res->cap, &err);
	if (err != ERR_OK) {
		free(res);
		ERR_WITH(err, NULL);
	}
	res->values = nalloc(sizeof(*res->values), res->cap, &err);
	if (err != ERR_OK) {
		free(res->keys);
		free(res);
		ERR_WITH(err, NULL);
	}
	res->len = n_items;

	for (i = 0; i < n_items; ++i) {
		res->keys[i] = string_copy(keys[i]);
		res->values[i] = value_copy(values[i]);
		res->fns += value_fns(res->values[i]);
	}

	return res;
}
HashMap_own
hashmap_deepcopy(HashMap_ref this, err_t ref err_out)
{ return hashmap_new(this->keys, this->values, this->len, err_out); }
HashMap_own
hashmap_copy(HashMap_ref this)
{
	/* to make environment cycle counts accurate */
	if (this->fns > 0) {
		usz i;
		for (i = 0; i < this->len; ++i) {
			value_copy(this->values[i]);
		}
	}

	COPY(HashMap);
}
void
hashmap_free(HashMap_own this)
{
	FREE_PRELUDE(HashMap);

	/* to make environment cycle counts accurate */
	if (this->fns > 0) {
		usz i;
		for (i = 0; i < this->len; ++i) {
			value_free(this->values[i]);
		}
	}

	if (this->ref_count == 0) {
		if (this->fns == 0 && this->keys != NULL) {
			usz i;
			for (i = 0; i < this->len; ++i) {
				string_free(this->keys[i]);
			}
			free(this->keys);
		}
		if (this->values != NULL) {
			usz i;
			for (i = 0; i < this->len; ++i) {
				value_free(this->values[i]);
			}
			free(this->values);
		}
		free((own_ptr)this);
	}
}

bool
hashmap_iseq(HashMap_ref this, HashMap_ref other)
{
	err_t err = ERR_OK;
	usz i;
	Value_ref val;

	if (this == other) return true;
	if (this->len != other->len) return false;

	for (i = 0; i < this->len; ++i) {
		val = hashmap_get(other, this->keys[i], &err);

		if (err != ERR_OK || !value_iseq(this->values[i], val)) {
			return false;
		}
	}

	return true;
}
void
hashmap_print(HashMap_ref this, FILE ref file, err_t ref err_out)
{
	err_t err = ERR_OK;
	usz i;

	fprintc('{', file, &err);
	TRY_VOID(err);

	for (i = 0; i < this->len; ++i) {
		if (i != 0) {
			fprintc(' ', file, &err);
			TRY_VOID(err);
		}

		string_print(this->keys[i], true, file, &err);
		TRY_VOID(err);
		fprintc(' ', file, &err);
		TRY_VOID(err);
		value_print(this->values[i], true, file, &err);
		TRY_VOID(err);
	}

	fprintc('}', file, &err);
	TRY_VOID(err);
}
usz
hashmap_size(HashMap_ref this)
{
	return this->len;
}
bool
hashmap_contains(HashMap_ref this, String_ref key)
{
	usz i;

	for (i = 0; i < this->len; ++i) {
		if (string_iseq(key, this->keys[i])) return true;
	}

	return false;
}
Value_ref
hashmap_get(HashMap_ref this, String_ref key, err_t ref err_out)
{
	usz i;

	for (i = 0; i < this->len; ++i) {
		if (string_iseq(key, this->keys[i])) return this->values[i];
	}

	ERR_WITH(ERR_INVAL, NULL);
}
HashMap_own
hashmap_set(HashMap_ref this, String_own key, Value_own value,
	    err_t ref err_out)
{
	err_t err = ERR_OK;
	HashMap_own res;

	res = hashmap_deepcopy(this, &err);
	if (err != ERR_OK) {
		string_free(key);
		value_free(value);
		ERR_WITH(err, NULL);
	}

	return _hashmap_assoc(
		(struct HashMap_struct own)res,
		key,
		value,
		err_out
	);
}
HashMap_own
hashmap_union(HashMap_ref this, HashMap_ref other, err_t ref err_out)
{
	/* WARN: currently time complexity is O(n^2),
	 * need to implement a proper hashmap structure
	 * to reduce time complexity down to O(n log n)
	 */
	err_t err = ERR_OK;
	HashMap_own res;

	res = hashmap_deepcopy(this, &err);
	TRY_WITH(err, NULL);

	return _hashmap_extend(
		(struct HashMap_struct own)res,
		other->keys,
		other->values,
		other->len,
		err_out
	);
}
usz
hashmap_fns(HashMap_ref this)
{
	return this->fns;
}
/* conceptually operates on constant values:
 * the contents of the hash-map does not change.
 * returns ERR_INVAL if newcap < this->len
 */
/* WARN: if the reallocs fail,
 * the hash-map is left in an invalid state,
 * and must be immediately free'd by the caller!
 */
void
_hashmap_grow(HashMap_ref this, usz newcap, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct HashMap_struct ref mut_this = (struct HashMap_struct ref)this;

	if (newcap < this->len) {
		ERR_VOID(ERR_INVAL);
	}

	mut_this->cap = newcap;

	mut_this->keys = nrealloc(
		mut_this->keys,
		sizeof(*mut_this->keys),
		mut_this->cap,
		&err
	);
	if (err != ERR_OK) {
		mut_this->keys = NULL;
		/* NOTE: theoretically,
		 * we should call `free_string` on all the keys,
		 * but if something went wrong with realloc,
		 * we really don't know what happened to that memory
		 * (is it deallocated yet? presumably)
		 * so we'll just have to leak them
		 */
		ERR_VOID(err);
	}
	mut_this->values = nrealloc(
		mut_this->values,
		sizeof(*mut_this->values),
		mut_this->cap,
		&err
	);
	if (err != ERR_OK) {
		mut_this->values = NULL;
		/* NOTE: theoretically,
		 * we should call `free_value` on all the values,
		 * but if something went wrong with realloc,
		 * we really don't know what happened to that memory
		 * (is it deallocated yet? presumably)
		 * so we'll just have to leak them
		 */
		ERR_VOID(err);
	}
}
HashMap_own
_hashmap_assoc(struct HashMap_struct own this, String_own key, Value_own value,
	       err_t ref err_out)
{
	err_t err = ERR_OK;
	usz i;

	if (this->ref_count != 1) {
		hashmap_free(this);
		string_free(key);
		value_free(value);

		ERR_WITH(ERR_PERM, NULL);
	}

	for (i = 0; i < this->len; ++i) {
		if (string_iseq(key, this->keys[i])) {
			string_free(key);
			this->fns -= value_fns(this->values[i]);
			value_free(this->values[i]);
			this->values[i] = value;
			this->fns += value_fns(this->values[i]);
			return this;
		}
	}

	if (this->len == this->cap) {
		_hashmap_grow(this, this->cap*2, &err);
		if (err != ERR_OK) {
			hashmap_free(this);
			ERR_WITH(err, NULL);
		}
	}

	this->keys[this->len] = key;
	this->values[this->len] = value;
	this->fns += value_fns(this->values[i]);
	++this->len;

	return this;
}
HashMap_own
_hashmap_extend(struct HashMap_struct own this, String_ref ref keys,
		Value_ref ref values, usz n_items, err_t ref err_out)
{
	/* WARN: currently time complexity is O(n^2),
	 * need to implement a proper hashmap structure
	 * to reduce time complexity down to O(n log n)
	 */

	err_t err = ERR_OK;
	usz i;

	for (i = 0; i < n_items; ++i) {
		this = (struct HashMap_struct own)_hashmap_assoc(
			this,
			keys[i],
			values[i],
			&err
		);
		/* in the case of an error,
		 * `_hashmap_assoc` will take care of cleanup
		 * */
		TRY_WITH(err, NULL);
	}

	return this;
}

HashMap_iter
hashmap_iter(HashMap_ref this)
{
	struct HashMap_iter res;

	res.hashmap = this;
	res.pos = 0;

	return res;
}
HashMap_iter
hashmap_next(HashMap_iter this)
{
	struct HashMap_iter res = this;

	++res.pos;
	if (res.pos > res.hashmap->len) {
		res.pos = res.hashmap->len;
	}

	return res;
}
bool
hashmap_isend(HashMap_iter this)
{
	return this.pos >= this.hashmap->len;
}
String_ref
hashmap_keyat(HashMap_iter this, err_t ref err_out)
{
	if (this.pos >= this.hashmap->len) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this.hashmap->keys[this.pos];
}
Value_ref
hashmap_valueat(HashMap_iter this, err_t ref err_out)
{
	if (this.pos >= this.hashmap->len) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this.hashmap->values[this.pos];
}
HashMap_own
_hashmap_at_set(struct HashMap_struct own this, HashMap_iter at,
		Value_own value, err_t ref err_out)
{
	if (at.hashmap != this) {
		hashmap_free(this);
		value_free(value);
		ERR_WITH(ERR_INVAL, NULL);
	}

	if (at.pos >= this->len) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	this->fns -= value_fns(this->values[at.pos]);
	value_free(this->values[at.pos]);
	this->values[at.pos] = value;
	this->fns += value_fns(this->values[at.pos]);

	return this;
}

#include "env.h"

struct Fn_struct {
	bool is_builtin;
	Env_own closure;
	union Fn_union {
		builtin_fn builtin;
		struct MalFn_struct {
			String_own own args;
			usz n_args;
			bool variadic;
			Value_own body;
		} mal;
	} f;

	usz ref_count;
};
Fn_own
fn_builtin(builtin_fn fn, Env_own env, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Fn_struct own res;

	ALLOC();

	res->is_builtin = true;
	res->closure = env;
	res->f.builtin = fn;

	return res;
}
Fn_own
fn_new(Value_own body, String_own own args, usz n_args, bool variadic,
       Env_own env, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Fn_struct own res;

	res = alloc(sizeof(*res), &err);
	if (err != ERR_OK) {
		usz i;
		value_free(body);
		for (i = 0; i < n_args; ++i) {
			string_free(args[i]);
		}
		free(args);
		ERR_WITH(err, NULL);
	}
	res->ref_count = 1;

	res->is_builtin = false;
	res->closure = env;
	res->f.mal.args = args;
	res->f.mal.n_args = n_args;
	res->f.mal.variadic = variadic;
	res->f.mal.body = body;

	return res;
}
Fn_own
fn_copy(Fn_ref this)
{
	/* to make environment cycle counts accurate */
	if (this->closure != NULL) (void)env_copy_const(this->closure);

	COPY(Fn);
}
void
fn_free(Fn_own this)
{
	FREE_PRELUDE(Fn);

	/* to make environment cycle counts accurate */
	if (this->closure != NULL) env_free(this->closure);

	if (this->ref_count == 0) {
		if (this->is_builtin) {
			/* nothing to do */
		} else {
			value_free(this->f.mal.body);
			if (this->f.mal.args != NULL) {
				usz i;
				for (i = 0; i < this->f.mal.n_args; ++i) {
					string_free(this->f.mal.args[i]);
				}
				free(this->f.mal.args);
			}
		}
	}
}

bool
fn_isbuiltin(Fn_ref this)
{
	return this->is_builtin;
}
builtin_fn
fn_getbuiltin(Fn_ref this, err_t ref err_out)
{
	if (!this->is_builtin) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this->f.builtin;
}
String_ref ref
fn_getargs(Fn_ref this, err_t ref err_out)
{
	if (this->is_builtin) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this->f.mal.args;
}
usz
fn_getnargs(Fn_ref this, err_t ref err_out)
{
	if (this->is_builtin) {
		ERR_WITH(ERR_INVAL, 0);
	}

	return this->f.mal.n_args;
}
bool
fn_isvariadic(Fn_ref this, err_t ref err_out)
{
	if (this->is_builtin) {
		ERR_WITH(ERR_INVAL, false);
	}

	return this->f.mal.variadic;
}
Value_ref
fn_getbody(Fn_ref this, err_t ref err_out)
{
	if (this->is_builtin) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this->f.mal.body;
}

#include "env.h"

bool
fn_iseq(Fn_ref this, Fn_ref other)
{
	if (this == other) return true;
	if (this->is_builtin != other->is_builtin) return false;
	if (this->closure != other->closure) return false;

	if (this->is_builtin) {
		return this->f.builtin == other->f.builtin;
	} else {
		/* theoretically,
		 * I could test here that they take the same arguments,
		 * and then I could test that the bodies are the same,
		 * but that's too much work for now.
		 * Maybe later.
		 */
		return false;
	}
}
void
fn_print(Fn_ref this, FILE ref file, err_t ref err_out)
{
	err_t err = ERR_OK;

	if (this->is_builtin) {
		fprints("<BUILTIN FUNCTION 0x", file, &err);
		TRY_VOID(err);
		fprintuzx((usz)this->f.builtin, file, &err);
		TRY_VOID(err);
		fprintc('>', file, &err);
		TRY_VOID(err);
	} else {
		fprintc('<', file, &err);
		TRY_VOID(err);
		if (this->f.mal.variadic) {
			fprints("VARIADIC ", file, &err);
			TRY_VOID(err);
		}
		fprints("FUNCTION WITH ", file, &err);
		TRY_VOID(err);
		fprintuz(this->f.mal.n_args, file, &err);
		TRY_VOID(err);
		fprints(" ARGUMENTS>", file, &err);
		TRY_VOID(err);
	}
}
Value_own
fn_call(Fn_ref this, List_own args, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Env_own outer;
	MutEnv_own env;
	Value_own res;

	if (fn_EVAL == NULL) {
		fprints(
			"ERROR: calling `fn_call` without defining value evaluation!\n",
			stderr,
			NULL
		);
		exit(1);
	}

	outer = this->closure == NULL ? NULL : env_copy_const(this->closure);
	env = env_new(outer, &err.e.errt);
	RTRY_WITH(err, NULL);

	if (this->is_builtin) {
		res = this->f.builtin(args, env, &err);
		if (!rerr_is_ok(err)) {
			env_free(env);
			RERR_WITH(err, NULL);
		}
	} else {
		env_bind(
			env,
			this->f.mal.args,
			this->f.mal.n_args,
			this->f.mal.variadic,
			args,
			&err
		);
		if (!rerr_is_ok(err)) {
			env_free(env);
			RERR_WITH(err, NULL);
		}
		res = fn_EVAL(value_copy(this->f.mal.body), env, &err);
		if (!rerr_is_ok(err)) {
			env_free(env);
			RERR_WITH(err, NULL);
		}
	}

	env_free(env);

	return res;
}
Env_ref
fn_closure(Fn_ref this)
{
	return this->closure;
}
