#ifndef _GLOBAL_VARIABLE_H_
#define _GLOBAL_VARIABLE_H_

#define GLOBAL_VARIABLE_TYPE_BYTE 1
#define GLOBAL_VARIABLE_TYPE_WORD 2

/* Raw array of characters without any charater as end */
#define GLOBAL_VARIABLE_TYPE_STRING 5
#define GLOBAL_VARIABLE_TYPE_DOLLAR_TERMINATED_STRING 6
#define GLOBAL_VARIABLE_TYPE_FAR_POINTER 9

struct GlobalVariable {
	const char *start;
	const char *end;
	unsigned int relative_address;
	unsigned int var_type;
};

#endif /* _GLOBAL_VARIABLE_H_ */
