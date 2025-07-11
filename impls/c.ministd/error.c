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
