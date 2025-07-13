#include "env.h"

#include <ministd_memory.h>
#include <ministd_string.h>

void
key_free(key_t own this)
{ mal_string_free(this); }
bool
key_cmp(const key_t ref a, const key_t ref b)
{
	usz len, i;

	if (a == b) return true;

	len = strlen(a->data);
	if (strnlen(b->data, len+1) != len) return false;

	for (i = 0; i < len; ++i) {
		if (a->data[i] != b->data[i]) return false;
	}

	return true;
}

struct env_t {
	const key_t own own keys;
	const value_t own own vals;
	usz len;
	usz cap;
};

env_t own
env_new(err_t ref err_out)
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

	return res;
}
void
env_free(env_t own this)
{
	usz i;

	for (i = 0; i < this->len; ++i) {
		if (this->keys != NULL) key_free((ptr)this->keys[i]);
		if (this->vals != NULL) value_free(this->vals[i]);
	}
	if (this->keys != NULL) free(this->keys);
	if (this->vals != NULL) free(this->vals);

	free(this);
}
void
env_add(env_t ref this, const key_t own key, const value_t own val,
	err_t ref err_out)
{
	err_t err = ERR_OK;

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

	this->keys[this->len] = key;
	this->vals[this->len] = val;
	++this->len;
}
const value_t ref
env_get(env_t ref this, const key_t ref key)
{
	usz i;

	for (i = 0; i < this->len; ++i) {
		if (key_cmp(this->keys[i], key)) {
			return this->vals[i];
		}
	}

	return NULL;
}
