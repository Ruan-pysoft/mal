#include "core.h"

#include <ministd_fmt.h>
#include <ministd_memory.h>

#include "env.h"
#include "error.h"
#include "printer.h"
#include "values.h"

void
core_load(MutEnv_ref env, err_t ref err_out)
{
	err_t err = ERR_OK;
	usz i;

	for (i = 0; i < CORE_LEN; ++i) {
		String_own name;
		Fn_own func;
		Value_own value;

		name = string_new(core_names[i], &err);
		TRY_VOID(err);
		func = fn_builtin(core_funcs[i], NULL, &err);
		if (err != ERR_OK) {
			string_free(name);
			ERR_VOID(err);
		}
		value = value_fn(func, &err);
		if (err != ERR_OK) {
			string_free(name);
			ERR_VOID(err);
		}

		env_set(env, name, value, &err);
		TRY_VOID(err);
	}
}

static Value_own op_add(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own op_sub(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own op_mul(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own op_div(List_own args, MutEnv_ref env, rerr_t ref err_out);

static Value_own prn(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own list(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own islist(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own isempty(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own count(List_own args, MutEnv_ref env, rerr_t ref err_out);

static Value_own cmp_eq(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own cmp_lt(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own cmp_le(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own cmp_gt(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own cmp_ge(List_own args, MutEnv_ref env, rerr_t ref err_out);

const char ref core_names[CORE_LEN] = {
	"+", "-", "*", "/",
	"prn", "list", "list?", "empty?", "count", "=",
	"<", "<=", ">", ">=",
};
builtin_fn core_funcs[CORE_LEN] = {
	op_add, op_sub, op_mul, op_div,
	prn, list, islist, isempty, count, cmp_eq,
	cmp_lt, cmp_le, cmp_gt, cmp_ge,
};

#define ARYTH_OP(op) do { \
		rerr_t err = RERR_OK; \
		Value_ref a, b; \
		Value_own res; \
		int na, nb, nres; \
		if (list_len(args) != 2) { \
			err = rerr_arg_len_mismatch(2, list_len(args)); \
			list_free(args); \
			RERR_WITH(err, NULL); \
		} \
		a = list_nth(args, 0, NULL); \
		b = list_nth(args, 1, NULL); \
		if (!value_isnumber(a)) { \
			err = rerr_arg_type_mismatch(a, 1, "number"); \
			list_free(args); \
			RERR_WITH(err, NULL); \
		} \
		if (!value_isnumber(b)) { \
			err = rerr_arg_type_mismatch(b, 2, "number"); \
			list_free(args); \
			RERR_WITH(err, NULL); \
		} \
		na = value_getnumber(a, NULL); \
		nb = value_getnumber(b, NULL); \
		nres = na op nb; \
		list_free(args); \
		res = value_number(nres, &err.e.errt); \
		RTRY_WITH(err, NULL); \
		return res; \
	} while (0)

static Value_own
op_add(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	(void)env;
	ARYTH_OP(+);
}
static Value_own
op_sub(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	(void)env;
	ARYTH_OP(-);
}
static Value_own
op_mul(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	(void)env;
	ARYTH_OP(*);
}
static Value_own
op_div(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	(void)env;
	ARYTH_OP(/);
}

static Value_own
prn(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref arg;
	const char own repr;
	static Value_own nil = NULL;

	(void)env;

	if (nil == NULL) {
		nil = value_nil(&err.e.errt);
		if (!rerr_is_ok(err)) {
			list_free(args);
			RERR_WITH(err, NULL);
		}
	}

	if (list_len(args) != 1) {
		err = rerr_arg_len_mismatch(1, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	arg = list_nth(args, 0, NULL);

	repr = pr_str(arg, true, &err.e.errt);
	list_free(args);
	RTRY_WITH(err, NULL);

	fprints(repr, stdout, &err.e.errt);
	free((own_ptr)repr);
	RTRY_WITH(err, NULL);
	fprintc('\n', stdout, &err.e.errt);
	RTRY_WITH(err, NULL);

	return value_copy(nil);
}
static Value_own
list(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	err_t err = ERR_OK;
	Value_own res;

	(void)env;

	res = value_list(args, &err);
	RTRY_FROM_WITH(err, NULL);

	return res;
}
static Value_own
islist(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref arg;
	Value_own res;

	(void)env;

	if (list_len(args) != 1) {
		err = rerr_arg_len_mismatch(1, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	arg = list_nth(args, 0, NULL);

	if (value_islist(arg)) {
		list_free(args);

		res = value_bool(true, &err.e.errt);
		RTRY_WITH(err, NULL);
		return res;
	} else {
		list_free(args);

		res = value_bool(false, &err.e.errt);
		RTRY_WITH(err, NULL);
		return res;
	}
}
static Value_own
isempty(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref arg;
	Value_own res;

	(void)env;

	if (list_len(args) != 1) {
		err = rerr_arg_len_mismatch(1, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	arg = list_nth(args, 0, NULL);
	if (!value_islist(arg)) {
		err = rerr_arg_type_mismatch(arg, 1, "list");
		list_free(args);
		RERR_WITH(err, NULL);
	}

	if (list_len(value_getlist(arg, NULL)) == 0) {
		list_free(args);

		res = value_bool(true, &err.e.errt);
		RTRY_WITH(err, NULL);
		return res;
	} else {
		list_free(args);

		res = value_bool(false, &err.e.errt);
		RTRY_WITH(err, NULL);
		return res;
	}
}
static Value_own
count(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref arg;
	Value_own res;

	(void)env;

	if (list_len(args) != 1) {
		err = rerr_arg_len_mismatch(1, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	arg = list_nth(args, 0, NULL);
	if (value_isnil(arg)) {
		list_free(args);
		res = value_number(0, &err.e.errt);
		RTRY_WITH(err, NULL);
		return res;
	}
	if (!value_islist(arg)) {
		err = rerr_arg_type_mismatch(arg, 1, "list");
		list_free(args);
		RERR_WITH(err, NULL);
	}

	res = value_number(list_len(value_getlist(arg, NULL)), &err.e.errt);
	list_free(args);
	RTRY_WITH(err, NULL);
	return res;
}

#define CMP(cmp) do { \
		rerr_t err = RERR_OK; \
		Value_ref a, b; \
		Value_own res; \
		int na, nb; \
		if (list_len(args) != 2) { \
			err = rerr_arg_len_mismatch(2, list_len(args)); \
			list_free(args); \
			RERR_WITH(err, NULL); \
		} \
		a = list_nth(args, 0, NULL); \
		b = list_nth(args, 1, NULL); \
		if (!value_isnumber(a)) { \
			err = rerr_arg_type_mismatch(a, 1, "number"); \
			list_free(args); \
			RERR_WITH(err, NULL); \
		} \
		if (!value_isnumber(b)) { \
			err = rerr_arg_type_mismatch(b, 2, "number"); \
			list_free(args); \
			RERR_WITH(err, NULL); \
		} \
		na = value_getnumber(a, NULL); \
		nb = value_getnumber(b, NULL); \
		if (na cmp nb) { \
			list_free(args); \
			res = value_bool(true, &err.e.errt); \
			RTRY_WITH(err, NULL); \
			return res; \
		} else { \
			list_free(args); \
			res = value_bool(false, &err.e.errt); \
			RTRY_WITH(err, NULL); \
			return res; \
		} \
	} while (0)

static Value_own
cmp_eq(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref a, b;
	Value_own res;

	(void)env;

	if (list_len(args) != 2) {
		err = rerr_arg_len_mismatch(2, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	a = list_nth(args, 0, NULL);
	b = list_nth(args, 1, NULL);

	if (value_iseq(a, b)) {
		list_free(args);

		res = value_bool(true, &err.e.errt);
		RTRY_WITH(err, NULL);
		return res;
	} else {
		list_free(args);

		res = value_bool(false, &err.e.errt);
		RTRY_WITH(err, NULL);
		return res;
	}
}
static Value_own
cmp_lt(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	(void)env;
	CMP(<);
}
static Value_own
cmp_le(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	(void)env;
	CMP(<=);
}
static Value_own
cmp_gt(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	(void)env;
	CMP(>);
}
static Value_own
cmp_ge(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	(void)env;
	CMP(>=);
}
