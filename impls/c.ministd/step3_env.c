#include <ministd.h>
#include <ministd_error.h>
#include <ministd_fmt.h>
#include <ministd_memory.h>

#ifndef FULL_ENV
#define FULL_ENV
#endif
#include "env.h"
#include "error.h"
#include "printer.h"
#include "reader.h"
#include "values.h"

/* `read` is a function in `ministd_fmt.h`...
 * I should either add optional prefixes to all ministd functions,
 * or I should just change read/write/close/etc
 * to file_read/file_write/file_close etc...
 */
static const value_t own
READ(const char ref line)
{
	perr_t err = PERR_OK;
	const value_t own val;

	val = read_str(line, &err);
	if (err.type != PT_OK) {
		perr_display(&err, stderr, NULL);
		fprintc('\n', stderr, NULL);
		return NULL;
	}

	return val;
}

static const value_t own EVAL(const value_t own val, env_t ref env,
			      rerr_t ref err_out);
static void
bind(env_t ref this, const struct cell own bindings, rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	err_t errt = ERR_OK;
	const struct cell own old;
	const value_t own name;
	const value_t own value;

	while (bindings != NULL) {
		if (bindings->next == NULL) {
			cell_free(bindings);

			err.type = RT_ARG_MISMATCH;
			err.e.msg = ": let*'s binding list must have an even number of arguments!";
			RERR_VOID(err);
		}

		name = value_copy(bindings->val);
		if (name->type != VT_SYM) {
			cell_free(bindings);

			err.type = RT_ARG_MISMATCH;
			err.e.msg = ": every odd element of let*'s binding list must be a symbol!";
			RERR_VOID(err);
		}
		value = EVAL(value_copy(bindings->next->val), this, &err);
		if (err.type != RT_OK) {
			cell_free(bindings);
			value_free(name);
			RERR_VOID(err);
		}

		env_set(this, name->v.sym, value, &errt);
		if (errt != ERR_OK) {
			cell_free(bindings);
			RERR_FROM_VOID(errt);
		}

		old = bindings;
		bindings = cell_copy(old->next->next);
		cell_free(old);
	}
}
static const struct cell own
eval_ls(const struct cell own ls, env_t ref env, usz ref len,
	rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	err_t errt = ERR_OK;
	const struct cell own res = NULL;
	const struct cell own old;
	const struct cell ref last;
	const value_t own val;

	if (ls == NULL) return NULL;

	val = EVAL(value_copy(ls->val), env, &err);
	if (err.type != RT_OK) {
		cell_free(ls);
		RERR_WITH(err, NULL);
	}
	res = cell_new_own(val, &errt);
	if (errt != ERR_OK) {
		cell_free(ls);
		RERR_FROM_WITH(errt, NULL);
	}
	last = res;
	*len = 1;

	old = ls;
	ls = cell_copy(old->next);
	cell_free(old);

	while (ls != NULL) {
		val = EVAL(value_copy(ls->val), env, &err);
		if (err.type != RT_OK) {
			cell_free(ls);
			cell_free(res);
			RERR_WITH(err, NULL);
		}
		_cell_unsafe_append((ptr)last, val, &errt);
		if (errt != ERR_OK) {
			cell_free(ls);
			cell_free(res);
			RERR_FROM_WITH(errt, NULL);
		}
		last = cell_tail(last);
		++*len;

		old = ls;
		ls = cell_copy(old->next);
		cell_free(old);
	}

	return res;
}
static const value_t own
eval_fn(const value_t own head, const struct cell own tail, env_t ref env,
	rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	err_t errt = ERR_OK;
	const value_t own fn;

	fn = EVAL(head, env, &err);
	if (err.type != RT_OK) {
		cell_free(tail);
		RERR_WITH(err, NULL);
	}

	switch (fn->type) {
		case VT_LIST: {
			err.type = RT_UNCALLABLE;
			err.e.msg = ": Lists aren't callable";
			ERR_WITH(err, NULL);
		break; }
		case VT_SYM: {
			err.type = RT_UNCALLABLE;
			err.e.msg = ": Symbols aren't callable";
			ERR_WITH(err, NULL);
		break; }
		case VT_NUM: {
			err.type = RT_UNCALLABLE;
			err.e.msg = ": Numbers aren't callable";
			ERR_WITH(err, NULL);
		break; }
		case VT_NIL: {
			err.type = RT_UNCALLABLE;
			err.e.msg = ": Nil isn't callable";
			ERR_WITH(err, NULL);
		break; }
		case VT_BOOL: {
			err.type = RT_UNCALLABLE;
			err.e.msg = ": Booleans aren't callable";
			ERR_WITH(err, NULL);
		break; }
		case VT_STR: {
			err.type = RT_UNCALLABLE;
			err.e.msg = ": Strings aren't callable";
			ERR_WITH(err, NULL);
		break; }
		case VT_KEY: {
			err.type = RT_UNCALLABLE;
			err.e.msg = ": Keys aren't callable";
			ERR_WITH(err, NULL);
		break; }
		case VT_BUILTIN_FN: {
			const struct cell own args = NULL;
			usz arg_count = 0;
			int res;
			const value_t own val;

			args = eval_ls(tail, env, &arg_count, &err);
			if (err.type != RT_OK) {
				value_free(fn);
				RERR_WITH(err, NULL);
			}

			if (arg_count != 2) {
				value_free(fn);
				cell_free(args);

				err.type = RT_ARG_MISMATCH;
				err.e.msg = ": Builtin functions only take two arguments!";
				RERR_WITH(err, NULL);
			}
			if (args->val->type != VT_NUM || args->next->val->type != VT_NUM) {
				value_free(fn);
				cell_free(args);

				err.type = RT_ARG_MISMATCH;
				err.e.msg = ": Builtin functions only take numeric arguments!";
				ERR_WITH(err, NULL);
			}

			res = fn->v.builtin(
				args->val->v.num,
				args->next->val->v.num
			);

			val = value_num(res, &errt);
			value_free(fn);
			cell_free(args);
			RTRY_FROM_WITH(errt, NULL);
			return val;
		break; }
	}

	return NULL;
}

