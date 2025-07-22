#include <ministd.h>
#include <ministd_error.h>
#include <ministd_fmt.h>
#include <ministd_memory.h>

#include "env.h"
#include "error.h"
#include "printer.h"
#include "reader.h"
#include "types.h"
#include "values.h"

static Value_own EVAL(Value_own val, MutEnv_ref env, rerr_t ref err_out);
Value_own (ref fn_EVAL)(Value_own expr, MutEnv_ref env, rerr_t ref err_out)
	= EVAL;

/* `read` is a function in `ministd_fmt.h`...
 * I should either add optional prefixes to all ministd functions,
 * or I should just change read/write/close/etc
 * to file_read/file_write/file_close etc...
 */
static Value_own
READ(const char ref line)
{
	perr_t err = PERR_OK;
	Value_own val;

	val = read_str(line, &err);
	if (!perr_is_ok(err)) {
		perr_display(&err, stderr, NULL);
		fprintc('\n', stderr, NULL);
		perr_deinit(err);
		return NULL;
	}

	return val;
}

static List_own
eval_each(List_ref list, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	List_iter at;
	struct List_struct own res;

	res = (struct List_struct own)list_new(NULL, 0, &err.e.errt);
	RTRY_WITH(err, NULL);

	for (at = list_iter(list); !list_isend(at); at = list_next(at)) {
		Value_own val;

		val = EVAL(value_copy(list_at(at, NULL)), env, &err);
		if (!rerr_is_ok(err)) {
			list_free(res);
			RERR_WITH(err, NULL);
		}

		res = (struct List_struct own)_list_append(
			res,
			val,
			&err.e.errt
		);
		RTRY_WITH(err, NULL);
	}

	return res;
}
static Value_own
eval_fn(Value_own head, List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_own fn;
	List_own evald_args;

	fn = EVAL(head, env, &err);
	if (!rerr_is_ok(err)) {
		list_free(args);
		RERR_WITH(err, NULL);
	}
	evald_args = eval_each(args, env, &err);
	if (!rerr_is_ok(err)) {
		list_free(args);
		value_free(fn);
		RERR_WITH(err, NULL);
	}

	switch (value_type(fn)) {
		case VT_SYM: {
			err = rerr_uncallable("symbol");
			ERR_WITH(err, NULL);
		break; }
		case VT_NUM: {
			err = rerr_uncallable("number");
			ERR_WITH(err, NULL);
		break; }
		case VT_LST: {
			err = rerr_uncallable("list");
			ERR_WITH(err, NULL);
		break; }
		case VT_STR: {
			err = rerr_uncallable("string");
			ERR_WITH(err, NULL);
		break; }
		case VT_NIL: {
			err = rerr_uncallable("nil");
			ERR_WITH(err, NULL);
		break; }
		case VT_BOO: {
			err = rerr_uncallable("bool");
			ERR_WITH(err, NULL);
		break; }
		case VT_KEY: {
			err = rerr_uncallable("keyword");
			ERR_WITH(err, NULL);
		break; }
		case VT_VEC: {
			err = rerr_uncallable("vector");
			ERR_WITH(err, NULL);
		break; }
		case VT_MAP: {
			err = rerr_uncallable("hash-map");
			ERR_WITH(err, NULL);
		break; }
		case VT_FNC: {
			Value_own res;
			res = fn_call(value_getfn(fn, NULL), evald_args, &err);
			value_free(fn);
			RTRY_WITH(err, NULL);
			return res;
		break; }
	}

	return NULL;
}

