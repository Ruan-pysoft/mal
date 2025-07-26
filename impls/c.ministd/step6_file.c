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

static Value_own EVAL(Value_own val, MutEnv_own env, rerr_t ref err_out);
static Value_own
fn_EVAL_wrapper(Value_own val, MutEnv_own env, rerr_t ref err_out)
{
	return EVAL(val, env_copy(env), err_out);
}
/* using `fn_call` again, to support swap! */
Value_own (ref fn_EVAL)(Value_own expr, MutEnv_ref env, rerr_t ref err_out)
	= fn_EVAL_wrapper;

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

struct tailcall_info {
	Value_own val;
	MutEnv_own env;
	bool tailcall;
};
static struct tailcall_info TAILCALL_NULL = { NULL, NULL, false };

static Value_own EVAL(Value_own val, MutEnv_own env, rerr_t ref err_out);
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

		val = EVAL(value_copy(list_at(at, NULL)), env_copy(env), &err);
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
static struct tailcall_info
call_fn(Fn_ref fn, List_own args, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Env_own outer;
	MutEnv_own env;
	struct tailcall_info res;

	outer = fn_closure(fn) == NULL ? NULL : env_copy_const(fn_closure(fn));
	env = env_new(outer, &err.e.errt);
	RTRY_WITH(err, TAILCALL_NULL);

	if (fn_isbuiltin(fn)) {
		Value_own val;
		val = fn_getbuiltin(fn, NULL)(args, env, &err);
		if (!rerr_is_ok(err)) {
			env_free(env);
			RERR_WITH(err, TAILCALL_NULL);
		}

		res.env = NULL;
		res.val = val;
		res.tailcall = false;

		env_free(env);

		return res;
	} else {
		env_bind(
			env,
			fn_getargs(fn, NULL),
			fn_getnargs(fn, NULL),
			fn_isvariadic(fn, NULL),
			args,
			&err
		);
		if (!rerr_is_ok(err)) {
			env_free(env);
			RERR_WITH(err, TAILCALL_NULL);
		}

		res.env = env;
		res.val = value_copy(fn_getbody(fn, NULL));
		res.tailcall = true;

		return res;
	}
}
static struct tailcall_info
eval_fn(Value_own head, List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_own fn;
	List_own evald_args;

	fn = EVAL(head, env_copy(env), &err);
	if (!rerr_is_ok(err)) {
		list_free(args);
		RERR_WITH(err, TAILCALL_NULL);
	}
	evald_args = eval_each(args, env, &err);
	list_free(args);
	if (!rerr_is_ok(err)) {
		value_free(fn);
		RERR_WITH(err, TAILCALL_NULL);
	}

	switch (value_type(fn)) {
		case VT_SYM: {
			err = rerr_uncallable("symbol");
			ERR_WITH(err, TAILCALL_NULL);
		break; }
		case VT_NUM: {
			err = rerr_uncallable("number");
			ERR_WITH(err, TAILCALL_NULL);
		break; }
		case VT_LST: {
			err = rerr_uncallable("list");
			ERR_WITH(err, TAILCALL_NULL);
		break; }
		case VT_STR: {
			err = rerr_uncallable("string");
			ERR_WITH(err, TAILCALL_NULL);
		break; }
		case VT_NIL: {
			err = rerr_uncallable("nil");
			ERR_WITH(err, TAILCALL_NULL);
		break; }
		case VT_BOO: {
			err = rerr_uncallable("bool");
			ERR_WITH(err, TAILCALL_NULL);
		break; }
		case VT_KEY: {
			err = rerr_uncallable("keyword");
			ERR_WITH(err, TAILCALL_NULL);
		break; }
		case VT_VEC: {
			err = rerr_uncallable("vector");
			ERR_WITH(err, TAILCALL_NULL);
		break; }
		case VT_MAP: {
			err = rerr_uncallable("hash-map");
			ERR_WITH(err, TAILCALL_NULL);
		break; }
		case VT_FNC: {
			struct tailcall_info res;
			res = call_fn(value_getfn(fn, NULL), evald_args, &err);
			value_free(fn);
			RTRY_WITH(err, TAILCALL_NULL);
			return res;
		break; }
		case VT_ATM: {
			err = rerr_uncallable("atom");
			ERR_WITH(err, TAILCALL_NULL);
		break; }
	}

	return TAILCALL_NULL;
}

