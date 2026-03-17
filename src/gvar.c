#include "gvar.h"
#include <assert.h>

void initialize_gvar(struct GlobalVariable *var, const char *start, unsigned int relative_address, unsigned int type) {
	var->start = start;
	var->end = start;
	var->relative_address = relative_address;
	var->var_type = type;
}

const char *get_gvar_start(const struct GlobalVariable *var) {
	return var->start;
}

const char *get_gvar_end(const struct GlobalVariable *var) {
	return (var->var_type & GVAR_TYPE_ARRAY)? var->end : var->start + get_gvar_size(var);
}

unsigned int get_gvar_size(const struct GlobalVariable *var) {
	if (var->var_type & GVAR_TYPE_ARRAY) {
		return var->end - var->start;
	}
	else if (var->var_type == GVAR_TYPE_BYTE) {
		return 1;
	}
	else if (var->var_type == GVAR_TYPE_WORD) {
		return 2;
	}
	else if (var->var_type == GVAR_TYPE_FAR_POINTER) {
		return 4;
	}
	else {
		assert(0);
	}
}

unsigned int get_gvar_relative_address(const struct GlobalVariable *var) {
	return var->relative_address;
}

unsigned int get_gvar_type(const struct GlobalVariable *var) {
	return var->var_type;
}

void set_gvar_end(struct GlobalVariable *var, const char *end) {
	assert(var->var_type & GVAR_TYPE_ARRAY && end > var->start);
	var->end = end;
}

void set_gvar_length(struct GlobalVariable *var, unsigned int length) {
	assert(var->var_type & GVAR_TYPE_ARRAY);
	var->end = var->start + length;
}

void set_gvar_type(struct GlobalVariable *var, unsigned int type) {
	var->end = var->start;
	var->var_type = type;
}
