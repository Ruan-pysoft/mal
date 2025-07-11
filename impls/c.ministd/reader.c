#include "reader.h"
#include "values.h"

#include <ministd.h>
#include <ministd_error.h>
#include <ministd_fmt.h>
#include <ministd_memory.h>
#include <ministd_string.h>


struct token {
	usz offset;
	usz len;
};

struct token_arr {
	struct token own toks;
	usz len;
};
static void
token_arr_deinit(struct token_arr ref this)
{
	free(this->toks);
	this->toks = NULL;
	this->len = 0;
}

static bool
special(char c) {
	return c != 0 && (
		c <= ' ' || c == '[' || c == ']' || c == '{' || c == '}'
		|| c == '(' || c == ')' || c == '\'' || c == '"' || c == ','
		|| c == ';'
	);
}
static bool
match_token(const char ref src, const struct token ref tok, const char ref to)
{
	usz i;

	if (strnlen(to, tok->len+1) != tok->len) return false;

	for (i = 0; i < tok->len; ++i) {
		if (src[tok->offset+i] != to[i]) return false;
	}

	return true;
}
static bool
get_token(const char ref str, usz ref idx, struct token ref out)
{
	while (str[*idx] != 0 && (str[*idx] <= ' ' || str[*idx] == ',')) {
		++*idx;
	}

	#define CHAR_TOK() do { \
		out->len = 1; \
		out->offset = *idx; \
		++*idx; \
	} while (0)
	if (str[*idx] == '~' && str[*idx] == '@') {
		out->len = 2;
		out->offset = *idx;
		*idx += 2;

		return true;
	} else if (str[*idx] == '"') {
		usz len = 1;
		bool escape = false;

		while (str[*idx+len] != 0 && (escape ||
				(str[*idx+len] != '"'
				&& str[*idx+len] != '\n'))) {
			escape = (str[*idx+len] == '\\');

			++len;
		}

		if (str[*idx+len] == '"') ++len;
		else {
			fprints("ERROR: Unclosed string literal\n", stderr, NULL);
		}

		out->len = len;
		out->offset = *idx;

		*idx += len;

		return true;
	} else if (str[*idx] == ';') {
		const usz len = strlen(&str[*idx]);
		out->len = len;
		out->offset = *idx;
		*idx += len;

		return true;
	} else if (special(str[*idx])) {
		CHAR_TOK();
		return true;
	} else if (str[*idx] == '~') {
		CHAR_TOK();
		return true;
	} else if (str[*idx] == '^') {
		CHAR_TOK();
		return true;
	} else if (str[*idx] == '@') {
		CHAR_TOK();
		return true;
	} else {
		usz len = 0;

		while (str[*idx+len] != 0 && !special(str[*idx+len])) {
			++len;
		}

		if (len > 0) {
			out->len = len;
			out->offset = *idx;

			*idx += len;

			return true;
		}
	}

	return false;
}
static struct token_arr
tokenize(const char ref str, err_t ref err_out)
{
	err_t err = ERR_OK;
	const struct token_arr empty = {
		NULL,
		0,
	};
	struct token_arr res;
	usz cap = 16;
	usz str_idx = 0;
	bool had_token;

	res.toks = alloc(sizeof(*res.toks) * cap, &err);
	res.len = 0;
	TRY_WITH(err, empty);

	do {
		if (res.len == cap) {
			res.toks = realloc(
				res.toks,
				sizeof(*res.toks) * cap * 2,
				&err
			);
			TRY_WITH(err, empty);
			cap *= 2;
		}

		had_token = get_token(str, &str_idx, &res.toks[res.len]);
		if (had_token) ++res.len;
	} while (had_token);

	return res;
}

struct reader {
	const char ref src;
	struct token_arr toks;
	usz idx;
};

static struct reader own
reader_new(const char ref src, struct token_arr toks, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct reader own res;

	res = alloc(sizeof(*res), &err);
	if (err != ERR_OK) {
		token_arr_deinit(&toks);
		ERR_WITH(err, NULL);
	}

	res->src = src;
	res->toks = toks;
	res->idx = 0;

	return res;
}
static void
reader_free(struct reader own this)
{
	token_arr_deinit(&this->toks);
	free(this);
}
static const struct token ref
r_next(struct reader ref this)
{
	return this->idx >= this->toks.len
		? NULL
		: &this->toks.toks[this->idx++];
}
static const struct token ref
r_peek(const struct reader ref this)
{
	return this->idx >= this->toks.len ? NULL : &this->toks.toks[this->idx];
}

