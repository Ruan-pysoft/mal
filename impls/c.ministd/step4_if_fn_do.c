#include <ministd.h>
#include <ministd_error.h>
#include <ministd_fmt.h>
#include <ministd_memory.h>

#include "core.h"
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
eval_def(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref name, value;

	if (list_len(args) != 2) {
		err = rerr_arg_len_mismatch(2, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	name = value_copy(list_nth(args, 0, NULL));
	value = value_copy(list_nth(args, 1, NULL));
	list_free(args);
	if (!value_issymbol(name)) {
		err = rerr_arg_type_mismatch(name, 1, "symbol");
		value_free(name);
		value_free(value);
		RERR_WITH(err, NULL);
	}

	value = EVAL(value, env, &err);
	if (!rerr_is_ok(err)) {
		value_free(name);
		RERR_WITH(err, NULL);
	}

	env_set(
		env,
		string_copy(value_getsymbol(name, NULL)),
		value_copy(value),
		&err.e.errt
	);
	value_free(name);
	if (!rerr_is_ok(err)) {
		value_free(value);
		RERR_WITH(err, NULL);
	}

	return value;
}
static Value_own
eval_let(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_own bindings, expr;
	List_ref lbindings;
	MutEnv_own subenv;
	usz i;

	if (list_len(args) != 2) {
		err = rerr_arg_len_mismatch(2, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	bindings = value_copy(list_nth(args, 0, NULL));
	expr = value_copy(list_nth(args, 1, NULL));
	list_free(args);
	if (!value_islist(bindings)) {
		err = rerr_arg_type_mismatch(bindings, 1, "list");
		value_free(bindings);
		value_free(expr);
		RERR_WITH(err, NULL);
	}
	lbindings = value_getlist(bindings, NULL);

	if (list_len(lbindings)%2 != 0) {
		value_free(bindings);
		value_free(expr);
		err = rerr_msg("let* expected an even number of elements in the bind list, got an odd number");
		RERR_WITH(err, NULL);
	}

	subenv = env_new(env, &err.e.errt);
	if (!rerr_is_ok(err)) {
		value_free(bindings);
		value_free(expr);
		ERR_WITH(err, NULL);
	}

	for (i = 0; i < list_len(lbindings)/2; ++i) {
		Value_ref key, val;
		Value_own evald;

		key = list_nth(lbindings, i*2, NULL);
		val = list_nth(lbindings, i*2 + 1, NULL);

		if (!value_issymbol(key)) {
			err = rerr_arg_type_mismatch(key, i*2+1, "symbol");
			value_free(bindings);
			value_free(expr);
			env_free(subenv);
			ERR_WITH(err, NULL);
		}

		evald = EVAL(value_copy(val), subenv, &err);
		if (!rerr_is_ok(err)) {
			value_free(bindings);
			value_free(expr);
			env_free(subenv);
			ERR_WITH(err, NULL);
		}

		env_set(
			subenv,
			string_copy(value_getsymbol(key, NULL)),
			evald,
			&err.e.errt
		);
		if (!rerr_is_ok(err)) {
			value_free(bindings);
			value_free(expr);
			env_free(subenv);
			ERR_WITH(err, NULL);
		}
	}

	value_free(bindings);

	return EVAL(expr, subenv, err_out);
}
static Value_own
eval_do(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	List_iter at;
	Value_own last = NULL;

	if (list_len(args) == 0) {
		err = rerr_arg_vararg_mismatch(1, 0);
		list_free(args);
		RERR_WITH(err, NULL);
	}

	for (at = list_iter(args); !list_isend(at); at = list_next(at)) {
		if (last != NULL) value_free(last);
		last = EVAL(value_copy(list_at(at, NULL)), env, &err);

		if (!rerr_is_ok(err)) {
			list_free(args);
			RERR_WITH(err, NULL);
		}
	}

	list_free(args);

	return last;
}
static Value_own
eval_if(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_own cond;
	Value_own res;

	if (list_len(args) < 2) {
		err = rerr_arg_vararg_mismatch(2, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}
	if (list_len(args) > 3) {
		err = rerr_arg_len_mismatch(3, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	cond = EVAL(value_copy(list_nth(args, 0, NULL)), env, &err);
	if (!rerr_is_ok(err)) {
		list_free(args);
		RERR_WITH(err, NULL);
	}

	if (!value_isnil(cond) && (!value_isbool(cond)
			|| value_getbool(cond, NULL) != false)) {
		res = EVAL(value_copy(list_nth(args, 1, NULL)), env, &err);
		list_free(args);
		RTRY_WITH(err, NULL);

		return res;
	} else if (list_len(args) == 3) {
		res = EVAL(value_copy(list_nth(args, 2, NULL)), env, &err);
		list_free(args);
		RTRY_WITH(err, NULL);

		return res;
	} else {
		list_free(args);
		res = value_nil(&err.e.errt);
		RTRY_WITH(err, NULL);

		return res;
	}
}
static Value_own
eval_lambda(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	List_own params;
	Value_own body;
	String_own own param_names;
	usz n_params;
	List_iter at;
	usz i;
	Fn_own fn;
	Value_own res;

	if (list_len(args) != 2) {
		err = rerr_arg_len_mismatch(2, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}
	if (!value_islist(list_nth(args, 0, NULL))) {
		err = rerr_arg_type_mismatch(
			list_nth(args, 0, NULL), 
			1, 
			"list"
		);
		list_free(args);
		RERR_WITH(err, NULL);
	}
	params = list_copy(value_getlist(list_nth(args, 0, NULL), NULL));
	body = value_copy(list_nth(args, 1, NULL));
	list_free(args);

	n_params = list_len(params);
	param_names = nalloc(sizeof(*param_names), n_params, &err.e.errt);
	if (!rerr_is_ok(err)) {
		list_free(params);
		value_free(body);
		RERR_WITH(err, NULL);
	}

	for (at = list_iter(params), i = 0; !list_isend(at);
			at = list_next(at), ++i) {
		if (!value_issymbol(list_at(at, NULL))) {
			usz j;
			err = rerr_arg_type_mismatch(
				list_at(at, NULL),
				i+1,
				"symbol"
			);
			list_free(params);
			value_free(body);

			for (j = 0; j < i; ++j) {
				string_free(param_names[j]);
			}
			free(param_names);

			ERR_WITH(err, NULL);
		}

		param_names[i] = string_copy(
			value_getsymbol(list_at(at, NULL), NULL)
		);
	}

	list_free(params);

	fn = fn_new(
		body, param_names, n_params, false, env_copy(env),
		&err.e.errt
	);
	RTRY_WITH(err, NULL);
	res = value_fn(fn, &err.e.errt);
	RTRY_WITH(err, NULL);

	return res;
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
		rerr_deinit(err);
		err = RERR_OK;
		debug = NULL;
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

		if (value_issymbol(head)) {
			String_ref fnname = value_getsymbol(head, NULL);
			if (string_match(fnname, "def!") == C_EQ) {
				value_free(head);

				return eval_def(tail, env, err_out);
			} else if (string_match(fnname, "let*") == C_EQ) {
				value_free(head);

				return eval_let(tail, env, err_out);
			} else if (string_match(fnname, "do") == C_EQ) {
				value_free(head);

				return eval_do(tail, env, err_out);
			} else if (string_match(fnname, "if") == C_EQ) {
				value_free(head);

				return eval_if(tail, env, err_out);
			} else if (string_match(fnname, "fn*") == C_EQ) {
				value_free(head);

				return eval_lambda(tail, env, err_out);
			}
		}

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

int
main(void)
{
	const char own out;
	MutEnv_own repl_env = env_new(NULL, NULL);

	/*env_set(
		repl_env,
		string_new("DEBUG-EVAL", NULL),
		value_bool(true, NULL),
		NULL
	);*/
	core_load(repl_env, NULL);

	free((own_ptr)rep("(def! not (fn* (a) (if a false true)))", repl_env));

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
			free((own_ptr)out);
		}
	}

	return 0;
}
