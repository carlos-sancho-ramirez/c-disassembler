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

unsigned int get_gvar_relative_address(const struct GlobalVariable *var) {
	return var->relative_address;
}

unsigned int get_gvar_type(const struct GlobalVariable *var) {
	return var->var_type;
}

void set_gvar_start(struct GlobalVariable *var, const char *start) {
	var->start = start;
}

void set_gvar_end(struct GlobalVariable *var, const char *end) {
	var->end = end;
}

void set_gvar_relative_address(struct GlobalVariable *var, unsigned int relative_address) {
	var->relative_address = relative_address;
}

void set_gvar_type(struct GlobalVariable *var, unsigned int type) {
	var->var_type = type;
}
