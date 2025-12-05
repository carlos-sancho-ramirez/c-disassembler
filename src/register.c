#include "register.h"
#include <assert.h>
#include <stdlib.h>

int is_register_al_defined(const struct Registers *regs) {
	return regs->defined & 0x0001;
}

int is_register_cl_defined(const struct Registers *regs) {
	return regs->defined & 0x0004;
}

int is_register_dl_defined(const struct Registers *regs) {
	return regs->defined & 0x0010;
}

int is_register_bl_defined(const struct Registers *regs) {
	return regs->defined & 0x0040;
}

int is_register_ah_defined(const struct Registers *regs) {
	return regs->defined & 0x0002;
}

int is_register_ch_defined(const struct Registers *regs) {
	return regs->defined & 0x0008;
}

int is_register_dh_defined(const struct Registers *regs) {
	return regs->defined & 0x0020;
}

int is_register_bh_defined(const struct Registers *regs) {
	return regs->defined & 0x0080;
}

int is_byte_register_defined(const struct Registers * regs, unsigned int index) {
	assert(index < 8);
	return (index < 4)?
			regs->defined & (1 << (index * 2)) :
			regs->defined & (2 << ((index - 4) * 2));
}

int is_register_ax_defined(const struct Registers *regs) {
	return (regs->defined & 0x0003) == 0x0003;
}

int is_register_cx_defined(const struct Registers *regs) {
	return (regs->defined & 0x000C) == 0x000C;
}

int is_register_dx_defined(const struct Registers *regs) {
	return (regs->defined & 0x0030) == 0x0030;
}

int is_register_bx_defined(const struct Registers *regs) {
	return (regs->defined & 0x00C0) == 0x00C0;
}

int is_register_sp_defined(const struct Registers *regs) {
	return regs->defined & 0x0100;
}

int is_register_bp_defined(const struct Registers *regs) {
	return regs->defined & 0x0200;
}

int is_register_si_defined(const struct Registers *regs) {
	return regs->defined & 0x0400;
}

int is_register_di_defined(const struct Registers *regs) {
	return regs->defined & 0x0800;
}

int is_register_ax_defined_absolute(const struct Registers *regs) {
	return (regs->relative & 0x0010) == 0 && is_register_ax_defined(regs);
}

int is_register_cx_defined_absolute(const struct Registers *regs) {
	return (regs->relative & 0x0020) == 0 && is_register_cx_defined(regs);
}

int is_register_dx_defined_absolute(const struct Registers *regs) {
	return (regs->relative & 0x0040) == 0 && is_register_dx_defined(regs);
}

int is_register_bx_defined_absolute(const struct Registers *regs) {
	return (regs->relative & 0x0080) == 0 && is_register_bx_defined(regs);
}

int is_register_sp_defined_absolute(const struct Registers *regs) {
	return (regs->relative & 0x0100) == 0 && is_register_sp_defined(regs);
}

int is_register_bp_defined_absolute(const struct Registers *regs) {
	return (regs->relative & 0x0200) == 0 && is_register_bp_defined(regs);
}

int is_register_si_defined_absolute(const struct Registers *regs) {
	return (regs->relative & 0x0400) == 0 && is_register_si_defined(regs);
}

int is_register_di_defined_absolute(const struct Registers *regs) {
	return (regs->relative & 0x0800) == 0 && is_register_si_defined(regs);
}

int is_register_ax_defined_relative(const struct Registers *regs) {
	return regs->relative & 0x0010 && is_register_ax_defined(regs);
}

int is_register_cx_defined_relative(const struct Registers *regs) {
	return regs->relative & 0x0020 && is_register_cx_defined(regs);
}

int is_register_dx_defined_relative(const struct Registers *regs) {
	return regs->relative & 0x0040 && is_register_dx_defined(regs);
}

int is_register_bx_defined_relative(const struct Registers *regs) {
	return regs->relative & 0x0080 && is_register_bx_defined(regs);
}

int is_register_sp_defined_relative(const struct Registers *regs) {
	return regs->relative & 0x0100 && is_register_sp_defined(regs);
}

int is_register_bp_defined_relative(const struct Registers *regs) {
	return regs->relative & 0x0200 && is_register_bp_defined(regs);
}

