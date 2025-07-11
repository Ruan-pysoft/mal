#ifndef TYPES_H
#define TYPES_H

#include <ministd_error.h>
#include <ministd_types.h>

enum value_type {
	VT_LIST,
	VT_NUM,
	VT_SYM

	/* VT_BOOL */
	/* VT_STR */
	/* VT_KEY */
};

struct value_t;

/* NOTE: Here I'm implementing reference counting to keep track of memory.
 * A reference-counted pointer will still be an `own` pointer,
 * where a `ref` refers to borrowed memory
 * (ie. not responsible for cleanup)
 * and an `own` refers to owned memory
 * (ie. responsible for cleanup).
 * All `own` pointers should eventually be freed,
 * and in this case the "free" functions will decrement the ref count
 * and then only actually free the memory when the ref count hits zero.
 *
 * I have also decided to create a "copy" function to increment the ref count.
 *
 * Lastly, while most types are *technically* mutable
 * (because of the ref count),
 * they are all *conceptually* constant,
 * so I will mark them const.
 */

struct cell {
	const struct value_t own val;
	const struct cell own next;
	usz ref_count;
};
const struct cell own cell_new(const struct value_t ref val,
			       err_t ref err_out);
const struct cell own cell_new_own(const struct value_t ref val,
				   err_t ref err_out);
const struct cell own cell_cons(const struct value_t ref val,
				const struct cell ref rest, err_t ref err_out);
const struct cell own cell_cons_own(const struct value_t ref val,
				    const struct cell ref rest,
				    err_t ref err_out);
const struct cell own cell_copy(const struct cell ref this);
void cell_free(const struct cell own this);
const struct cell ref cell_tail(const struct cell ref this);
void _cell_unsafe_append(struct cell ref this, const struct value_t own val,
			 err_t ref err_out);

struct mal_string {
	const char own data;
	usz ref_count;
};
const struct mal_string own mal_string_new(const char ref cstr,
					   err_t ref err_out);
const struct mal_string own mal_string_newn(const char ref cstr, usz len,
					    err_t ref err_out);
const struct mal_string own mal_string_copy(const struct mal_string ref this);
void mal_string_free(const struct mal_string own this);

typedef struct value_t {
	enum value_type type;
	union value_union {
		const struct cell own ls;
		int num;
		const struct mal_string own sym;
	} v;
	usz ref_count;
} value_t;
const value_t own value_list_own(const struct cell own cell, err_t ref err_out);
const value_t own value_sym_own(const struct mal_string own sym,
				err_t ref err_out);
const value_t own value_list(const struct cell ref cell, err_t ref err_out);
const value_t own value_num(int num, err_t ref err_out);
const value_t own value_sym(const struct mal_string ref sym, err_t ref err_out);
const value_t own value_copy(const value_t ref this);
void value_free(const value_t own this);

#endif /* TYPES_H */
