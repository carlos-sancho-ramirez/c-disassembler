#ifndef _FAR_POINTER_H_
#define _FAR_POINTER_H_

#include <stdint.h>

struct FarPointer {
	uint16_t offset;
	uint16_t segment;
};

#endif /* _FAR_POINTER_H_ */
