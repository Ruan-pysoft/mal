#include <ministd.h>
#include <ministd_fmt.h>

/* `read` is a function in `ministd_fmt.h`...
 * I should either add optional prefixes to all ministd functions,
 * or I should just change read/write/close/etc
 * to file_read/file_write/file_close etc...
 */
const char ref
READ(const char ref line)
{
	return line;
}

const char ref
EVAL(const char ref processed)
{
	return processed;
}

const char ref
PRINT(const char ref value)
{
	return value;
}

const char ref
rep(const char ref line)
{
	const char ref read_res;
	const char ref eval_res;
	const char ref print_res;

	read_res = READ(line);
	eval_res = EVAL(read_res);
	print_res = PRINT(eval_res);

	return print_res;
}

#define LINECAP (16 * 1024)
char linebuf[LINECAP];

int
main(void)
{
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
		prints(rep(linebuf), NULL);
	}

	return 0;
}
