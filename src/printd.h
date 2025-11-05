#ifndef _PRINT_DEBUG_H_
#define _PRINT_DEBUG_H_

#include "register.h"
#include "stack.h"
#include "gvwvmap.h"
#include "itable.h"

void print_regs(struct Registers *regs);
void print_stack(struct Stack *stack);
void print_gvwvmap(const struct GlobalVariableWordValueMap *map, const char *buffer);
void print_itable(struct InterruptionTable *table);

#endif /* _PRINT_DEBUG_H_ */
