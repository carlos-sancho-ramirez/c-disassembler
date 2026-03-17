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

	/**
	 * Pointer to the first position just after the end of this variable.
	 * This is only relevant for array types. As the size is known already for all other types.
	 * This value will match the value at start if the end is still unknown, which is common in arrays.
	 */
	const char *end;
	unsigned int relative_address;
	unsigned int var_type;
};

/**
 * Initialize the GlobalVariable structure with the given start, relative address and type.
 *
 * In case of providing an array type, this method will initialize the variable with unknown length.
 * Whenever its end/length is known, set_gvar_end or set_gvar_length should be called to define it.
 * If the type is not an array, the length is already known according to its type.
 *
 * This will overwrite any value in the given struct.
 */
void initialize_gvar(struct GlobalVariable *var, const char *start, unsigned int relative_address, unsigned int type);

const char *get_gvar_start(const struct GlobalVariable *var);
const char *get_gvar_end(const struct GlobalVariable *var);
unsigned int get_gvar_size(const struct GlobalVariable *var);
unsigned int get_gvar_relative_address(const struct GlobalVariable *var);
unsigned int get_gvar_type(const struct GlobalVariable *var);

/**
 * Changes the current variable to to the given one.
 *
 * This method will also update its corresponding size. In case the given type is an array, the
 * size will become unknown. Clients of this method should call set_gvar_end or set_gvar_length
 * whenever its size is known.
 */
void set_gvar_type(struct GlobalVariable *var, unsigned int type);
void set_gvar_end(struct GlobalVariable *var, const char *end);
void set_gvar_length(struct GlobalVariable *var, unsigned int length);

#endif /* _GLOBAL_VARIABLE_H_ */
