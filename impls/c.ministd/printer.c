#include "printer.h"
#include "values.h"

#include <ministd_fmt.h>
#include <ministd_memory.h>
#include <ministd_string.h>

#include "types.h"
#include "values.h"

const char own
pr_str(Value_ref val, bool print_readably, err_t ref err_out)
{
	err_t err = ERR_OK;
	char own res;
	String own str;
	StringFile own file;

	str = s_new(&err);
	TRY_WITH(err, NULL);

	file = sf_open(str, &err);
	if (err != ERR_OK) {
		s_free(str);
		ERR_WITH(err, NULL);
	}

	value_print(val, print_readably, (FILE ref)file, &err);
	if (err != ERR_OK) {
		close((FILE ref)file, NULL);
		s_free(str);
		ERR_WITH(err, NULL);
	}

	close((FILE ref)file, NULL);

	res = nalloc(sizeof(char), s_len(str)+1, &err);
	if (err != ERR_OK) {
		s_free(str);
		ERR_WITH(err, NULL);
	}
	memmove(res, s_to_c(str), sizeof(char) * (s_len(str)+1));

	free(str);

	return res;
}
