#include "reader.h"

#include <ministd.h>
#include <ministd_error.h>
#include <ministd_fmt.h>
#include <ministd_memory.h>
#include <ministd_string.h>


#include "error.h"
#include "values.h"

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
get_token(const char ref str, usz ref idx, struct token ref out, perr_t ref err_out)
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
				(str[*idx+len] != '"'))) {
			escape = !escape && (str[*idx+len] == '\\');

			++len;
		}

		if (str[*idx+len] == '"') ++len;
		else {
			perr_t err = perr_eof("parsing string", "closing '\"'");
			ERR_WITH(err, false);
		}

		out->len = len;
		out->offset = *idx;

		*idx += len;

		return true;
	} else if (str[*idx] == ';') {
		const usz len = strlen(&str[*idx]);
		*idx += len;

		return false;
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
tokenize(const char ref str, perr_t ref err_out)
{
	perr_t err = PERR_OK;
	const struct token_arr empty = {
		NULL,
		0,
	};
	struct token_arr res;
	usz cap = 16;
	usz str_idx = 0;
	bool had_token;

	res.toks = alloc(sizeof(*res.toks) * cap, &err.e.errt);
	PTRY_WITH(err, empty);
	res.len = 0;

	do {
		if (res.len == cap) {
			res.toks = realloc(
				res.toks,
				sizeof(*res.toks) * cap * 2,
				&err.e.errt
			);
			PTRY_WITH(err, empty);
			cap *= 2;
		}

		had_token = get_token(str, &str_idx, &res.toks[res.len], &err);
		if (!perr_is_ok(err)) {
			free(res.toks);
			PERR_WITH(err, empty);
		}
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

static Value_own read_form(struct reader ref this, perr_t ref err_out);
static List_own
read_list(struct reader ref this, perr_t ref err_out)
{
	perr_t err = PERR_OK;
	const struct token ref tok;
	List_own res;
	Value_own val;

	res = list_new(NULL, 0, &err.e.errt);
	PTRY_WITH(err, NULL);

	tok = r_peek(this);
	if (tok == NULL || tok->len == 0) {
		/* no tokens found OR token of length zero */
		list_free(res);
		err = perr_eof("parsing list", "closing ')'");
		PERR_WITH(err, NULL);
	}

	while (tok != NULL && tok->len != 0 && this->src[tok->offset] != ')') {
		val = read_form(this, &err);
		if (!perr_is_ok(err)) {
			list_free(res);
			ERR_WITH(err, NULL);
		}

		res = _list_append(
			(struct List_struct own)res,
			val,
			&err.e.errt
		);
		if (!perr_is_ok(err)) {
			list_free(res);
			PERR_WITH(err, NULL);
		}

		tok = r_peek(this);
	}

	if (tok == NULL || tok->len == 0) {
		/* no tokens found OR token of length zero */
		list_free(res);
		err = perr_eof("parsing list", "closing ')'");
		PERR_WITH(err, NULL);
	} else { /* closing paren */
		r_next(this);
	}

	return res;
}
static String_own
read_string(struct reader ref this, const struct token ref tok,
	    perr_t ref err_out)
{
	perr_t err = PERR_OK;
	String own str;
	FILE own file;
	usz offset;
	String_own res;

	str = s_newalloc(tok->len, &err.e.errt);
	PTRY_WITH(err, NULL);
	file = (FILE own)sf_open(str, &err.e.errt);
	if (!perr_is_ok(err)) {
		s_free(str);
		PERR_WITH(err, NULL);
	}

	offset = tok->offset + 1;
	while (offset < tok->offset + tok->len - 1) {
		if (this->src[offset] == '\\') {
			char c;
			++offset;

			c = this->src[offset];

			if (c == 'n') {
				fprintc('\n', file, &err.e.errt);
			} else {
				fprintc(c, file, &err.e.errt);
			}
			if (!perr_is_ok(err)) {
				close(file, NULL);
				free(file);
				s_free(str);
				PERR_WITH(err, NULL);
			}
		} else {
			fprintc(this->src[offset], file, &err.e.errt);
			if (!perr_is_ok(err)) {
				close(file, NULL);
				free(file);
				s_free(str);
				PERR_WITH(err, NULL);
			}
		}

		++offset;
	}

	close(file, &err.e.errt);
	free(file);
	if (!perr_is_ok(err)) {
		s_free(str);
		PERR_WITH(err, NULL);
	}

	res = string_new(s_to_c(str), &err.e.errt);
	s_free(str);
	PTRY_WITH(err, NULL);

	return res;
}
static Value_own
read_atom(struct reader ref this, perr_t ref err_out)
{
	err_t err = ERR_OK;
	const struct token ref tok = r_next(this);
	Value_own res;

	if (tok == NULL || tok->len == 0) {
		/* this should never happen */
		fprints(
			"ERROR: called `read_atom` at end of token stream!\n",
			stderr,
			&err
		);
		exit(1);
	}

	if ((this->src[tok->offset] == '-' && '0' <= this->src[tok->offset+1]
			&& this->src[tok->offset+1] <= '9')
			|| ('0' <= this->src[tok->offset]
			&& this->src[tok->offset] <= '9')) {
		/* number */

		bool neg = this->src[tok->offset] == '-';
		int n = 0;
		usz i;

		for (i = neg ? 1 : 0; i < tok->len; ++i) {
			const char c = this->src[tok->offset+i];

			if ('0' > c || c > '9') {
				/* TODO: return parse error here */
				fprints(
					"ERROR: non-numeric character in numeric literal!\n",
					stderr,
					&err
				);
				PTRY_FROM_WITH(err, NULL);

				res = value_number(0, &err);
				PTRY_FROM_WITH(err, NULL);
				return res;
			}

			n *= 10;
			n += c - '0';
		}

		res = value_number(neg ? -n : n, &err);
		PTRY_FROM_WITH(err, NULL);
		return res;
	} else if (match_token(this->src, tok, "nil")) {
		res = value_nil(&err);
		PTRY_FROM_WITH(err, NULL);
		return res;
	} else if (match_token(this->src, tok, "true")) {
		res = value_bool(true, &err);
		PTRY_FROM_WITH(err, NULL);
		return res;
	} else if (match_token(this->src, tok, "false")) {
		res = value_bool(false, &err);
		PTRY_FROM_WITH(err, NULL);
		return res;
	} else if (this->src[tok->offset] == '"') {
		/* string */

		perr_t perr = PERR_OK;

		String_own str = read_string(this, tok, &perr);
		PTRY_FROM_WITH(err, NULL);

		res = value_string(str, &err);
		PTRY_FROM_WITH(err, NULL);
		return res;
	} else if (this->src[tok->offset] == ':') {
		/* key */

		String_own key = string_newn(
			&this->src[tok->offset],
			tok->len,
			&err
		);
		PTRY_FROM_WITH(err, NULL);

		res = value_keyword(key, &err);
		PTRY_FROM_WITH(err, NULL);
		return res;
	} else if (match_token(this->src, tok, "@")) {
		perr_t err = PERR_OK;
		String_own str;
		Value_own val;
		List_own res;

		res = list_new(NULL, 0, &err.e.errt);
		PTRY_WITH(err, NULL);

		str = string_new("deref", &err.e.errt);
		if (!perr_is_ok(err)) {
			list_free(res);
			PERR_WITH(err, NULL);
		}
		val = value_symbol(str, &err.e.errt);
		if (!perr_is_ok(err)) {
			list_free(res);
			PERR_WITH(err, NULL);
		}
		res = _list_append(
			(struct List_struct own)res,
			val,
			&err.e.errt
		);
		PTRY_WITH(err, NULL);

		val = read_form(this, &err);
		if (!perr_is_ok(err)) {
			list_free(res);
			PERR_WITH(err, NULL);
		}
		res = _list_append(
			(struct List_struct own)res,
			val,
			&err.e.errt
		);
		PTRY_WITH(err, NULL);

		val = value_list(res, &err.e.errt);
		PTRY_WITH(err, NULL);

		return val;
	} else {
		/* symbol */

		String_own sym = string_newn(
			&this->src[tok->offset],
			tok->len,
			&err
		);
		PTRY_FROM_WITH(err, NULL);

		res = value_symbol(sym, &err);
		PTRY_FROM_WITH(err, NULL);
		return res;
	}
}
static Value_own
read_form(struct reader ref this, perr_t ref err_out)
{
	perr_t err = PERR_OK;
	const struct token ref tok;
	List_own list;
	Value_own res;

	tok = r_peek(this);
	if (tok == NULL || tok->len == 0) {
		/* no tokens found OR first token of length zero */
		list = list_new(NULL, 0, &err.e.errt);
		PTRY_WITH(err, NULL);
		res = value_list(list, &err.e.errt);
		PTRY_WITH(err, NULL);
		return res;
	}

	if (this->src[tok->offset] == '(') {

		r_next(this); /* skip opening paren */
		list = read_list(this, &err);
		PTRY_WITH(err, NULL);

		res = value_list(list, &err.e.errt);
		PTRY_WITH(err, NULL);
		return res;
	} else {
		return read_atom(this, err_out);
	}
}
Value_own
read_str(const char ref str, perr_t ref err_out)
{
	perr_t err = PERR_OK;
	struct token_arr arr;
	struct reader own reader;
	Value_own res;

	arr = tokenize(str, &err);
	PTRY_WITH(err, NULL);

	reader = reader_new(str, arr, &err.e.errt);
	if (!perr_is_ok(err)) {
		token_arr_deinit(&arr);
		PERR_WITH(err, NULL);
	}

	res = read_form(reader, &err);
	reader_free(reader);
	PTRY_WITH(err, NULL);

	return res;
}
