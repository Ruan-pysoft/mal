#ifndef VALUES_H
#define VALUES_H

#include <ministd_types.h>

#include "error.h"
#include "types.h"

enum Value_type {
	VT_SYM,
	VT_NUM,
	VT_LST,
	VT_STR,
	VT_NIL,
	VT_BOO,
	VT_KEY,
	VT_VEC,
	VT_MAP,
	VT_FNC
};

Value_own value_symbol(String_own symbol, err_t ref err_out);
Value_own value_number(int number, err_t ref err_out);
Value_own value_list(List_own list, err_t ref err_out);
Value_own value_string(String_own string, err_t ref err_out);
Value_own value_nil(err_t ref err_out);
Value_own value_bool(bool boolean, err_t ref err_out);
Value_own value_keyword(String_own keyword, err_t ref err_out);
Value_own value_vector(List_own vector, err_t ref err_out);
Value_own value_hashmap(HashMap_own hashmap, err_t ref err_out);
Value_own value_fn(Fn_own fn, err_t ref err_out);
Value_own value_copy(Value_ref this);
void value_free(Value_own this);

void value_print(Value_ref this, bool repr, FILE ref file, err_t ref err_out);
enum Value_type value_type(Value_ref this);
bool value_issymbol(Value_ref this);
bool value_isnumber(Value_ref this);
bool value_islist(Value_ref this);
bool value_isstring(Value_ref this);
bool value_isnil(Value_ref this);
bool value_isbool(Value_ref this);
bool value_iskeyword(Value_ref this);
bool value_isvector(Value_ref this);
bool value_ishashmap(Value_ref this);
bool value_isfn(Value_ref this);
/* error with ERR_INVAL for !value_issymbol(this) */
String_ref value_getsymbol(Value_ref this, err_t ref err_out);
/* error with ERR_INVAL for !value_isnumber(this) */
int value_getnumber(Value_ref this, err_t ref err_out);
/* error with ERR_INVAL for !value_islist(this) */
List_ref value_getlist(Value_ref this, err_t ref err_out);
/* error with ERR_INVAL for !value_isstring(this) */
String_ref value_getstring(Value_ref this, err_t ref err_out);
/* error with ERR_INVAL for !value_isbool(this) */
bool value_getbool(Value_ref this, err_t ref err_out);
/* error with ERR_INVAL for !value_iskeyword(this) */
String_ref value_getkeyword(Value_ref this, err_t ref err_out);
/* error with ERR_INVAL for !value_isvector(this) */
List_ref value_getvector(Value_ref this, err_t ref err_out);
/* error with ERR_INVAL for !value_ishashmap(this) */
HashMap_ref value_gethashmap(Value_ref this, err_t ref err_out);
/* error with ERR_INVAL for !value_isfn(this) */
Fn_ref value_getfn(Value_ref this, err_t ref err_out);
usz value_fns(Value_ref this);

#endif /* VALUES_H */
