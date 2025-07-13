#include <ministd.h>
#include <ministd_error.h>
#include <ministd_fmt.h>
#include <ministd_memory.h>

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
static const value_t own
eval_fn(const value_t own head, const struct cell own tail, env_t ref env,
	rerr_t ref err_out)
{
	rerr_t err = RERR_OK;
	err_t errt = ERR_OK;
	const value_t own fn;
	const struct cell own args = NULL;
	usz arg_count = 0;

	fn = EVAL(head, env, &err);
	if (err.type != RT_OK) {
		cell_free(tail);
		RERR_WITH(err, NULL);
	}
	if (tail != NULL) {
		const value_t own val;
		const struct cell own old;
		const struct cell ref last_arg;

		val = EVAL(value_copy(tail->val), env, &err);
		if (err.type != RT_OK) {
			value_free(fn);
			cell_free(tail);
			RERR_WITH(err, NULL);
		}
		args = cell_new_own(val, &errt);
		if (errt != ERR_OK) {
			value_free(fn);
			cell_free(tail);
			RERR_FROM_WITH(errt, NULL);
		}
		last_arg = args;
		arg_count = 1;

		old = tail;
		tail = cell_copy(old->next);
		cell_free(old);

		while (tail != NULL) {
			val = EVAL(value_copy(tail->val), env, &err);
			if (err.type != RT_OK) {
				value_free(fn);
				cell_free(tail);
				cell_free(args);
				RERR_WITH(err, NULL);
			}
			_cell_unsafe_append((ptr)last_arg, val, &errt);
			if (errt != ERR_OK) {
				value_free(fn);
				cell_free(tail);
				cell_free(args);
				RERR_WITH(err, NULL);
			}
			last_arg = cell_tail(last_arg);
			++arg_count;

			old = tail;
			tail = cell_copy(old->next);
			cell_free(old);
		}
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
			int res;
			const value_t own val;

			if (arg_count != 2) {
				value_free(fn);
				cell_free(args);

				err.type = RT_ARG_MISMATCH;
				err.e.msg = ": Builtin functions only take two arguments!";
				ERR_WITH(err, NULL);
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
EVAL(const value_t own val, env_t ref env, rerr_t ref err_out)
{
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
	env_t own repl_env = env_new(NULL);

	env_add(
		repl_env,
		mal_string_new("+", NULL),
		value_builtin(op_add, NULL),
		NULL
	);
	env_add(
		repl_env,
		mal_string_new("-", NULL),
		value_builtin(op_sub, NULL),
		NULL
	);
	env_add(
		repl_env,
		mal_string_new("*", NULL),
		value_builtin(op_mul, NULL),
		NULL
	);
	env_add(
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