int is_register_si_defined_relative(const struct Registers *regs) {
	return regs->relative & 0x0400 && is_register_si_defined(regs);
}

int is_register_di_defined_relative(const struct Registers *regs) {
	return regs->relative & 0x0800 && is_register_si_defined(regs);
}

int is_word_register_defined(const struct Registers *regs, unsigned int index) {
	assert(index < 8);
	if (index < 4) {
		const uint16_t mask = 3 << index * 2;
		return (regs->defined & mask) == mask;
	}
	else {
		return regs->defined & 0x10 << index;
	}
}

int is_word_register_defined_relative(const struct Registers *regs, unsigned int index) {
	assert(index < 8);
	return is_word_register_defined(regs, index) && regs->relative & (0x10 << index);
}

int is_register_es_defined(const struct Registers *regs) {
	return regs->defined & 0x1000;
}

int is_register_cs_defined(const struct Registers *regs) {
	return regs->defined & 0x2000;
}

int is_register_ss_defined(const struct Registers *regs) {
	return regs->defined & 0x4000;
}

int is_register_ds_defined(const struct Registers *regs) {
	return regs->defined & 0x8000;
}

int is_segment_register_defined(const struct Registers *regs, unsigned int index) {
	assert(index < 4);
	return regs->defined & 0x1000 << index;
}

int is_segment_register_defined_absolute(const struct Registers *regs, unsigned int index) {
	assert(index < 4);
	return (regs->relative & 0x1000 << index) == 0 && is_segment_register_defined(regs, index);
}

int is_register_es_defined_relative(const struct Registers *regs) {
	return regs->relative & 0x1000 && is_register_es_defined(regs);
}

int is_register_cs_defined_relative(const struct Registers *regs) {
	return regs->relative & 0x2000 && is_register_cs_defined(regs);
}

int is_register_ss_defined_relative(const struct Registers *regs) {
	return regs->relative & 0x4000 && is_register_ss_defined(regs);
}

int is_register_ds_defined_relative(const struct Registers *regs) {
	return regs->relative & 0x8000 && is_register_ds_defined(regs);
}

int is_segment_register_defined_relative(const struct Registers *regs, unsigned int index) {
	assert(index < 4);
	return regs->relative & 0x1000 << index && is_segment_register_defined(regs, index);
}

const char *get_register_ax_value_origin(const struct Registers *regs) {
	const char *value_origin = regs->value_origin[0];
	return (value_origin == regs->value_origin[1])? value_origin : NULL;
}

const char *get_register_dx_value_origin(const struct Registers *regs) {
	const char *value_origin = regs->value_origin[4];
	return (value_origin == regs->value_origin[5])? value_origin : NULL;
}

const char *get_word_register_value_origin(const struct Registers *regs, unsigned int index) {
	assert(index < 8);
	if (index < 4) {
		const char *value_origin = regs->value_origin[index * 2];
		return (value_origin == regs->value_origin[index * 2 + 1])? value_origin : NULL;
	}
	else {
		return regs->value_origin[index + 4];
	}
}

const char *get_segment_register_value_origin(const struct Registers *regs, unsigned int index) {
	assert(index < 4);
	return regs->value_origin[index + 12];
}

unsigned int get_register_al(const struct Registers *regs) {
	return regs->al;
}

unsigned int get_register_ah(const struct Registers *regs) {
	return regs->ah;
}

unsigned int get_register_cl(const struct Registers *regs) {
	return regs->cl;
}

unsigned int get_register_ch(const struct Registers *regs) {
	return regs->ch;
}

unsigned int get_register_dl(const struct Registers *regs) {
	return regs->dl;
}

unsigned int get_register_dh(const struct Registers *regs) {
	return regs->dh;
}

unsigned int get_register_bl(const struct Registers *regs) {
	return regs->bl;
}

unsigned int get_register_bh(const struct Registers *regs) {
	return regs->bh;
}

unsigned int get_register_ax(const struct Registers *regs) {
	return (regs->ah << 8) + regs->al;
}

unsigned int get_register_cx(const struct Registers *regs) {
	return (regs->ch << 8) + regs->cl;
}

unsigned int get_register_dx(const struct Registers *regs) {
	return (regs->dh << 8) + regs->dl;
}

