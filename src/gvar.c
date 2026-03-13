#include "gvar.h"

const char *get_gvar_start(const struct GlobalVariable *var) {
	return var->start;
}

const char *get_gvar_end(const struct GlobalVariable *var) {
	return var->end;
}

unsigned int get_gvar_size(const struct GlobalVariable *var) {
	return var->end - var->start;
}

void set_gvar_start(struct GlobalVariable *var, const char *start) {
	var->start = start;
}

void set_gvar_end(struct GlobalVariable *var, const char *end) {
	var->end = end;
}
