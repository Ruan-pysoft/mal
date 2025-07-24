#include "values.h"

#include <ministd_fmt.h>
#include <ministd_memory.h>

struct Value_struct {
	enum Value_type type;
	union Value_union {
		String_own str;
		int num;
		List_own lst;
		bool boo;
		HashMap_own map;
		Fn_own fnc;
	} v;

	usz fns;
	usz ref_count;
};

static struct Value_struct own
_value_alloc(enum Value_type type, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Value_struct own res;

	res = alloc(sizeof(*res), &err);
	TRY_WITH(err, NULL);
	res->ref_count = 1;

	res->type = type;

	return res;
}
Value_own
value_symbol(String_own symbol, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Value_struct own res = _value_alloc(VT_SYM, &err);
	if (err != ERR_OK) {
		string_free(symbol);
		ERR_WITH(err, NULL);
	}

	res->fns = 0;
	res->v.str = symbol;

	return res;
}
Value_own
value_number(int number, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Value_struct own res = _value_alloc(VT_NUM, &err);
	TRY_WITH(err, NULL);

	res->fns = 0;
	res->v.num = number;

	return res;
}
Value_own
value_list(List_own list, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Value_struct own res = _value_alloc(VT_LST, &err);
	if (err != ERR_OK) {
		list_free(list);
		ERR_WITH(err, NULL);
	}

	res->fns = list_fns(list);
	res->v.lst = list;

	return res;
}
Value_own
value_string(String_own string, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Value_struct own res = _value_alloc(VT_STR, &err);
	if (err != ERR_OK) {
		string_free(string);
		ERR_WITH(err, NULL);
	}

	res->fns = 0;
	res->v.str = string;

	return res;
}
Value_own
value_nil(err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Value_struct own res = _value_alloc(VT_NIL, &err);
	TRY_WITH(err, NULL);

	res->fns = 0;

	return res;
}
Value_own
value_bool(bool boolean, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Value_struct own res = _value_alloc(VT_BOO, &err);
	TRY_WITH(err, NULL);

	res->fns = 0;
	res->v.boo = boolean;

	return res;
}
Value_own
value_keyword(String_own keyword, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Value_struct own res = _value_alloc(VT_KEY, &err);
	if (err != ERR_OK) {
		string_free(keyword);
		ERR_WITH(err, NULL);
	}

	res->fns = 0;
	res->v.str = keyword;

	return res;
}
Value_own
value_vector(List_own vector, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Value_struct own res = _value_alloc(VT_VEC, &err);
	if (err != ERR_OK) {
		list_free(vector);
		ERR_WITH(err, NULL);
	}

	res->fns = list_fns(vector);
	res->v.lst = vector;

	return res;
}
Value_own
value_hashmap(HashMap_own hashmap, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Value_struct own res = _value_alloc(VT_MAP, &err);
	if (err != ERR_OK) {
		hashmap_free(hashmap);
		ERR_WITH(err, NULL);
	}

	res->fns = hashmap_fns(hashmap);
	res->v.map = hashmap;

	return res;
}
Value_own
value_fn(Fn_own fn, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct Value_struct own res = _value_alloc(VT_FNC, &err);
	if (err != ERR_OK) {
		fn_free(fn);
		ERR_WITH(err, NULL);
	}

	res->fns = 1;
	res->v.fnc = fn;

	return res;
}
Value_own
value_copy(Value_ref this)
{
	++((struct Value_struct ref)this)->ref_count;

	/* to make environment cycle counts accurate */
	switch (this->type) {
		case VT_SYM:
		case VT_STR:
		case VT_KEY: {
			string_copy(this->v.str);
		break; }
		case VT_NUM: break;
		case VT_LST:
		case VT_VEC: {
			list_copy(this->v.lst);
		break; }
		case VT_NIL: break;
		case VT_BOO: break;
		case VT_MAP: {
			hashmap_copy(this->v.map);
		break; }
		case VT_FNC: {
			fn_copy(this->v.fnc);
		break; }
	}

	return (Value_own)this;
}
void
value_free(Value_own this)
{
	if (this == NULL) return;

	--((struct Value_struct ref)this)->ref_count;

	/* to make environment cycle counts accurate */
	switch (this->type) {
		case VT_SYM: {
			string_free(this->v.str);
		break; }
		case VT_NUM: break;
		case VT_LST: {
			list_free(this->v.lst);
		break; }
		case VT_STR: {
			string_free(this->v.str);
		break; }
		case VT_NIL: break;
		case VT_BOO: break;
		case VT_KEY: {
			string_free(this->v.str);
		break; }
		case VT_VEC: {
			list_free(this->v.lst);
		break; }
		case VT_MAP: {
			hashmap_free(this->v.map);
		break; }
		case VT_FNC: {
			fn_free(this->v.fnc);
		break; }
	}
	if (this->ref_count == 0) {
		free((own_ptr)this);
	}
}