unsigned int get_register_bx(const struct Registers *regs) {
	return (regs->bh << 8) + regs->bl;
}

unsigned int get_register_sp(const struct Registers *regs) {
	return regs->sp;
}

unsigned int get_register_bp(const struct Registers *regs) {
	return regs->bp;
}

unsigned int get_register_si(const struct Registers *regs) {
	return regs->si;
}

unsigned int get_register_di(const struct Registers *regs) {
	return regs->di;
}

unsigned int get_register_es(const struct Registers *regs) {
	return regs->es;
}

unsigned int get_register_cs(const struct Registers *regs) {
	return regs->cs;
}

unsigned int get_register_ss(const struct Registers *regs) {
	return regs->ss;
}

unsigned int get_register_ds(const struct Registers *regs) {
	return regs->ds;
}

unsigned int get_byte_register(const struct Registers *regs, unsigned int index) {
	assert(index < 8);

	if (index == 0) {
		return regs->al;
	}
	else if (index == 1) {
		return regs->cl;
	}
	else if (index == 2) {
		return regs->dl;
	}
	else if (index == 3) {
		return regs->bl;
	}
	else if (index == 4) {
		return regs->ah;
	}
	else if (index == 5) {
		return regs->ch;
	}
	else if (index == 6) {
		return regs->dh;
	}
	else {
		/* Assuming index == 7 */
		return regs->bh;
	}
}

unsigned int get_word_register(const struct Registers *regs, unsigned int index) {
	assert(index < 8);

	if (index == 0) {
		return get_register_ax(regs);
	}
	else if (index == 1) {
		return get_register_cx(regs);
	}
	else if (index == 2) {
		return get_register_dx(regs);
	}
	else if (index == 3) {
		return get_register_bx(regs);
	}
	else if (index == 4) {
		return regs->sp;
	}
	else if (index == 5) {
		return regs->bp;
	}
	else if (index == 6) {
		return regs->si;
	}
	else {
		/* Assuming index == 7 */
		return regs->di;
	}
}

unsigned int get_segment_register(const struct Registers *regs, unsigned int index) {
	assert(index < 4);

	if (index == 0) {
		return get_register_es(regs);
	}
	else if (index == 1) {
		return get_register_cs(regs);
	}
	else if (index == 2) {
		return get_register_ss(regs);
	}
	else {
		/* Assuming index == 3 */
		return get_register_ds(regs);
	}
}

void set_byte_register(struct Registers *regs, unsigned int index, const char *last_update, const char *value_origin, unsigned char value) {
	assert(index < 8);

	/*
		This should be optimised using the regs pointer plus index to determinate the byte within
		the struct to change. But for now I implement it in a way that I can ensure it works in any
		architecture.
	*/

	if (index == 0) {
		regs->al = value;
		regs->last_update[0] = last_update;
		regs->value_origin[0] = value_origin;
		regs->defined |= 0x01;
	}
	else if (index == 1) {
		regs->cl = value;
		regs->last_update[2] = last_update;
		regs->value_origin[2] = value_origin;
		regs->defined |= 0x04;
	}
	else if (index == 2) {
		regs->dl = value;
		regs->last_update[4] = last_update;
		regs->value_origin[4] = value_origin;
		regs->defined |= 0x10;
	}
	else if (index == 3) {
		regs->bl = value;
		regs->last_update[6] = last_update;
		regs->value_origin[6] = value_origin;
		regs->defined |= 0x40;
	}
	else if (index == 4) {
		regs->ah = value;
		regs->last_update[1] = last_update;
		regs->value_origin[1] = value_origin;
		regs->defined |= 0x02;
	}
	else if (index == 5) {
		regs->ch = value;
		regs->last_update[3] = last_update;
		regs->value_origin[3] = value_origin;
		regs->defined |= 0x06;
	}
	else if (index == 6) {
		regs->dh = value;
		regs->last_update[5] = last_update;
		regs->value_origin[5] = value_origin;
		regs->defined |= 0x20;
	}
	else {
		/* Assuming index == 7 */
		regs->bh = value;
		regs->last_update[7] = last_update;
		regs->value_origin[7] = value_origin;
		regs->defined |= 0x60;
	}
}

