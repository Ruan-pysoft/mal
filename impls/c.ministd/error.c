#include "error.h"

#include <ministd_fmt.h>
#include <ministd_memory.h>
#include <ministd_string.h>

const perr_t PERR_OK = {
	true,
	{ ERR_OK },
};

void
perr_display(const perr_t ref this, FILE ref file, err_t ref err_out)
{
	err_t err = ERR_OK;

	if (this->is_errt) {
		fprints("Internal error occurred: ", file, &err);
		TRY_VOID(err);
		fprints(err_str(this->e.errt), file, err_out);
	} else {
		fprints("PARSE ERROR: ", file, &err);
		TRY_VOID(err);
		fprints(this->e.msg, file, err_out);
	}
}
void
perr_deinit(perr_t this)
{
	if (!this.is_errt) {
		free((own_ptr)this.e.msg);
	}
}
bool
perr_is_ok(perr_t this)
{
	return this.is_errt && this.e.errt == ERR_OK;
}
perr_t
perr_eof(const char ref doing, const char ref expected)
{
	String own str;
	FILE own file;
	char own cstr;
	perr_t res = PERR_OK;

	str = s_new(&res.e.errt);
	if (!perr_is_ok(res)) return res;
	file = (FILE own)sf_open(str, &res.e.errt);
	if (!perr_is_ok(res)) {
		close(file, NULL);
		free(file);
		return res;
	}

	fprints("Encountered EOF while ", file, &res.e.errt);
	if (!perr_is_ok(res)) {
		close(file, NULL);
		free(file);
		s_free(str);
		return res;
	}
	fprints(doing, file, &res.e.errt);
	if (!perr_is_ok(res)) {
		close(file, NULL);
		free(file);
		s_free(str);
		return res;
	}
	fprints(", expected ", file, &res.e.errt);
	if (!perr_is_ok(res)) {
		close(file, NULL);
		free(file);
		s_free(str);
		return res;
	}
	fprints(expected, file, &res.e.errt);
	if (!perr_is_ok(res)) {
		close(file, NULL);
		free(file);
		s_free(str);
		return res;
	}

	close(file, &res.e.errt);
	free(file);
	if (!perr_is_ok(res)) {
		s_free(str);
		return res;
	}

	cstr = nalloc(sizeof(char), s_len(str)+1, &res.e.errt);
	if (!perr_is_ok(res)) {
		s_free(str);
		return res;
	}
	memmove(cstr, s_to_c(str), sizeof(char) * (s_len(str)+1));

	res.is_errt = false;
	res.e.msg = cstr;

	s_free(str);
	return res;
}

const rerr_t RERR_OK = {
	true,
	{ ERR_OK },
};

void
rerr_display(const rerr_t ref this, FILE ref file, err_t ref err_out)
{
	err_t err = ERR_OK;

	if (this->is_errt) {
		fprints("Internal error occurred: ", file, &err);
		TRY_VOID(err);
		fprints(err_str(this->e.errt), file, err_out);
	} else {
		fprints("RUNTIME ERROR: ", file, &err);
		TRY_VOID(err);
		fprints(this->e.msg, file, err_out);
	}
}
void
rerr_deinit(rerr_t this)
{
	if (!this.is_errt) {
		free((own_ptr)this.e.msg);
	}
}
bool
rerr_is_ok(rerr_t this)
{
	return this.is_errt && this.e.errt == ERR_OK;
}
rerr_t
rerr_arg_len_mismatch(usz expected, usz got)
{
	String own str;
	FILE own file;
	char own cstr;
	rerr_t res = RERR_OK;

	str = s_new(&res.e.errt);
	if (!rerr_is_ok(res)) return res;
	file = (FILE own)sf_open(str, &res.e.errt);
	if (!rerr_is_ok(res)) {
		close(file, NULL);
		free(file);
		return res;
	}

	fprints(
		"Argument length mismatch while calling function: expected ",
		file,
		&res.e.errt
	);
	if (!rerr_is_ok(res)) {
		close(file, NULL);
		free(file);
		s_free(str);
		return res;
	}
	fprinti(expected, file, &res.e.errt);
	if (!rerr_is_ok(res)) {
		close(file, NULL);
		free(file);
		s_free(str);
		return res;
	}
	fprints(", got ", file, &res.e.errt);
	if (!rerr_is_ok(res)) {
		close(file, NULL);
		free(file);
		s_free(str);
		return res;
	}
	fprinti(got, file, &res.e.errt);
	if (!rerr_is_ok(res)) {
		close(file, NULL);
		free(file);
		s_free(str);
		return res;
	}

	close(file, &res.e.errt);
	free(file);
	if (!rerr_is_ok(res)) {
		s_free(str);
		return res;
	}

	cstr = nalloc(sizeof(char), s_len(str)+1, &res.e.errt);
	if (!rerr_is_ok(res)) {
		s_free(str);
		return res;
	}
	memmove(cstr, s_to_c(str), sizeof(char) * (s_len(str)+1));

	res.is_errt = false;
	res.e.msg = cstr;

	s_free(str);
	return res;
}
rerr_t
rerr_arg_vararg_mismatch(usz minimum, usz got)
{
	String own str;
	FILE own file;
	char own cstr;
	rerr_t res = RERR_OK;

	str = s_new(&res.e.errt);
	if (!rerr_is_ok(res)) return res;
	file = (FILE own)sf_open(str, &res.e.errt);
	if (!rerr_is_ok(res)) {
		close(file, NULL);
		free(file);
		return res;
	}

	fprints(
		"Argument length mismatch while calling variadic function: expected at least ",
		file,
		&res.e.errt
	);
	if (!rerr_is_ok(res)) {
		close(file, NULL);
		free(file);
		s_free(str);
		return res;
	}
	fprinti(minimum, file, &res.e.errt);
	if (!rerr_is_ok(res)) {
		close(file, NULL);
		free(file);
		s_free(str);
		return res;
	}
	fprints(", got ", file, &res.e.errt);
	if (!rerr_is_ok(res)) {
		close(file, NULL);
		free(file);
		s_free(str);
		return res;
	}
	fprinti(got, file, &res.e.errt);
	if (!rerr_is_ok(res)) {
		close(file, NULL);
		free(file);
		s_free(str);
		return res;
	}

	close(file, &res.e.errt);
	free(file);
	if (!rerr_is_ok(res)) {
		s_free(str);
		return res;
	}

	cstr = nalloc(sizeof(char), s_len(str)+1, &res.e.errt);
	if (!rerr_is_ok(res)) {
		s_free(str);
		return res;
	}
	memmove(cstr, s_to_c(str), sizeof(char) * (s_len(str)+1));

	res.is_errt = false;
	res.e.msg = cstr;

	s_free(str);
	return res;
}
