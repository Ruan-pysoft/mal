#include "core.h"

#include <ministd_fmt.h>
#include <ministd_memory.h>
#include <ministd_string.h>

#include "env.h"
#include "error.h"
#include "printer.h"
#include "reader.h"
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

static Value_own list(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own islist(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own isempty(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own count(List_own args, MutEnv_ref env, rerr_t ref err_out);

static Value_own cmp_eq(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own cmp_lt(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own cmp_le(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own cmp_gt(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own cmp_ge(List_own args, MutEnv_ref env, rerr_t ref err_out);

static Value_own core_prstr(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own str(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own prn(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own println(List_own args, MutEnv_ref env, rerr_t ref err_out);

static Value_own read_string(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own slurp(List_own args, MutEnv_ref env, rerr_t ref err_out);

static Value_own atom(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own isatom(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own deref(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own reset(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own swap(List_own args, MutEnv_ref env, rerr_t ref err_out);

static Value_own cons(List_own args, MutEnv_ref env, rerr_t ref err_out);
static Value_own concat(List_own args, MutEnv_ref env, rerr_t ref err_out);

const char ref core_names[CORE_LEN] = {
	"+", "-", "*", "/",
	"list", "list?", "empty?", "count", "=",
	"<", "<=", ">", ">=",
	"pr-str", "str", "prn", "println",
	"read-string", "slurp",
	"atom", "atom?", "deref", "reset!", "swap!",
	"cons", "concat",
};
builtin_fn core_funcs[CORE_LEN] = {
	op_add, op_sub, op_mul, op_div,
	list, islist, isempty, count, cmp_eq,
	cmp_lt, cmp_le, cmp_gt, cmp_ge,
	core_prstr, str, prn, println,
	read_string, slurp,
	atom, isatom, deref, reset, swap,
	cons, concat,
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

static Value_own
core_prstr(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	String own str;
	FILE own file;
	Value_ref arg;
	const char own repr;
	List_iter at;
	bool first = true;
	String_own str_res;
	Value_own res;

	(void)env;

	str = s_new(&err.e.errt);
	RTRY_WITH(err, NULL);
	file = (FILE own)sf_open(str, &err.e.errt);
	if (!rerr_is_ok(err)) {
		s_free(str);
		RERR_WITH(err, NULL);
	}


	for (at = list_iter(args); !list_isend(at); at = list_next(at)) {
		arg = list_at(at, NULL);

		if (!first) {
			fprintc(' ', file, &err.e.errt);
			if (!rerr_is_ok(err)) {
				list_free(args);
				close(file, NULL);
				free(file);
				s_free(str);
				RERR_WITH(err, NULL);
			}
		}
		first = false;

		repr = pr_str(arg, true, &err.e.errt);
		if (!rerr_is_ok(err)) {
			list_free(args);
			close(file, NULL);
			free(file);
			s_free(str);
			RERR_WITH(err, NULL);
		}

		fprints(repr, file, &err.e.errt);
		free((own_ptr)repr);
		if (!rerr_is_ok(err)) {
			list_free(args);
			close(file, NULL);
			free(file);
			s_free(str);
			RERR_WITH(err, NULL);
		}
	}

	list_free(args);

	close(file, &err.e.errt);
	free(file);
	if (!rerr_is_ok(err)) {
		s_free(str);
		RERR_WITH(err, NULL);
	}

	str_res = string_new(s_to_c(str), &err.e.errt);
	s_free(str);
	RTRY_WITH(err, NULL);

	res = value_string(str_res, &err.e.errt);
	RTRY_WITH(err, NULL);

	return res;
}
static Value_own
str(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	String own str;
	FILE own file;
	Value_ref arg;
	const char own repr;
	List_iter at;
	String_own str_res;
	Value_own res;

	(void)env;

	str = s_new(&err.e.errt);
	if (!rerr_is_ok(err)) {
		list_free(args);
		RERR_WITH(err, NULL);
	}
	file = (FILE own)sf_open(str, &err.e.errt);
	if (!rerr_is_ok(err)) {
		list_free(args);
		s_free(str);
		RERR_WITH(err, NULL);
	}

	for (at = list_iter(args); !list_isend(at); at = list_next(at)) {
		arg = list_at(at, NULL);

		repr = pr_str(arg, false, &err.e.errt);
		if (!rerr_is_ok(err)) {
			list_free(args);
			close(file, NULL);
			free(file);
			s_free(str);
			RERR_WITH(err, NULL);
		}

		fprints(repr, file, &err.e.errt);
		free((own_ptr)repr);
		if (!rerr_is_ok(err)) {
			list_free(args);
			close(file, NULL);
			free(file);
			s_free(str);
			RERR_WITH(err, NULL);
		}
	}

	list_free(args);

	close(file, &err.e.errt);
	free(file);
	if (!rerr_is_ok(err)) {
		s_free(str);
		RERR_WITH(err, NULL);
	}

	str_res = string_new(s_to_c(str), &err.e.errt);
	s_free(str);
	RTRY_WITH(err, NULL);

	res = value_string(str_res, &err.e.errt);
	RTRY_WITH(err, NULL);

	return res;
}
static Value_own
prn(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref arg;
	List_iter at;
	const char own repr;
	static Value_own nil = NULL;
	bool first = true;

	(void)env;

	if (nil == NULL) {
		nil = value_nil(&err.e.errt);
		if (!rerr_is_ok(err)) {
			list_free(args);
			RERR_WITH(err, NULL);
		}
	}

	for (at = list_iter(args); !list_isend(at); at = list_next(at)) {
		arg = list_at(at, NULL);

		if (!first) {
			fprintc(' ', stdout, &err.e.errt);
			if (!rerr_is_ok(err)) {
				list_free(args);
				RERR_WITH(err, NULL);
			}
		}
		first = false;

		repr = pr_str(arg, true, &err.e.errt);
		if (!rerr_is_ok(err)) {
			list_free(args);
			RERR_WITH(err, NULL);
		}

		fprints(repr, stdout, &err.e.errt);
		free((own_ptr)repr);
		if (!rerr_is_ok(err)) {
			list_free(args);
			RERR_WITH(err, NULL);
		}
	}

	list_free(args);

	fprintc('\n', stdout, &err.e.errt);
	RTRY_WITH(err, NULL);

	return value_copy(nil);
}
static Value_own
println(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref arg;
	List_iter at;
	const char own repr;
	static Value_own nil = NULL;
	bool first = true;

	(void)env;

	if (nil == NULL) {
		nil = value_nil(&err.e.errt);
		if (!rerr_is_ok(err)) {
			list_free(args);
			RERR_WITH(err, NULL);
		}
	}

	for (at = list_iter(args); !list_isend(at); at = list_next(at)) {
		arg = list_at(at, NULL);

		if (!first) {
			fprintc(' ', stdout, &err.e.errt);
			if (!rerr_is_ok(err)) {
				list_free(args);
				RERR_WITH(err, NULL);
			}
		}
		first = false;

		repr = pr_str(arg, false, &err.e.errt);
		if (!rerr_is_ok(err)) {
			list_free(args);
			RERR_WITH(err, NULL);
		}

		fprints(repr, stdout, &err.e.errt);
		free((own_ptr)repr);
		if (!rerr_is_ok(err)) {
			list_free(args);
			RERR_WITH(err, NULL);
		}
	}

	list_free(args);

	fprintc('\n', stdout, &err.e.errt);
	RTRY_WITH(err, NULL);

	return value_copy(nil);
}

static Value_own
read_string(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	perr_t perr = PERR_OK;
	Value_ref arg;
	String_ref argstr;
	Value_own res;

	(void)env;

	if (list_len(args) != 1) {
		err = rerr_arg_len_mismatch(1, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	arg = list_nth(args, 0, NULL);
	if (!value_isstring(arg)) {
		err = rerr_arg_type_mismatch(arg, 1, "string");
		list_free(args);
		RERR_WITH(err, NULL);
	}

	argstr = value_getstring(arg, NULL);

	res = read_str(_string_cstr(argstr), &perr);
	list_free(args);
	if (!perr_is_ok(perr)) {
		err = rerr_perr(perr);
		RERR_WITH(err, NULL);
	}

	return res;
}
static char slurp_buf[4096];
static Value_own
slurp(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref arg;
	String_ref filename;
	FILE own file;
	usz bytes_read;
	String own str;
	FILE own str_file;
	String_own str_res;
	Value_own res;

	(void)env;

	if (list_len(args) != 1) {
		err = rerr_arg_len_mismatch(1, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	arg = list_nth(args, 0, NULL);
	if (!value_isstring(arg)) {
		err = rerr_arg_type_mismatch(arg, 1, "string");
		list_free(args);
		RERR_WITH(err, NULL);
	}

	str = s_new(&err.e.errt);
	if (!rerr_is_ok(err)) {
		list_free(args);
		RERR_WITH(err, NULL);
	}
	str_file = (FILE own)sf_open(str, &err.e.errt);
	if (!rerr_is_ok(err)) {
		list_free(args);
		s_free(str);
		RERR_WITH(err, NULL);
	}

	filename = value_getstring(arg, NULL);
	file = open(_string_cstr(filename), &err.e.errt);
	list_free(args);
	if (!rerr_is_ok(err)) {
		close(str_file, NULL);
		free(str_file);
		s_free(str);
		RERR_WITH(err, NULL);
	}

	do {
		bytes_read = read(file, slurp_buf, 4096, &err.e.errt);
		if (!rerr_is_ok(err)) {
			close(str_file, NULL);
			free(str_file);
			s_free(str);
			close(file, NULL);
			free(file);
			RERR_WITH(err, NULL);
		}
		write(str_file, slurp_buf, bytes_read, &err.e.errt);
		if (!rerr_is_ok(err)) {
			close(str_file, NULL);
			free(str_file);
			s_free(str);
			close(file, NULL);
			free(file);
			RERR_WITH(err, NULL);
		}
	} while (bytes_read != 0);

	close(file, &err.e.errt);
	free(file);
	if (!rerr_is_ok(err)) {
		close(str_file, NULL);
		free(str_file);
		s_free(str);
		RERR_WITH(err, NULL);
	}
	close(str_file, &err.e.errt);
	free(str_file);
	if (!rerr_is_ok(err)) {
		s_free(str);
		RERR_WITH(err, NULL);
	}

	str_res = string_new(s_to_c(str), &err.e.errt);
	s_free(str);
	RTRY_WITH(err, NULL);

	res = value_string(str_res, &err.e.errt);
	RTRY_WITH(err, NULL);

	return res;
}

static Value_own
atom(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	MutAtom_own atom;
	Value_own res;

	(void)env;

	if (list_len(args) != 1) {
		err = rerr_arg_len_mismatch(1, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	atom = atom_new(list_nth(args, 0, NULL), &err.e.errt);
	list_free(args);
	RTRY_WITH(err, NULL);
	res = value_atom(atom, &err.e.errt);
	RTRY_WITH(err, NULL);

	return res;
}
static Value_own
isatom(List_own args, MutEnv_ref env, rerr_t ref err_out)
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

	if (value_isatom(arg)) {
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
deref(List_own args, MutEnv_ref env, rerr_t ref err_out)
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
	if (!value_isatom(arg)) {
		err = rerr_arg_type_mismatch(arg, 1, "atom");
		list_free(args);
		RERR_WITH(err, NULL);
	}

	res = value_copy(atom_get(value_getatom(arg, NULL)));
	list_free(args);

	return res;
}
static Value_own
reset(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref atom;
	Value_ref val;
	Value_own res;

	(void)env;

	if (list_len(args) != 2) {
		err = rerr_arg_len_mismatch(2, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	atom = list_nth(args, 0, NULL);
	val = list_nth(args, 1, NULL);
	if (!value_isatom(atom)) {
		err = rerr_arg_type_mismatch(atom, 1, "atom");
		list_free(args);
		RERR_WITH(err, NULL);
	}

	atom_set(value_getatom(atom, NULL), val);
	res = value_copy(val);
	list_free(args);

	return res;
}
static Value_own
swap(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref atom;
	Value_ref atom_val;
	Value_ref fn;
	List_own fn_args;
	Value_own res;

	(void)env;

	if (list_len(args) < 2) {
		err = rerr_arg_vararg_mismatch(2, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	atom = list_nth(args, 0, NULL);
	if (!value_isatom(atom)) {
		err = rerr_arg_type_mismatch(atom, 1, "atom");
		list_free(args);
		RERR_WITH(err, NULL);
	}
	fn = list_nth(args, 1, NULL);
	if (!value_isfn(fn)) {
		err = rerr_arg_type_mismatch(fn, 2, "function");
		list_free(args);
		RERR_WITH(err, NULL);
	}

	atom_val = atom_get(value_getatom(atom, NULL));
	fn_args = list_new(&atom_val, 1, &err.e.errt);
	if (!rerr_is_ok(err)) {
		list_free(fn_args);
		list_free(args);
		RERR_WITH(err, NULL);
	}

	if (list_len(args) > 2) {
		fn_args = _list_join(
			(struct List_struct own)fn_args,
			args, 2, &err.e.errt
		);
		if (!rerr_is_ok(err)) {
			list_free(args);
			RERR_WITH(err, NULL);
		}
	}

	res = fn_call(value_getfn(fn, NULL), fn_args, &err);
	if (!rerr_is_ok(err)) {
		list_free(args);
		RERR_WITH(err, NULL);
	}
	atom_set(value_getatom(atom, NULL), res);

	list_free(args);

	return res;
}

static Value_own
cons(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref fst;
	Value_ref rest;
	List_own list;
	Value_own res;

	(void)env;

	if (list_len(args) < 2) {
		err = rerr_arg_vararg_mismatch(2, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	fst = list_nth(args, 0, NULL);
	rest = list_nth(args, 1, NULL);
	if (!value_islist(rest)) {
		err = rerr_arg_type_mismatch(rest, 2, "list");
		list_free(args);
		RERR_WITH(err, NULL);
	}

	list = list_new(&fst, 1, &err.e.errt);
	if (!rerr_is_ok(err)) {
		list_free(args);
		RERR_WITH(err, NULL);
	}

	list = _list_join(
		(struct List_struct ref)list,
		value_getlist(rest, NULL),
		0, &err.e.errt
	);
	list_free(args);
	RTRY_WITH(err, NULL);

	res = value_list(list, &err.e.errt);
	RTRY_WITH(err, NULL);

	return res;
}
static Value_own
concat(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	List_iter at;
	usz arg_num = 1;
	Value_ref arg;
	List_own list;
	Value_own res;

	(void)env;

	list = list_new(NULL, 0, &err.e.errt);
	if (!rerr_is_ok(err)) {
		list_free(args);
		RERR_WITH(err, NULL);
	}

	for (at = list_iter(args); !list_isend(at); at = list_next(at)) {
		arg = list_at(at, NULL);
		if (!value_islist(arg)) {
			err = rerr_arg_type_mismatch(arg, arg_num, "list");
			list_free(args);
			RERR_WITH(err, NULL);
		}
		++arg_num;

		list = _list_join(
			(struct List_struct ref)list,
			value_getlist(arg, NULL),
			0, &err.e.errt
		);
		if (!value_islist(arg)) {
			list_free(args);
			RERR_WITH(err, NULL);
		}
	}

	list_free(args);

	res = value_list(list, &err.e.errt);
	RTRY_WITH(err, NULL);

	return res;
}