void set_word_register(struct Registers *regs, unsigned int index, const char *last_update, const char *value_origin, uint16_t value) {
	assert(index < 8);

	/*
		This should be optimised using the regs pointer plus index to determinate the byte within
		the struct to change. But for now I implement it in a way that I can ensure it works in any
		architecture.
	*/

	if (index == 0) {
		regs->al = value & 0xFF;
		regs->ah = (value >> 8) & 0xFF;
		regs->last_update[0] = last_update;
		regs->last_update[1] = last_update;
		regs->value_origin[0] = value_origin;
		regs->value_origin[1] = value_origin;
		regs->defined |= 0x0003;
		regs->relative &= ~0x10;
	}
	else if (index == 1) {
		regs->cl = value & 0xFF;
		regs->ch = (value >> 8) & 0xFF;
		regs->last_update[2] = last_update;
		regs->last_update[3] = last_update;
		regs->value_origin[2] = value_origin;
		regs->value_origin[3] = value_origin;
		regs->defined |= 0x000C;
		regs->relative &= ~0x20;
	}
	else if (index == 2) {
		regs->dl = value & 0xFF;
		regs->dh = (value >> 8) & 0xFF;
		regs->last_update[4] = last_update;
		regs->last_update[5] = last_update;
		regs->value_origin[4] = value_origin;
		regs->value_origin[5] = value_origin;
		regs->defined |= 0x0030;
		regs->relative &= ~0x40;
	}
	else if (index == 3) {
		regs->bl = value & 0xFF;
		regs->bh = (value >> 8) & 0xFF;
		regs->last_update[6] = last_update;
		regs->last_update[7] = last_update;
		regs->value_origin[6] = value_origin;
		regs->value_origin[7] = value_origin;
		regs->defined |= 0x00C0;
		regs->relative &= ~0x80;
	}
	else if (index == 4) {
		regs->sp = value;
		regs->last_update[8] = last_update;
		regs->value_origin[8] = value_origin;
		regs->defined |= 0x0100;
		regs->relative &= ~0x100;
	}
	else if (index == 5) {
		regs->bp = value;
		regs->last_update[9] = last_update;
		regs->value_origin[9] = value_origin;
		regs->defined |= 0x0200;
		regs->relative &= ~0x200;
	}
	else if (index == 6) {
		regs->si = value;
		regs->last_update[10] = last_update;
		regs->value_origin[10] = value_origin;
		regs->defined |= 0x0400;
		regs->relative &= ~0x400;
	}
	else {
		/* Assuming index == 7 */
		regs->di = value;
		regs->last_update[11] = last_update;
		regs->value_origin[11] = value_origin;
		regs->defined |= 0x0800;
		regs->relative &= ~0x800;
	}
}

void set_word_register_relative(struct Registers *regs, unsigned int index, const char *last_update, const char *value_origin, uint16_t value) {
	assert(index < 8);

	/*
		This should be optimised using the regs pointer plus index to determinate the byte within
		the struct to change. But for now I implement it in a way that I can ensure it works in any
		architecture.
	*/

	if (index == 0) {
		regs->al = value & 0xFF;
		regs->ah = value >> 8 & 0xFF;
		regs->last_update[0] = last_update;
		regs->last_update[1] = last_update;
		regs->value_origin[0] = value_origin;
		regs->value_origin[1] = value_origin;
		regs->defined |= 0x0003;
		regs->relative |= 0x10;
	}
	else if (index == 1) {
		regs->cl = value & 0xFF;
		regs->ch = value >> 8 & 0xFF;
		regs->last_update[2] = last_update;
		regs->last_update[3] = last_update;
		regs->value_origin[2] = value_origin;
		regs->value_origin[3] = value_origin;
		regs->defined |= 0x000C;
		regs->relative |= 0x20;
	}
	else if (index == 2) {
		regs->dl = value & 0xFF;
		regs->dh = value >> 8 & 0xFF;
		regs->last_update[4] = last_update;
		regs->last_update[5] = last_update;
		regs->value_origin[4] = value_origin;
		regs->value_origin[5] = value_origin;
		regs->defined |= 0x0030;
		regs->relative |= 0x40;
	}
	else if (index == 3) {
		regs->bl = value & 0xFF;
		regs->bh = value >> 8 & 0xFF;
		regs->last_update[6] = last_update;
		regs->last_update[7] = last_update;
		regs->value_origin[6] = value_origin;
		regs->value_origin[7] = value_origin;
		regs->defined |= 0x00C0;
		regs->relative |= 0x80;
	}
	else if (index == 4) {
		regs->sp = value;
		regs->last_update[8] = last_update;
		regs->value_origin[8] = value_origin;
		regs->defined |= 0x0100;
		regs->relative |= 0x100;
	}
	else if (index == 5) {
		regs->bp = value;
		regs->last_update[9] = last_update;
		regs->value_origin[9] = value_origin;
		regs->defined |= 0x0200;
		regs->relative |= 0x200;
	}
	else if (index == 6) {
		regs->si = value;
		regs->last_update[10] = last_update;
		regs->value_origin[10] = value_origin;
		regs->defined |= 0x0400;
		regs->relative |= 0x400;
	}
	else {
		/* Assuming index == 7 */
		regs->di = value;
		regs->last_update[11] = last_update;
		regs->value_origin[11] = value_origin;
		regs->defined |= 0x0800;
		regs->relative |= 0x800;
	}
}

