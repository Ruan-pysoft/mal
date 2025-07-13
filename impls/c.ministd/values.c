#include "values.h"
#include "ministd_error.h"

#include <ministd_memory.h>
#include <ministd_string.h>

/* NOTE: cell can be `NULL` at any time to represent an empty list */
const struct cell own
cell_new(const struct value_t ref val, err_t ref err_out)
{
	return cell_new_own(value_copy(val), err_out);
}
const struct cell own
cell_new_own(const struct value_t own val, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct cell own res;

	res = alloc(sizeof(*res), &err);
	if (err != ERR_OK) {
		value_free(val);
		ERR_WITH(err, NULL);
	}

	res->val = val;
	res->next = NULL;
	res->ref_count = 1;

	return res;
}
const struct cell own
cell_cons(const struct value_t ref val, const struct cell ref rest,
	  err_t ref err_out)
{
	return cell_cons_own(value_copy(val), rest, err_out);
}
const struct cell own
cell_cons_own(const struct value_t own val, const struct cell ref rest,
	  err_t ref err_out)
{
	err_t err = ERR_OK;
	struct cell own res;

	res = (struct cell own)cell_new_own(val, &err);
	if (err != ERR_OK) {
		value_free(val);
		ERR_WITH(err, NULL);
	}

	res->next = cell_copy(rest);

	return res;
}
const struct cell own
cell_copy(const struct cell ref this)
{
	if (this == NULL) return NULL;

	++((struct cell ref)this)->ref_count;

	return (const struct cell own)this;
}
void
cell_free(const struct cell own this)
{
	if (this == NULL) return;

	--((struct cell own)this)->ref_count;

	if (this->ref_count == 0) {
		value_free(this->val);
		cell_free(this->next);
		free((own_ptr)this);
	}
}
const struct cell ref
cell_tail(const struct cell ref this)
{
	const struct cell ref tail;

	if (this == NULL) return NULL;

	tail = this;
	while (tail->next != NULL) tail = tail->next;

	return tail;
}
void
_cell_unsafe_append(struct cell ref this, const struct value_t own val,
		    err_t ref err_out)
{
	struct cell ref tail;

	if (this == NULL) ERR_VOID(ERR_INVAL);

	tail = (struct cell ref)cell_tail(this);

	tail->next = cell_new_own(val, err_out);
}

const struct mal_string own
mal_string_new(const char ref cstr, err_t ref err_out)
{
	return mal_string_newn(cstr, strlen(cstr), err_out);
}
const struct mal_string own
mal_string_newn(const char ref cstr, usz len, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct mal_string own res;

	res = alloc(sizeof(*res), &err);
	TRY_WITH(err, NULL);

	res->data = alloc(sizeof(char) * (len+1), &err);
	if (err != ERR_OK) {
		free(res);
		ERR_WITH(err, NULL);
	}
	memmove((own_ptr)res->data, cstr, sizeof(char) * len);
	((char ref)res->data)[len] = 0;
	res->ref_count = 1;

	return res;
}
const struct mal_string own
mal_string_copy(const struct mal_string ref this)
{
	++((struct mal_string ref)this)->ref_count;

	return (const struct mal_string own)this;
}
void
mal_string_free(const struct mal_string own this)
{
	--((struct mal_string own)this)->ref_count;

	if (this->ref_count == 0) {
		free((own_ptr)this->data);
		free((own_ptr)this);
	}
}

static value_t own
_value_new(enum value_type type, union value_union val, err_t ref err_out)
{
	err_t err = ERR_OK;
	value_t own res;

	res = alloc(sizeof(*res), &err);
	TRY_WITH(err, NULL);

	res->type = type;
	res->v = val;
	res->ref_count = 1;

	return res;
}
const value_t own
value_list_own(const struct cell own cell, err_t ref err_out)
{
	union value_union val;
	val.ls = cell;
	return _value_new(VT_LIST, val, err_out);
}
const value_t own
value_sym_own(const struct mal_string own sym, err_t ref err_out)
{
	union value_union val;
	val.sym = sym;
	return _value_new(VT_SYM, val, err_out);
}
const value_t own
value_str_own(const struct mal_string own str, err_t ref err_out)
{
	union value_union val;
	val.str = str;
	return _value_new(VT_STR, val, err_out);
}
const value_t own
value_key_own(const struct mal_string own key, err_t ref err_out)
{
	union value_union val;
	val.key = key;
	return _value_new(VT_KEY, val, err_out);
}
const value_t own
value_list(const struct cell ref cell, err_t ref err_out)
{
	return value_list_own(cell_copy(cell), err_out);
}
const value_t own
value_num(int num, err_t ref err_out)
{
	union value_union val;
	val.num = num;
	return _value_new(VT_NUM, val, err_out);
}
const value_t own
value_sym(const struct mal_string ref sym, err_t ref err_out)
{
	return value_sym_own(mal_string_copy(sym), err_out);
}
const value_t own
value_nil(err_t ref err_out)
{
	union value_union val;
	memzero(&val, sizeof(val));
	return _value_new(VT_NIL, val, err_out);
}
const value_t own
value_bool(bool boo, err_t ref err_out)
{
	union value_union val;
	val.boo = boo;
	return _value_new(VT_BOOL, val, err_out);
}
const value_t own
value_str(const struct mal_string ref str, err_t ref err_out)
{
	return value_str_own(mal_string_copy(str), err_out);
}
const value_t own
value_key(const struct mal_string ref key, err_t ref err_out)
{
	return value_key_own(mal_string_copy(key), err_out);
}
const value_t own
value_builtin(builtin_fn builtin, err_t ref err_out)
{
	union value_union val;
	val.builtin = builtin;
	return _value_new(VT_BUILTIN_FN, val, err_out);
}
const value_t own
value_copy(const value_t ref this)
{
	++((value_t ref)this)->ref_count;

	return (const value_t own)this;
}
void
value_free(const value_t own this)
{
	--((value_t own)this)->ref_count;

	if (this->ref_count == 0) {
		switch (this->type) {
			case VT_LIST: {
				cell_free(this->v.ls);
			break; }
			case VT_NUM: break;
			case VT_SYM: {
				mal_string_free(this->v.sym);
			break; }
			case VT_NIL: break;
			case VT_BOOL: break;
			case VT_STR: {
				mal_string_free(this->v.str);
			break; }
			case VT_KEY: {
				mal_string_free(this->v.key);
			break; }
			case VT_BUILTIN_FN: break;
		}
		free((own_ptr)this);
	}
}
