#include "gvar.h"
#include <assert.h>

void initialize_gvar(struct GlobalVariable *var, const char *start, unsigned int length, unsigned int relative_address, unsigned int type) {
	var->start = start;
	var->end = start + length;
	var->relative_address = relative_address;
	var->var_type = type;
}

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

void set_gvar_end(struct GlobalVariable *var, const char *end) {
	assert(end > var->start);
	var->end = end;
}

void set_gvar_length_and_type(struct GlobalVariable *var, unsigned int length, unsigned int type) {
	var->end = var->start + length;
	var->var_type = type;
}