void set_register_ax_undefined(struct Registers *regs, const char *last_update) {
	regs->defined &= 0xFFFC;
	regs->last_update[0] = last_update;
	regs->last_update[1] = last_update;
}

void set_register_cx_undefined(struct Registers *regs, const char *last_update) {
	regs->defined &= 0xFFF3;
	regs->last_update[2] = last_update;
	regs->last_update[3] = last_update;
}

void set_register_dx_undefined(struct Registers *regs, const char *last_update) {
	regs->defined &= 0xFFCF;
	regs->last_update[4] = last_update;
	regs->last_update[5] = last_update;
}

void set_register_bx_undefined(struct Registers *regs, const char *last_update) {
	regs->defined &= 0xFF3F;
	regs->last_update[6] = last_update;
	regs->last_update[7] = last_update;
}

void set_register_es_undefined(struct Registers *regs, const char *last_update) {
	regs->defined &= 0xEFFF;
	regs->last_update[12] = last_update;
}

void set_register_ds_undefined(struct Registers *regs, const char *last_update) {
	regs->defined &= 0x7FFF;
	regs->last_update[15] = last_update;
}

void set_word_register_undefined(struct Registers *regs, unsigned int index, const char *last_update) {
	assert(index < 8);

	if (index < 4) {
		regs->last_update[index * 2] = NULL;
		regs->last_update[index * 2 + 1] = NULL;
	}
	else {
		regs->last_update[index + 4] = NULL;
	}
}

void set_register_al_undefined(struct Registers *regs, const char *last_update) {
	regs->defined &= 0xFFFE;
	regs->last_update[0] = last_update;
}

void set_register_sp(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value) {
	regs->sp = value;
	regs->last_update[8] = last_update;
	regs->value_origin[8] = value_origin;
	regs->defined |= 0x0100;
	regs->relative &= ~0x100;
}

void set_register_es(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value) {
	regs->es = value;
	regs->last_update[12] = last_update;
	regs->value_origin[12] = value_origin;
	regs->defined |= 0x1000;
	regs->relative &= ~0x1000;
}

void set_register_cs(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value) {
	regs->cs = value;
	regs->last_update[13] = last_update;
	regs->value_origin[13] = value_origin;
	regs->defined |= 0x2000;
	regs->relative &= ~0x2000;
}

void set_register_ss(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value) {
	regs->ss = value;
	regs->last_update[14] = last_update;
	regs->value_origin[14] = value_origin;
	regs->defined |= 0x4000;
	regs->relative &= ~0x4000;
}

void set_register_ds(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value) {
	regs->ds = value;
	regs->last_update[15] = last_update;
	regs->value_origin[15] = value_origin;
	regs->defined |= 0x8000;
	regs->relative &= ~0x8000;
}

void set_segment_register(struct Registers *regs, unsigned int index, const char *last_update, const char *value_origin, uint16_t value) {
	assert(index < 4);

	if (index == 0) {
		set_register_es(regs, last_update, value_origin, value);
	}
	else if (index == 1) {
		set_register_cs(regs, last_update, value_origin, value);
	}
	else if (index == 2) {
		set_register_ss(regs, last_update, value_origin, value);
	}
	else {
		/* Assuming index == 3 */
		set_register_ds(regs, last_update, value_origin, value);
	}
}

