#ifndef ERROR_H
#define ERROR_H

#include <ministd_error.h>
#include <ministd_io.h>

enum perr_type {
	PT_OK,
	PT_ERRT,
	PT_EOF
};

typedef struct perr_t {
	enum perr_type type;
	union perr_union {
		err_t errt;
		const char ref eof;
	} e;
} perr_t;

void perr_display(const perr_t ref this, FILE ref file, err_t ref err_out);

extern const perr_t PERR_OK;

#define PERR_WITH(perr, val) do { \
		if (err_out != NULL && err_out->type == PT_OK) { \
			*err_out = (perr); \
		} \
		return (val); \
	} while (0)
#define PERR_VOID(perr) do { \
		if (err_out != NULL && err_out->type == PT_OK) { \
			*err_out = (perr); \
		} \
		return; \
	} while (0)
#define PERR_FROM_WITH(err, val) do { \
		if (err_out != NULL && err_out->type == PT_OK) { \
			err_out->type = PT_ERRT; \
			err_out->e.errt = (err); \
		} \
		return (val); \
	} while (0)
#define PERR_FROM_VOID(err) do { \
		if (err_out != NULL && err_out->type == PT_OK) { \
			err_out->type = PT_ERRT; \
			err_out->e.errt = (err); \
		} \
		return; \
	} while (0)
#define PTRY_WITH(perr, val) do { \
		if ((perr).type != PT_OK) { \
			PERR_WITH(perr, val); \
		} \
	} while (0)
#define PTRY_VOID(perr) do { \
		if ((perr).type != PT_OK) { \
			PERR_VOID(perr); \
		} \
	} while (0)
#define PTRY_FROM_WITH(err, val) do { \
		if (err != ERR_OK) { \
			PERR_FROM_WITH(err, val); \
		} \
	} while (0)
#define PTRY_FROM_VOID(err) do { \
		if (err != ERR_OK) { \
			PERR_FROM_VOID(err); \
		} \
	} while (0)

#endif /* ERROR_H */
