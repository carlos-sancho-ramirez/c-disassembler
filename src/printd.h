#ifndef _PRINT_DEBUG_H_
#define _PRINT_DEBUG_H_

#ifdef DEBUG

#include "register.h"
#include "stack.h"
#include "gvwvmap.h"
#include "itable.h"
#include "cblist.h"

#include <stdio.h>

#define DEBUG_PRINT0(msg) fprintf(stderr, msg)
#define DEBUG_PRINT1(msg, arg1) fprintf(stderr, msg, arg1)
#define DEBUG_PRINT2(msg, arg1, arg2) fprintf(stderr, msg, arg1, arg2)
#define DEBUG_PRINT3(msg, arg1, arg2, arg3) fprintf(stderr, msg, arg1, arg2, arg3)
#define DEBUG_INDENTED_PRINT0(depth, msg) \
if (1) { \
	int debug_indentation_index; \
	for (debug_indentation_index = 0; debug_indentation_index < depth; debug_indentation_index++) { \
		fprintf(stderr, " "); \
	} \
} \
DEBUG_PRINT0(msg)

#define DEBUG_INDENTED_PRINT1(depth, msg, arg1) \
if (1) { \
	int debug_indentation_index; \
	for (debug_indentation_index = 0; debug_indentation_index < depth; debug_indentation_index++) { \
		fprintf(stderr, " "); \
	} \
} \
DEBUG_PRINT1(msg, arg1)

#define DEBUG_INDENTED_PRINT2(depth, msg, arg1, arg2) \
if (1) { \
	int debug_indentation_index; \
	for (debug_indentation_index = 0; debug_indentation_index < depth; debug_indentation_index++) { \
		fprintf(stderr, " "); \
	} \
} \
DEBUG_PRINT2(msg, arg1, arg2)

#define DEBUG_INDENTED_PRINT3(depth, msg, arg1, arg2, arg3) \
if (1) { \
	int debug_indentation_index; \
	for (debug_indentation_index = 0; debug_indentation_index < depth; debug_indentation_index++) { \
		fprintf(stderr, " "); \
	} \
} \
DEBUG_PRINT3(msg, arg1, arg2, arg3)

#define DEBUG_PRINT_STATE(ip, regs, stack, map, buffer, table) \
DEBUG_PRINT1("  IP=%x", ip); \
print_regs(regs); \
fprintf(stderr, "\n  "); \
print_stack(stack); \
fprintf(stderr, "\n  "); \
print_gvwvmap(map, buffer); \
fprintf(stderr, "\n  "); \
print_itable(table); \
fprintf(stderr, "\n")

#define DEBUG_CBLIST(list) print_cblist(list)

#define WARN_PRINT0(msg) fprintf(stderr, "Warning: " msg)

#else /* DEBUG */

#define DEBUG_PRINT0(msg)
#define DEBUG_PRINT1(msg, arg1)
#define DEBUG_PRINT2(msg, arg1, arg2)
#define DEBUG_PRINT3(msg, arg1, arg2, arg3)
#define DEBUG_INDENTED_PRINT0(depth, msg)
#define DEBUG_INDENTED_PRINT1(depth, msg, arg1)
#define DEBUG_INDENTED_PRINT2(depth, msg, arg1, arg2)
#define DEBUG_INDENTED_PRINT3(depth, msg, arg1, arg2, arg3)
#define DEBUG_PRINT_STATE(ip, regs, stack, map, buffer, table)
#define DEBUG_CBLIST(list)

#define WARN_PRINT0(msg)

#endif /* DEBUG */
#endif /* _PRINT_DEBUG_H_ */