static Value_own
eval_def(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_own name, value;

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

	value = EVAL(value, env_copy(env), &err);
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
static struct tailcall_info
eval_let(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_own bindings, expr;
	List_ref lbindings;
	MutEnv_own subenv;
	usz i;
	struct tailcall_info res;
	res.tailcall = true;

	if (list_len(args) != 2) {
		err = rerr_arg_len_mismatch(2, list_len(args));
		list_free(args);
		RERR_WITH(err, TAILCALL_NULL);
	}

	bindings = value_copy(list_nth(args, 0, NULL));
	expr = value_copy(list_nth(args, 1, NULL));
	list_free(args);
	if (!value_islist(bindings)) {
		err = rerr_arg_type_mismatch(bindings, 1, "list");
		value_free(bindings);
		value_free(expr);
		RERR_WITH(err, TAILCALL_NULL);
	}
	lbindings = value_getlist(bindings, NULL);

	if (list_len(lbindings)%2 != 0) {
		value_free(bindings);
		value_free(expr);
		err = rerr_msg("let* expected an even number of elements in the bind list, got an odd number");
		RERR_WITH(err, TAILCALL_NULL);
	}

	subenv = env_new(env_copy(env), &err.e.errt);
	if (!rerr_is_ok(err)) {
		value_free(bindings);
		value_free(expr);
		ERR_WITH(err, TAILCALL_NULL);
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
			ERR_WITH(err, TAILCALL_NULL);
		}

		evald = EVAL(value_copy(val), env_copy(subenv), &err);
		if (!rerr_is_ok(err)) {
			value_free(bindings);
			value_free(expr);
			env_free(subenv);
			ERR_WITH(err, TAILCALL_NULL);
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
			ERR_WITH(err, TAILCALL_NULL);
		}
	}

	value_free(bindings);

	res.env = subenv;
	res.val = expr;
	return res;
}
static struct tailcall_info
eval_do(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	List_iter at;
	Value_own last = NULL;
	struct tailcall_info res;
	res.env = NULL;
	res.tailcall = true;

	if (list_len(args) == 0) {
		err = rerr_arg_vararg_mismatch(1, 0);
		list_free(args);
		RERR_WITH(err, TAILCALL_NULL);
	}

	for (at = list_iter(args); !list_isend(at); at = list_next(at)) {
		if (last != NULL) {
			Value_own val;
			val = EVAL(last, env_copy(env), &err);
			if (!rerr_is_ok(err)) {
				list_free(args);
				RERR_WITH(err, TAILCALL_NULL);
			}
			value_free(val);
		}
		last = value_copy(list_at(at, NULL));
	}

	list_free(args);

	res.val = last;

	return res;
}
static struct tailcall_info
eval_if(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_own cond;
	struct tailcall_info res;
	res.env = NULL;
	res.tailcall = true;

	if (list_len(args) < 2) {
		err = rerr_arg_vararg_mismatch(2, list_len(args));
		list_free(args);
		RERR_WITH(err, TAILCALL_NULL);
	}
	if (list_len(args) > 3) {
		err = rerr_arg_len_mismatch(3, list_len(args));
		list_free(args);
		RERR_WITH(err, TAILCALL_NULL);
	}

	cond = EVAL(value_copy(list_nth(args, 0, NULL)), env_copy(env), &err);
	if (!rerr_is_ok(err)) {
		list_free(args);
		RERR_WITH(err, TAILCALL_NULL);
	}

	if (!value_isnil(cond) && (!value_isbool(cond)
			|| value_getbool(cond, NULL) != false)) {
		value_free(cond);
		res.val = value_copy(list_nth(args, 1, NULL));
		list_free(args);

		return res;
	} else if (list_len(args) == 3) {
		value_free(cond);
		res.val = value_copy(list_nth(args, 2, NULL));
		list_free(args);

		return res;
	} else {
		value_free(cond);
		list_free(args);
		res.val = value_nil(&err.e.errt);
		RTRY_WITH(err, TAILCALL_NULL);

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
EVAL(Value_own val, MutEnv_own env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	static String_own debug_key = NULL;
	Value_own debug;
	struct tailcall_info tci;
	Value_own res;

	if (debug_key == NULL) {
		debug_key = string_new("DEBUG-EVAL", NULL);
	}

	for (;;) {
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
				env_free(env);
				ERR_WITH(err, NULL);
			}

			fprints("EVAL: ", stderr, &err.e.errt);
			if (!rerr_is_ok(err)) {
				value_free(val);
				env_free(env);
				ERR_WITH(err, NULL);
			}
			fprints(repr, stderr, &err.e.errt);
			if (!rerr_is_ok(err)) {
				value_free(val);
				env_free(env);
				ERR_WITH(err, NULL);
			}
			fprintc('\n', stderr, &err.e.errt);
			if (!rerr_is_ok(err)) {
				value_free(val);
				env_free(env);
				ERR_WITH(err, NULL);
			}
		}

		if (value_islist(val)) {
			List_ref list = value_getlist(val, NULL);
			Value_own head;
			List_own tail;

			if (list_len(list) == 0) {
				env_free(env);
				return val;
			}
			head = value_copy(list_nth(list, 0, &err.e.errt));
			if (!rerr_is_ok(err)) {
				value_free(val);
				env_free(env);
				ERR_WITH(err, NULL);
			}
			tail = list_tail(list, 1, &err.e.errt);
			if (!rerr_is_ok(err)) {
				value_free(val);
				value_free(head);
				env_free(env);
				ERR_WITH(err, NULL);
			}

			value_free(val);

			#define TCO() { \
					if (!rerr_is_ok(err)) { \
						env_free(env); \
						RERR_WITH(err, NULL); \
					} \
					if (tci.env != NULL) { \
						env_free(env); \
						env = tci.env; \
					} \
					val = tci.val; \
					if (tci.tailcall) continue; \
					else { \
						env_free(env); \
						return val; \
					} \
					fprints("Something went horribly wrong!", stderr, NULL); \
					exit(1); \
				} do {} while (0)
			if (value_issymbol(head)) {
				String_ref fnname = value_getsymbol(head, NULL);
				if (string_match(fnname, "def!") == C_EQ) {
					value_free(head);

					res = eval_def(tail, env, &err);
					env_free(env);
					RTRY_WITH(err, NULL);
					return res;
				} else if (string_match(fnname, "let*") == C_EQ) {
					value_free(head);

					tci = eval_let(tail, env, &err);
					TCO();
				} else if (string_match(fnname, "do") == C_EQ) {
					value_free(head);

					tci = eval_do(tail, env, &err);
					TCO();
				} else if (string_match(fnname, "if") == C_EQ) {
					value_free(head);

					tci = eval_if(tail, env, &err);
					TCO();
				} else if (string_match(fnname, "fn*") == C_EQ) {
					value_free(head);

					res = eval_lambda(tail, env, &err);
					env_free(env);
					RTRY_WITH(err, NULL);
					return res;
				}
			}

			tci = eval_fn(head, tail, env, &err);
			TCO();
		} else if (value_issymbol(val)) {
			Value_ref res_ref = env_get(
				env,
				value_getsymbol(val, NULL),
				&err.e.errt
			);
			Value_own res;
			if (err.is_errt && err.e.errt == ERR_INVAL) {
				/* symbol not in env */
				err = rerr_undefined_name(value_getsymbol(val, NULL));
				value_free(val);
				env_free(env);
				ERR_WITH(err, NULL);
			} else if (!rerr_is_ok(err)) {
				value_free(val);
				env_free(env);
				ERR_WITH(err, NULL);
			}
			value_free(val);

			res = value_copy(res_ref);
			env_free(env);
			return res;
		} else {
			env_free(env);
			return val;
		}
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

		eval_res = EVAL(read_res, env_copy(env), &err);
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

MutEnv_own repl_env;
static Value_own
mal_eval(List_own args, MutEnv_ref env, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	Value_ref arg;

	(void)env;

	if (list_len(args) != 1) {
		err = rerr_arg_len_mismatch(1, list_len(args));
		list_free(args);
		RERR_WITH(err, NULL);
	}

	arg = value_copy(list_nth(args, 0, NULL));

	list_free(args);

	return EVAL(arg, env_copy(repl_env), err_out);
}

#define LINECAP (16 * 1024)
static char linebuf[LINECAP];

int
main(void)
{
	const char own out;

	repl_env = env_new(NULL, NULL);

	/*env_set(
		repl_env,
		string_new("DEBUG-EVAL", NULL),
		value_bool(true, NULL),
		NULL
	);*/
	core_load(repl_env, NULL);

	free((own_ptr)rep("(def! not (fn* (a) (if a false true)))", repl_env));

	{
		String_own name;
		Fn_own func;
		Value_own value;

		name = string_new("eval", NULL);
		func = fn_builtin(mal_eval, NULL, NULL);
		value = value_fn(func, NULL);

		env_set(repl_env, name, value, NULL);
	}

	free((own_ptr)rep("(def! load-file (fn* (f) (eval (read-string (str \"(do \" (slurp f) \"\nnil)\")))))", repl_env));

	for (;;) {
		prints("user> ", NULL);
		if (!getline(linebuf, LINECAP, NULL)) {
			/* line longer than 16K */
			return 1;
		}
		if (*linebuf == 0) {
			/* hit eof */

			env_free(repl_env);
			env_free_cycles();

			return 0;
		}
		out = rep(linebuf, repl_env);
		if (out != NULL) {
			prints(out, NULL);
			printc('\n', NULL);
			free((own_ptr)out);
		}

		env_free_cycles();
	}

	return 0;
}