static const value_t own
eval_def(const struct cell own tail, env_t ref env, rerr_t ref err_out)
{
	err_t errt = ERR_OK;
	rerr_t err = RERR_OK;
	const value_t own fst;
	const value_t own scd;

	if (tail == NULL || tail->next == NULL || tail->next->next != NULL) {
		cell_free(tail);

		err.type = RT_ARG_MISMATCH;
		err.e.msg = ": def! only takes two arguments!";
		RERR_WITH(err, NULL);
	}

	fst = value_copy(tail->val);
	scd = value_copy(tail->next->val);
	cell_free(tail);
	if (fst->type != VT_SYM) {
		value_free(fst);
		value_free(scd);

		err.type = RT_ARG_MISMATCH;
		err.e.msg = ": def!'s first argument must be a symbol!";
		RERR_WITH(err, NULL);
	}

	scd = EVAL(scd, env, &err);
	if (err.type != RT_OK) {
		value_free(fst);
		RERR_WITH(err, NULL);
	}

	env_set(
		env,
		mal_string_copy(fst->v.sym),
		value_copy(scd),
		&errt
	);
	value_free(fst);
	if (errt != ERR_OK) {
		value_free(scd);
		RERR_FROM_WITH(errt, NULL);
	}

	return scd;
}
static const value_t own
eval_let(const struct cell own tail, const env_t ref env, rerr_t ref err_out)
{
	err_t errt = ERR_OK;
	rerr_t err = RERR_OK;
	const value_t own bindings;
	const value_t own expr;
	env_t own subenv;

	if (tail == NULL || tail->next == NULL || tail->next->next != NULL) {
		cell_free(tail);

		err.type = RT_ARG_MISMATCH;
		err.e.msg = ": let* only takes two arguments!";
		RERR_WITH(err, NULL);
	}

	bindings = value_copy(tail->val);
	expr = value_copy(tail->next->val);
	cell_free(tail);
	if (bindings->type != VT_LIST) {
		value_free(bindings);
		value_free(expr);

		err.type = RT_ARG_MISMATCH;
		err.e.msg = ": let*'s first argument must be a bindings list!";
		RERR_WITH(err, NULL);
	}

	subenv = env_new(env, &errt);
	if (errt != ERR_OK) {
		value_free(bindings);
		value_free(expr);

		RERR_FROM_WITH(errt, NULL);
	}

	bind(subenv, bindings->v.ls, &err);
	if (err.type != RT_OK) {
		value_free(expr);
		env_free(subenv);

		RERR_WITH(err, NULL);
	}

	return EVAL(expr, subenv, err_out);
}