void set_segment_register_undefined(struct Registers *regs, unsigned int index, const char *last_update) {
	assert(index < 4);
	regs->last_update[index + 12] = last_update;
	regs->defined &= ~(0x1000 << index);
}

void set_register_es_relative(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value) {
	regs->es = value;
	regs->last_update[12] = last_update;
	regs->value_origin[12] = value_origin;
	regs->defined |= 0x1000;
	regs->relative |= 0x1000;
}

void set_register_cs_relative(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value) {
	regs->cs = value;
	regs->last_update[13] = last_update;
	regs->value_origin[13] = value_origin;
	regs->defined |= 0x2000;
	regs->relative |= 0x2000;
}

void set_register_ss_relative(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value) {
	regs->ss = value;
	regs->last_update[14] = last_update;
	regs->value_origin[14] = value_origin;
	regs->defined |= 0x4000;
	regs->relative |= 0x4000;
}

void set_register_ds_relative(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value) {
	regs->ds = value;
	regs->last_update[15] = last_update;
	regs->value_origin[15] = value_origin;
	regs->defined |= 0x8000;
	regs->relative |= 0x8000;
}

void set_segment_register_relative(struct Registers *regs, unsigned int index, const char *last_update, const char *value_origin, uint16_t value) {
	assert(index < 4);

	if (index == 0) {
		set_register_es_relative(regs, last_update, value_origin, value);
	}
	else if (index == 1) {
		set_register_cs_relative(regs, last_update, value_origin, value);
	}
	else if (index == 2) {
		set_register_ss_relative(regs, last_update, value_origin, value);
	}
	else {
		/* Assuming index == 3 */
		set_register_ds_relative(regs, last_update, value_origin, value);
	}
}

void copy_registers(struct Registers *target_regs, const struct Registers *source_regs) {
	int i;

	/* TODO: Optimise this copy */
	target_regs->al = source_regs->al;
	target_regs->ah = source_regs->ah;
	target_regs->cl = source_regs->cl;
	target_regs->ch = source_regs->ch;
	target_regs->dl = source_regs->dl;
	target_regs->dh = source_regs->dh;
	target_regs->bl = source_regs->bl;
	target_regs->bh = source_regs->bh;
	target_regs->sp = source_regs->sp;
	target_regs->bp = source_regs->bp;
	target_regs->si = source_regs->si;
	target_regs->di = source_regs->di;
	target_regs->es = source_regs->es;
	target_regs->cs = source_regs->cs;
	target_regs->ss = source_regs->ss;
	target_regs->ds = source_regs->ds;
	target_regs->defined = source_regs->defined;
	target_regs->relative = source_regs->relative;

	for (i = 0; i < 16; i++) {
		target_regs->last_update[i] = source_regs->last_update[i];
		target_regs->value_origin[i] = source_regs->value_origin[i];
	}
}

