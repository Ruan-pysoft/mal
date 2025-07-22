#ifndef ERROR_H
#define ERROR_H

#include <ministd_error.h>
#include <ministd_io.h>

typedef struct perr_t {
	bool is_errt;
	union perr_union {
		err_t errt;
		const char own msg;
	} e;
} perr_t;

void perr_display(const perr_t ref this, FILE ref file, err_t ref err_out);
void perr_deinit(perr_t this);
bool perr_is_ok(perr_t this);
perr_t perr_eof(const char ref doing, const char ref expected);

extern const perr_t PERR_OK;

#define PERR_WITH(perr, val) do { \
		if (err_out != NULL && perr_is_ok(*err_out)) { \
			*err_out = (perr); \
		} \
		return (val); \
	} while (0)
#define PERR_VOID(perr) do { \
		if (err_out != NULL && perr_is_ok(*err_out)) { \
			*err_out = (perr); \
		} \
		return; \
	} while (0)
#define PERR_FROM_WITH(err, val) do { \
		if (err_out != NULL && perr_is_ok(*err_out)) { \
			err_out->is_errt = true; \
			err_out->e.errt = (err); \
		} \
		return (val); \
	} while (0)
#define PERR_FROM_VOID(err) do { \
		if (err_out != NULL && perr_is_ok(*err_out)) { \
			err_out->is_errt = true; \
			err_out->e.errt = (err); \
		} \
		return; \
	} while (0)
#define PTRY_WITH(perr, val) do { \
		if (!perr_is_ok(perr)) { \
			PERR_WITH(perr, val); \
		} \
	} while (0)
#define PTRY_VOID(perr) do { \
		if (!perr_is_ok(perr)) { \
			PERR_VOID(perr); \
		} \
	} while (0)
#define PTRY_FROM_WITH(err, val) do { \
		if ((err) != ERR_OK) { \
			PERR_FROM_WITH(err, val); \
		} \
	} while (0)
#define PTRY_FROM_VOID(err) do { \
		if ((err) != ERR_OK) { \
			PERR_FROM_VOID(err); \
		} \
	} while (0)

typedef struct rerr_t {
	bool is_errt;
	union rerr_union {
		err_t errt;
		const char own msg;
	} e;
} rerr_t;

void rerr_display(const rerr_t ref this, FILE ref file, err_t ref err_out);
void rerr_deinit(rerr_t this);
bool rerr_is_ok(rerr_t this);
rerr_t rerr_arg_len_mismatch(usz expected, usz got);
rerr_t rerr_arg_vararg_mismatch(usz minimum, usz got);

extern const rerr_t RERR_OK;

#define RERR_WITH(rerr, val) do { \
		if (err_out != NULL && rerr_is_ok(*err_out)) { \
			*err_out = (rerr); \
		} \
		return (val); \
	} while (0)
#define RERR_VOID(rerr) do { \
		if (err_out != NULL && rerr_is_ok(*err_out)) { \
			*err_out = (rerr); \
		} \
		return; \
	} while (0)
#define RERR_FROM_WITH(err, val) do { \
		if (err_out != NULL && rerr_is_ok(*err_out)) { \
			err_out->is_errt = true; \
			err_out->e.errt = (err); \
		} \
		return (val); \
	} while (0)
#define RERR_FROM_VOID(err) do { \
		if (err_out != NULL && rerr_is_ok(*err_out)) { \
			err_out->is_errt = true; \
			err_out->e.errt = (err); \
		} \
		return; \
	} while (0)
#define RTRY_WITH(rerr, val) do { \
		if (!rerr_is_ok(rerr)) { \
			RERR_WITH(rerr, val); \
		} \
	} while (0)
#define RTRY_VOID(rerr) do { \
		if (!rerr_is_ok(rerr)) { \
			RERR_VOID(rerr); \
		} \
	} while (0)
#define RTRY_FROM_WITH(err, val) do { \
		if ((err) != ERR_OK) { \
			RERR_FROM_WITH(err, val); \
		} \
	} while (0)
#define RTRY_FROM_VOID(err) do { \
		if ((err) != ERR_OK) { \
			RERR_FROM_VOID(err); \
		} \
	} while (0)

#endif /* ERROR_H */
