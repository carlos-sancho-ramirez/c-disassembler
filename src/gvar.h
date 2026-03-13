#ifndef _GLOBAL_VARIABLE_H_
#define _GLOBAL_VARIABLE_H_

#define GVAR_TYPE_BYTE 1
#define GVAR_TYPE_WORD 2
#define GVAR_TYPE_ARRAY 4

/* Raw array of characters without any charater as end */
#define GVAR_TYPE_BYTE_STRING (GVAR_TYPE_BYTE | GVAR_TYPE_ARRAY)
#define GVAR_TYPE_WORD_STRING (GVAR_TYPE_WORD | GVAR_TYPE_ARRAY)
#define GVAR_TYPE_DOLLAR_TERMINATED_STRING 7
#define GVAR_TYPE_FAR_POINTER 9

struct GlobalVariable {
	const char *start;
	const char *end;
	unsigned int relative_address;
	unsigned int var_type;
};

const char *get_gvar_start(const struct GlobalVariable *var);
const char *get_gvar_end(const struct GlobalVariable *var);
unsigned int get_gvar_size(const struct GlobalVariable *var);
unsigned int get_gvar_relative_address(const struct GlobalVariable *var);
unsigned int get_gvar_type(const struct GlobalVariable *var);

void set_gvar_start(struct GlobalVariable *var, const char *start);
void set_gvar_end(struct GlobalVariable *var, const char *end);
void set_gvar_relative_address(struct GlobalVariable *var, unsigned int relative_address);
void set_gvar_type(struct GlobalVariable *var, unsigned int type);

#endif /* _GLOBAL_VARIABLE_H_ */
