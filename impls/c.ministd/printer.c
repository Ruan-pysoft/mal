#include "printer.h"

#include <ministd_fmt.h>
#include <ministd_memory.h>
#include <ministd_string.h>

static void pr_val(const value_t ref val, FILE ref file, err_t ref err_out);

static void
pr_list(const struct cell ref list, FILE ref file, err_t ref err_out)
{
	err_t err = ERR_OK;
	bool first = true;
	const struct cell ref curr;

	if (list == NULL) {
		fprints("()", file, &err);
		return;
	}

	fprintc('(', file, &err);
	TRY_VOID(err);

	for (curr = list; curr != NULL; curr = curr->next) {
		if (!first) {
			fprintc(' ', file, &err);
			TRY_VOID(err);
		}
		first = false;

		pr_val(curr->val, file, &err);
		TRY_VOID(err);
	}

	fprintc(')', file, &err);
	TRY_VOID(err);
}
static void
pr_num(int num, FILE ref file, err_t ref err_out)
{
	fprinti(num, file, err_out);
}
static void
pr_sym(const struct mal_string ref sym, FILE ref file, err_t ref err_out)
{
	fprints(sym->data, file, err_out);
}

static void
pr_val(const value_t ref val, FILE ref file, err_t ref err_out)
{
	switch (val->type) {
		case VT_LIST: {
			pr_list(val->v.ls, file, err_out);
		break; }
		case VT_NUM: {
			pr_num(val->v.num, file, err_out);
		break; }
		case VT_SYM: {
			pr_sym(val->v.sym, file, err_out);
		break; }
	}
}

const char own
pr_str(const value_t ref val, err_t ref err_out)
{
	err_t err = ERR_OK;
	const char own res;
	String own str;
	StringFile own file;

	str = s_new(&err);
	TRY_WITH(err, NULL);

	file = sf_open(str, &err);
	if (err != ERR_OK) {
		s_free(str);
		ERR_WITH(err, NULL);
	}

	pr_val(val, (FILE ref)file, &err);
	if (err != ERR_OK) {
		close((FILE ref)file, NULL);
		s_free(str);
		ERR_WITH(err, NULL);
	}

	close((FILE ref)file, NULL);

	/* WARN: some unsafe string operations,
	 * I should probably add library support for this
	 */
	res = str->base;
	free(str);
	return res;
}