static const value_t own read_form(struct reader ref this, err_t ref err_out);
static const struct cell own
read_list(struct reader ref this, err_t ref err_out)
{
	err_t err = ERR_OK;
	const struct token ref tok;
	const struct cell own res = NULL;
	const struct cell ref tail = NULL;
	const value_t own val;

	tok = r_peek(this);
	if (tok == NULL || tok->len == 0) {
		/* no tokens found OR token of length zero */
		fprints("ERROR: Unclosed list!\n", stderr, err_out);
		return NULL;
	}
	if (this->src[tok->offset] == ')') {
		/* empty list */
		r_next(this);
		return NULL;
	}

	val = read_form(this, &err);
	TRY_WITH(err, NULL);

	res = cell_new_own(val, &err);
	TRY_WITH(err, NULL);
	tail = res;

	tok = r_peek(this);
	while (tok != NULL && tok->len != 0 && this->src[tok->offset] != ')') {
		val = read_form(this, &err);
		if (err != ERR_OK) {
			cell_free(res);
			ERR_WITH(err, NULL);
		}

		_cell_unsafe_append((ptr)tail, val, &err);
		if (err != ERR_OK) {
			cell_free(res);
			ERR_WITH(err, NULL);
		}

		tail = cell_tail(tail);

		tok = r_peek(this);
	}

	if (tok == NULL || tok->len == 0) {
		/* no tokens found OR token of length zero */
		fprints("ERROR: Unclosed list!\n", stderr, err_out);
	} else { /* closing paren */
		r_next(this);
	}

	return res;
}
static const value_t own
read_atom(struct reader ref this, err_t ref err_out)
{
	const struct token ref tok = r_next(this);

	if (tok == NULL || tok->len == 0) {
		/* this should never happen */
		fprints(
			"ERROR: called `read_atom` at end of token stream!\n",
			stderr,
			err_out
		);
		exit(1);
	}

	if ('0' <= this->src[tok->offset] && this->src[tok->offset] <= '9') {
		/* number */

		err_t err = ERR_OK;
		int n = 0;
		usz i;

		for (i = 0; i < tok->len; ++i) {
			const char c = this->src[tok->offset+i];

			if ('0' > c || c > '9') {
				fprints(
					"ERROR: non-numeric character in numeric literal!\n",
					stderr,
					&err
				);
				TRY_WITH(err, NULL);

				return value_num(0, err_out);
			}

			n *= 10;
			n += c - '0';
		}

		return value_num(n, err_out);
	} else if (match_token(this->src, tok, "nil")) {
		return value_nil(err_out);
	} else if (match_token(this->src, tok, "true")) {
		return value_bool(true, err_out);
	} else if (match_token(this->src, tok, "false")) {
		return value_bool(false, err_out);
	} else if (this->src[tok->offset] == '"') {
		/* string */

		/* TODO: actually parse the string */

		err_t err = ERR_OK;
		const struct mal_string own str = mal_string_newn(
			&this->src[tok->offset], tok->len, &err
		);
		TRY_WITH(err, NULL);

		return value_str_own(str, err_out);
	} else {
		/* symbol */

		err_t err = ERR_OK;
		const struct mal_string own sym = mal_string_newn(
			&this->src[tok->offset], tok->len, &err
		);
		TRY_WITH(err, NULL);

		return value_sym_own(sym, err_out);
	}
}
static const value_t own
read_form(struct reader ref this, err_t ref err_out)
{
	const struct token ref tok;

	tok = r_peek(this);
	if (tok == NULL || tok->len == 0) {
		/* no tokens found OR first token of length zero */
		return value_list_own(NULL, err_out);
	}

	if (this->src[tok->offset] == '(') {
		err_t err = ERR_OK;
		const struct cell own list;

		r_next(this); /* skip opening paren */
		list = read_list(this, &err);
		TRY_WITH(err, NULL);
		return value_list_own(list, err_out);
	} else {
		return read_atom(this, err_out);
	}
}
const value_t own
read_str(const char ref str, err_t ref err_out)
{
	err_t err = ERR_OK;
	struct token_arr arr;
	struct reader own reader;
	const value_t own res;

	arr = tokenize(str, &err);
	TRY_WITH(err, NULL);

	reader = reader_new(str, arr, &err);
	if (err != ERR_OK) {
		token_arr_deinit(&arr);
		ERR_WITH(err, NULL);
	}

	res = read_form(reader, &err);
	reader_free(reader);
	TRY_WITH(err, NULL);

	return res;
}