void merge_registers(struct Registers *regs, const struct Registers *other_regs) {
	int i;

	/* AX */
	if ((regs->relative & 0x10) != (other_regs->relative & 0x10)) {
		regs->defined &= 0xFFFC;
	}
	else {
		if ((other_regs->defined & 0x0001) == 0 || regs->al != other_regs->al) {
			regs->defined &= 0xFFFE;
		}

		if ((other_regs->defined & 0x0002) == 0 || regs->ah != other_regs->ah) {
			regs->defined &= 0xFFFD;
		}
	}

	/* CX */
	if ((regs->relative & 0x20) != (other_regs->relative & 0x20)) {
		regs->defined &= 0xFFF3;
	}
	else {
		if ((other_regs->defined & 0x0004) == 0 || regs->cl != other_regs->cl) {
			regs->defined &= 0xFFFB;
		}

		if ((other_regs->defined & 0x0008) == 0 || regs->ch != other_regs->ch) {
			regs->defined &= 0xFFF7;
		}
	}

	/* DX */
	if ((regs->relative & 0x40) != (other_regs->relative & 0x40)) {
		regs->defined &= 0xFFCF;
	}
	else {
		if ((other_regs->defined & 0x0010) == 0 || regs->dl != other_regs->dl) {
			regs->defined &= 0xFFEF;
		}

		if ((other_regs->defined & 0x0020) == 0 || regs->dh != other_regs->dh) {
			regs->defined &= 0xFFDF;
		}
	}

	/* BX */
	if ((regs->relative & 0x80) != (other_regs->relative & 0x80)) {
		regs->defined &= 0xFF3F;
	}
	else {
		if ((other_regs->defined & 0x0040) == 0 || regs->bl != other_regs->bl) {
			regs->defined &= 0xFFBF;
		}

		if ((other_regs->defined & 0x0080) == 0 || regs->bh != other_regs->bh) {
			regs->defined &= 0xFF7F;
		}
	}

	/* SP */
	if ((other_regs->defined & 0x0100) == 0 || regs->sp != other_regs->sp || (regs->relative & 0x0100) != (other_regs->relative & 0x0100)) {
		regs->defined &= 0xFEFF;
	}

	/* BP */
	if ((other_regs->defined & 0x0200) == 0 || regs->bp != other_regs->bp || (regs->relative & 0x0200) != (other_regs->relative & 0x0200)) {
		regs->defined &= 0xFDFF;
	}

	/* SI */
	if ((other_regs->defined & 0x0400) == 0 || regs->si != other_regs->si || (regs->relative & 0x0400) != (other_regs->relative & 0x0400)) {
		regs->defined &= 0xFBFF;
	}

	/* DI */
	if ((other_regs->defined & 0x0800) == 0 || regs->di != other_regs->di || (regs->relative & 0x0800) != (other_regs->relative & 0x0800)) {
		regs->defined &= 0xF7FF;
	}

	/* ES */
	if ((other_regs->defined & 0x1000) == 0 || regs->es != other_regs->es || (regs->relative & 0x1000) != (other_regs->relative & 0x1000)) {
		regs->defined &= 0xEFFF;
	}

	/* CS */
	if ((other_regs->defined & 0x2000) == 0 || regs->cs != other_regs->cs || (regs->relative & 0x2000) != (other_regs->relative & 0x2000)) {
		regs->defined &= 0xDFFF;
	}

	/* SS */
	if ((other_regs->defined & 0x4000) == 0 || regs->ss != other_regs->ss || (regs->relative & 0x4000) != (other_regs->relative & 0x4000)) {
		regs->defined &= 0xBFFF;
	}

	/* DS */
	if ((other_regs->defined & 0x8000) == 0 || regs->ds != other_regs->ds || (regs->relative & 0x8000) != (other_regs->relative & 0x8000)) {
		regs->defined &= 0x7FFF;
	}

	for (i = 0; i < 16; i++) {
		if (regs->last_update[i] != other_regs->last_update[i]) {
			regs->last_update[i] = NULL;
		}

		if (regs->value_origin[i] != other_regs->value_origin[i]) {
			regs->value_origin[i] = NULL;
		}
	}

	regs->merged = 0xFFFF;
}

int changes_on_merging_registers(const struct Registers *regs, const struct Registers *other_regs) {
	int i;

	if ((regs->defined & other_regs->defined) != regs->defined) {
		return 1;
	}

	if ((regs->defined & regs->relative) != (other_regs->defined & other_regs->relative)) {
		return 1;
	}

	for (i = 0; i < 8; i++) {
		if (get_word_register(regs, i) != get_word_register(other_regs, i)) {
			return 1;
		}
	}

	for (i = 0; i < 4; i++) {
		if (get_segment_register(regs, i) != get_segment_register(other_regs, i)) {
			return 1;
		}
	}

	return 0;
}

void set_all_registers_undefined(struct Registers *regs) {
	int i;
	regs->defined = 0;

	for (i = 0; i < 16; i++) {
		regs->last_update[i] = NULL;
		regs->value_origin[i] = NULL;
	}
}

void set_all_registers_undefined_except_cs(struct Registers *regs) {
	int i;
	regs->defined &= 0x2000;

	for (i = 0; i < 16; i++) {
		if (i != 13) {
			regs->last_update[i] = NULL;
			regs->value_origin[i] = NULL;
		}
	}
}
