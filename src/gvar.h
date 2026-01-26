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

#endif /* _GLOBAL_VARIABLE_H_ */
