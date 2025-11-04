#ifndef _GLOBAL_VARIABLE_H_
#define _GLOBAL_VARIABLE_H_

#define GVAR_TYPE_BYTE 1
#define GVAR_TYPE_WORD 2

/* Raw array of characters without any charater as end */
#define GVAR_TYPE_STRING 5
#define GVAR_TYPE_DOLLAR_TERMINATED_STRING 6
#define GVAR_TYPE_FAR_POINTER 9

struct GlobalVariable {
	const char *start;
	const char *end;
	unsigned int relative_address;
	unsigned int var_type;
};

#endif /* _GLOBAL_VARIABLE_H_ */