bool
value_iseq(Value_ref this, Value_ref other)
{
	if (this == other) return true;
	if (this->type != other->type) return false;

	switch (this->type) {
		case VT_SYM:
		case VT_STR:
		case VT_KEY: {
			return string_iseq(this->v.str, other->v.str);
		break; }
		case VT_NUM: {
			return this->v.num == other->v.num;
		break; }
		case VT_LST:
		case VT_VEC: {
			return list_iseq(this->v.lst, other->v.lst);
		break; }
		case VT_NIL: return true;
		case VT_BOO: {
			return this->v.boo == other->v.boo;
		break; }
		case VT_MAP: {
			return hashmap_iseq(this->v.map, other->v.map);
		break; }
		case VT_FNC: {
			return fn_iseq(this->v.fnc, other->v.fnc);
		break; }
	}

	/* WARN: execution should never reach this point
	 * however, I'm adding it here, otherwise gcc complains...
	 */
	return false;
}
void
value_print(Value_ref this, bool repr, FILE ref file, err_t ref err_out)
{
	switch (this->type) {
		case VT_SYM:
		case VT_STR:
		case VT_KEY: {
			string_print(this->v.str, repr, file, err_out);
		break; }
		case VT_NUM: {
			fprinti(this->v.num, file, err_out);
		break; }
		case VT_LST: {
			list_print(this->v.lst, '(', ')', file, err_out);
		break; }
		case VT_NIL: {
			fprints("nil", file, err_out);
		break; }
		case VT_BOO: {
			fprints(this->v.boo ? "true" : "false", file, err_out);
		break; }
		case VT_VEC: {
			list_print(this->v.lst, '[', ']', file, err_out);
		break; }
		case VT_MAP: {
			hashmap_print(this->v.map, file, err_out);
		break; }
		case VT_FNC: {
			fn_print(this->v.fnc, file, err_out);
		break; }
	}
}
enum Value_type
value_type(Value_ref this)
{
	return this->type;
}
bool
value_issymbol(Value_ref this)
{
	return this->type == VT_SYM;
}
bool
value_isnumber(Value_ref this)
{
	return this->type == VT_NUM;
}
bool
value_islist(Value_ref this)
{
	return this->type == VT_LST;
}
bool
value_isstring(Value_ref this)
{
	return this->type == VT_STR;
}
bool
value_isnil(Value_ref this)
{
	return this->type == VT_NIL;
}
bool
value_isbool(Value_ref this)
{
	return this->type == VT_BOO;
}
bool
value_iskeyword(Value_ref this)
{
	return this->type == VT_KEY;
}
bool
value_isvector(Value_ref this)
{
	return this->type == VT_VEC;
}
bool
value_ishashmap(Value_ref this)
{
	return this->type == VT_MAP;
}
bool
value_isfn(Value_ref this)
{
	return this->type == VT_FNC;
}
String_ref
value_getsymbol(Value_ref this, err_t ref err_out)
{
	if (!value_issymbol(this)) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this->v.str;
}
int
value_getnumber(Value_ref this, err_t ref err_out)
{
	if (!value_isnumber(this)) {
		ERR_WITH(ERR_INVAL, 0);
	}

	return this->v.num;
}
List_ref
value_getlist(Value_ref this, err_t ref err_out)
{
	if (!value_islist(this)) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this->v.lst;
}
String_ref
value_getstring(Value_ref this, err_t ref err_out)
{
	if (!value_isstring(this)) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this->v.str;
}
bool
value_getbool(Value_ref this, err_t ref err_out)
{
	if (!value_isbool(this)) {
		ERR_WITH(ERR_INVAL, false);
	}

	return this->v.boo;
}
String_ref
value_getkeyword(Value_ref this, err_t ref err_out)
{
	if (!value_iskeyword(this)) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this->v.str;
}
List_ref
value_getvector(Value_ref this, err_t ref err_out)
{
	if (!value_isvector(this)) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this->v.lst;
}
HashMap_ref
value_gethashmap(Value_ref this, err_t ref err_out)
{
	if (!value_ishashmap(this)) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this->v.map;
}
Fn_ref
value_getfn(Value_ref this, err_t ref err_out)
{
	if (!value_isfn(this)) {
		ERR_WITH(ERR_INVAL, NULL);
	}

	return this->v.fnc;
}
usz
value_fns(Value_ref this)
{
	return this->fns;
}