static const value_t own
EVAL(const value_t own val, env_t ref env, rerr_t ref err_out)
{
	static const struct mal_string debug_key = {
		"DEBUG-EVAL", 0
	};
	const value_t ref debug = env_get(env, &debug_key);
	if (debug != NULL && debug->type != VT_NIL && (debug->type != VT_BOOL
			|| debug->v.boo != false)) {
		err_t err = ERR_OK;
		const char own repr = pr_str(val, 1, &err);
		RTRY_FROM_WITH(err, NULL);

		fprints("EVAL: ", stderr, &err);
		RTRY_FROM_WITH(err, NULL);
		fprints(repr, stderr, &err);
		RTRY_FROM_WITH(err, NULL);
		fprintc('\n', stderr, &err);
		RTRY_FROM_WITH(err, NULL);
	}

	switch (val->type) {
		case VT_LIST: {
			const value_t own head;
			const struct cell own tail;
			if (val->v.ls == NULL) {
				return val;
			}
			head = value_copy(val->v.ls->val);
			tail = cell_copy(val->v.ls->next);

			value_free(val);

			if (head->type == VT_SYM) {
				if (mal_string_match(head->v.sym, "def!") == CR_EQ) {
					value_free(head);

					return eval_def(tail, env, err_out);
				} else if (mal_string_match(head->v.sym, "let*") == CR_EQ) {
					value_free(head);

					return eval_let(tail, env, err_out);
				}
			}

			return eval_fn(head, tail, env, err_out);
		break; }
		case VT_SYM: {
			const value_t ref res = env_get(env, val->v.sym);
			value_free(val);
			if (res == NULL) {
				rerr_t err = RERR_OK;
				err.type = RT_NOT_FOUND;
				err.e.msg = "";
				RERR_WITH(err, NULL);
			} else {
				return value_copy(res);
			}
		break; }
		default: return val;
	}
}

static const char own
PRINT(const value_t own value)
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
rep(const char ref line, env_t ref env)
{
	const value_t own read_res;
	const value_t own eval_res;
	const char own print_res;

	read_res = READ(line);
	if (read_res != NULL) {
		rerr_t err = RERR_OK;

		eval_res = EVAL(read_res, env, &err);
		if (err.type != RT_OK) {
			rerr_display(&err, stderr, NULL);
			fprintc('\n', stderr, NULL);

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
op_add(int a, int b)
{ return a+b; }
int
op_sub(int a, int b)
{ return a-b; }
int
op_mul(int a, int b)
{ return a*b; }
int
op_div(int a, int b)
{ return a/b; }

int
main(void)
{
	const char own out;
	env_t own repl_env = env_new(NULL, NULL);

	env_set(
		repl_env,
		mal_string_new("DEBUG-EVAL", NULL),
		value_bool(true, NULL),
		NULL
	);
	env_set(
		repl_env,
		mal_string_new("+", NULL),
		value_builtin(op_add, NULL),
		NULL
	);
	env_set(
		repl_env,
		mal_string_new("-", NULL),
		value_builtin(op_sub, NULL),
		NULL
	);
	env_set(
		repl_env,
		mal_string_new("*", NULL),
		value_builtin(op_mul, NULL),
		NULL
	);
	env_set(
		repl_env,
		mal_string_new("/", NULL),
		value_builtin(op_div, NULL),
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
