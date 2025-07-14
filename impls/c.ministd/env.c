#include "env.h"
#include "values.h"

#include <ministd_memory.h>
#include <ministd_string.h>

void
key_free(key_t own this)
{ mal_string_free(this); }
#ifndef FULL_ENV
bool
key_cmp(const key_t ref a, const key_t ref b)
#else
enum cmp_res
key_cmp(const key_t ref a, const key_t ref b)
#endif
{
	usz len, i;

#ifndef FULL_ENV
	if (a == b) return true;
#else
	if (a == b) return CR_EQ;
#endif

	len = strlen(a->data);
#ifndef FULL_ENV
	if (strnlen(b->data, len+1) != len) return false;
#endif

	for (i = 0; i < len; ++i) {
#ifndef FULL_ENV
		if (a->data[i] != b->data[i]) return false;
#else
		if (a->data[i] < b->data[i]) return CR_LT;
		if (a->data[i] > b->data[i]) return CR_GT;
#endif
	}

#ifndef FULL_ENV
	return true;
#else
	return CR_EQ;
#endif
}

struct env_t {
	const key_t own own keys;
	const value_t own own vals;
	usz len;
	usz cap;

#ifdef FULL_ENV
	const env_t own outer;
#endif
	usz ref_count;
};

#ifndef FULL_ENV
env_t own
env_new(err_t ref err_out)
#else
env_t own
env_new(const env_t own outer, err_t ref err_out)
#endif
{
	err_t err = ERR_OK;
	env_t own res;

	res = alloc(sizeof(*res), &err);
	TRY_WITH(err, NULL);

	res->cap = 16;
	res->len = 0;
	res->keys = alloc(sizeof(*res->keys) * res->cap, &err);
	if (err != ERR_OK) {
		free(res);
		ERR_WITH(err, NULL);
	}
	res->vals = alloc(sizeof(*res->vals) * res->cap, &err);
	if (err != ERR_OK) {
		free(res->keys);
		free(res);
		ERR_WITH(err, NULL);
	}

#ifdef FULL_ENV
	res->outer = outer;
#endif
	res->ref_count = 1;

	return res;
}
env_t own
env_copy(env_t ref this)
{
	++this->ref_count;

	return (env_t own)this;
}
void
env_free(env_t own this)
{
	usz i;

	if (this == NULL) return;

	--this->ref_count;

	if (this->ref_count == 0) {
		for (i = 0; i < this->len; ++i) {
			if (this->keys != NULL) key_free((ptr)this->keys[i]);
			if (this->vals != NULL) value_free(this->vals[i]);
		}
		if (this->keys != NULL) free(this->keys);
		if (this->vals != NULL) free(this->vals);

#ifdef FULL_ENV
		free((env_t own)this->outer);
#endif

		free(this);
	}
}
#ifndef FULL_ENV
void
env_add(env_t ref this, const key_t own key, const value_t own val,
	err_t ref err_out)
#else
void
env_set(env_t ref this, const key_t own key, const value_t own val,
	err_t ref err_out)
#endif
{
	err_t err = ERR_OK;
#ifdef FULL_ENV
	usz i;
#endif

	if (this->len == this->cap) {
		this->keys = realloc(
			this->keys,
			sizeof(*this->keys) * 2*this->cap,
			&err
		);
		TRY_VOID(err);

		this->vals = realloc(
			this->vals,
			sizeof(*this->vals) * 2*this->cap,
			&err
		);
		TRY_VOID(err);

		this->cap *= 2;
	}

#ifdef FULL_ENV
	for (i = 0; i < this->len; ++i) {
		if (key_cmp(key, this->keys[i]) == CR_EQ) {
			value_free(this->vals[i]);
			key_free((ptr)key);
			this->vals[i] = val;
			return;
		}
	}
#endif

	this->keys[this->len] = key;
	this->vals[this->len] = val;
	++this->len;
}
const value_t ref
env_get(const env_t ref this, const key_t ref key)
{
	usz i;

	for (i = 0; i < this->len; ++i) {
#ifndef FULL_ENV
		if (key_cmp(this->keys[i], key)) {
#else
		if (key_cmp(this->keys[i], key) == CR_EQ) {
#endif
			return this->vals[i];
		}
	}

#ifndef FULL_ENV
	return NULL;
#else
	return this->outer == NULL ? NULL : env_get(this->outer, key);
#endif
}