static Value_own
EVAL(Value_own val, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	static String_own debug_key = NULL;
	Value_own debug;

	if (debug_key == NULL) {
		debug_key = string_new("DEBUG-EVAL", NULL);
	}
	debug = env_get(env, debug_key, &err.e.errt);
	if (!rerr_is_ok(err)) {
		value_free(val);
		ERR_WITH(err, NULL);
	}
	if (debug != NULL && !value_isnil(debug) && (!value_isbool(debug)
			|| value_getbool(debug, NULL))) {
		const char own repr = pr_str(val, 1, &err.e.errt);
		if (!rerr_is_ok(err)) {
			value_free(val);
			ERR_WITH(err, NULL);
		}

		fprints("EVAL: ", stderr, &err.e.errt);
		if (!rerr_is_ok(err)) {
			value_free(val);
			ERR_WITH(err, NULL);
		}
		fprints(repr, stderr, &err.e.errt);
		if (!rerr_is_ok(err)) {
			value_free(val);
			ERR_WITH(err, NULL);
		}
		fprintc('\n', stderr, &err.e.errt);
		if (!rerr_is_ok(err)) {
			value_free(val);
			ERR_WITH(err, NULL);
		}
	}

	if (value_islist(val)) {
		List_ref list = value_getlist(val, NULL);
		Value_own head;
		List_own tail;

		if (list_len(list) == 0) {
			return val;
		}
		head = value_copy(list_nth(list, 0, &err.e.errt));
		if (!rerr_is_ok(err)) {
			value_free(val);
			ERR_WITH(err, NULL);
		}
		tail = list_tail(list, 1, &err.e.errt);
		if (!rerr_is_ok(err)) {
			value_free(val);
			value_free(head);
			ERR_WITH(err, NULL);
		}

		value_free(val);
		return eval_fn(head, tail, env, err_out);
	} else if (value_issymbol(val)) {
		Value_ref res = env_get(
			env,
			value_getsymbol(val, NULL),
			&err.e.errt
		);
		if (err.is_errt && err.e.errt == ERR_INVAL) {
			/* symbol not in env */
			err = rerr_undefined_name(value_getsymbol(val, NULL));
			value_free(val);
			ERR_WITH(err, NULL);
		} else if (!rerr_is_ok(err)) {
			value_free(val);
			ERR_WITH(err, NULL);
		}
		value_free(val);

		return value_copy(res);
	} else {
		return val;
	}
}

static const char own
PRINT(Value_own value)
{
	err_t err = ERR_OK;
	const char own str;

	str = pr_str(value, true, &err);
	value_free(value);
	if (err != ERR_OK) {
		perror(err, "Encountered error while printing value");
		return NULL;
	}

	return str;
}

static const char own
rep(const char ref line, MutEnv_ref env)
{
	Value_own read_res;
	Value_own eval_res;
	const char own print_res;

	read_res = READ(line);
	if (read_res != NULL) {
		rerr_t err = RERR_OK;

		eval_res = EVAL(read_res, env, &err);
		if (!rerr_is_ok(err)) {
			rerr_display(&err, stderr, NULL);
			fprintc('\n', stderr, NULL);
			rerr_deinit(err);

			return NULL;
		} else if (eval_res == NULL) {
			fprints("ERROR: eval returned NULL for some reason??\n", stderr, NULL);
			return NULL;
		} else {
			print_res = PRINT(eval_res);
			return print_res;
		}
	} else {
		return NULL;
	}
}

#define LINECAP (16 * 1024)
static char linebuf[LINECAP];

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
			err = rerr_arg_type_mismatch(a, 2, "number"); \
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

Value_own
op_add(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	(void)env;
	ARYTH_OP(+);
}
Value_own
op_sub(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	(void)env;
	ARYTH_OP(-);
}
Value_own
op_mul(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	(void)env;
	ARYTH_OP(*);
}
Value_own
op_div(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	(void)env;
	ARYTH_OP(/);
}

int
main(void)
{
	const char own out;
	MutEnv_own repl_env = env_new(NULL, NULL);

	env_set(
		repl_env,
		string_new("DEBUG-EVAL", NULL),
		value_bool(true, NULL),
		NULL
	);
	env_set(
		repl_env,
		string_new("+", NULL),
		value_fn(fn_builtin(op_add, NULL, NULL), NULL),
		NULL
	);
	env_set(
		repl_env,
		string_new("-", NULL),
		value_fn(fn_builtin(op_sub, NULL, NULL), NULL),
		NULL
	);
	env_set(
		repl_env,
		string_new("*", NULL),
		value_fn(fn_builtin(op_mul, NULL, NULL), NULL),
		NULL
	);
	env_set(
		repl_env,
		string_new("/", NULL),
		value_fn(fn_builtin(op_div, NULL, NULL), NULL),
		NULL
	);

	for (;;) {
		prints("user> ", NULL);
		if (!getline(linebuf, LINECAP, NULL)) {
			/* line longer than 16K */
			return 1;
		}
		if (*linebuf == 0) {
			/* hit eof */
			return 0;
		}
		out = rep(linebuf, repl_env);
		if (out != NULL) {
			prints(out, NULL);
			printc('\n', NULL);
			free((ptr)out);
		}
	}

	return 0;
}
