#include "error.h"

#include <ministd_fmt.h>

const perr_t PERR_OK = {
	PT_OK,
	{ 0 },
};

void
perr_display(const perr_t ref this, FILE ref file, err_t ref err_out)
{
	err_t err = ERR_OK;

	switch (this->type) {
		case PT_OK: {
			fprints("No error has occurred", file, err_out);
		break; }
		case PT_ERRT: {
			fprints("Internal error occurred: ", file, &err);
			TRY_VOID(err);
			fprints(err_str(this->e.errt), file, err_out);
		break; }
		case PT_EOF: {
			fprints("Hit EOF while parsing", file, &err);
			TRY_VOID(err);
			fprints(this->e.eof, file, err_out);
		break; }
	}
}

const rerr_t RERR_OK = {
	RT_OK,
	{ 0 },
};

void
rerr_display(const rerr_t ref this, FILE ref file, err_t ref err_out)
{
	err_t err = ERR_OK;

	switch (this->type) {
		case RT_OK: {
			fprints("No error has occurred", file, err_out);
		break; }
		case RT_ERRT: {
			fprints("Internal error occurred: ", file, &err);
			TRY_VOID(err);
			fprints(err_str(this->e.errt), file, err_out);
		break; }
		case RT_NOT_FOUND: {
			fprints("Identifier not found", file, &err);
			TRY_VOID(err);
			fprints(this->e.msg, file, err_out);
		break; }
		case RT_UNCALLABLE: {
			fprints("Value is not callable", file, &err);
			TRY_VOID(err);
			fprints(this->e.msg, file, err_out);
		break; }
		case RT_ARG_MISMATCH: {
			fprints("Incorrect arguments", file, &err);
			TRY_VOID(err);
			fprints(this->e.msg, file, err_out);
		break; }
	}
}
