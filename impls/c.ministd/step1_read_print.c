#include <ministd.h>
#include <ministd_error.h>
#include <ministd_fmt.h>
#include <ministd_memory.h>

#include "values.h"
#include "reader.h"
#include "printer.h"

/* `read` is a function in `ministd_fmt.h`...
 * I should either add optional prefixes to all ministd functions,
 * or I should just change read/write/close/etc
 * to file_read/file_write/file_close etc...
 */
static const value_t own
READ(const char ref line)
{
	err_t err = ERR_OK;
	const value_t own val;

	val = read_str(line, &err);
	if (err != ERR_OK) {
		perror(err, "Encountered error while parsing input");
		return NULL;
	}

	return val;
}

static const value_t own
EVAL(const value_t own processed)
{
	return processed;
}

static const char own
PRINT(const value_t own value)
{
	err_t err = ERR_OK;
	const char own str;

	str = pr_str(value, &err);
	value_free(value);
	if (err != ERR_OK) {
		perror(err, "Encountered error while printing value");
		return NULL;
	}

	return str;
}

static const char own
rep(const char ref line)
{
	const value_t own read_res;
	const value_t own eval_res;
	const char own print_res;

	read_res = READ(line);
	eval_res = EVAL(read_res);
	print_res = PRINT(eval_res);

	return print_res;
}

#define LINECAP (16 * 1024)
static char linebuf[LINECAP];

int
main(void)
{
	const char own out;

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
		out = rep(linebuf);
		prints(out, NULL);
		printc('\n', NULL);
		free((ptr)out);
	}

	return 0;
}
