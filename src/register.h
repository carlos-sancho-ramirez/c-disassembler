#ifndef _REGISTER_H_
#define _REGISTER_H_
#include <stdint.h>

struct Registers {
	unsigned char al;
	unsigned char ah;
	unsigned char cl;
	unsigned char ch;
	unsigned char dl;
	unsigned char dh;
	unsigned char bl;
	unsigned char bh;
	uint16_t sp;
	uint16_t bp;
	uint16_t si;
	uint16_t di;
	uint16_t es;
	uint16_t cs;
	uint16_t ss;
	uint16_t ds;

	/**
	 * Points to the opcode where the value in this register was set.
	 * NULL if it is unknown.
	 */
	const char *value_origin[16];

	/**
	 * Points to the last opcode that modified the register value.
	 * NULL if it is unknown.
	 */
	const char *last_update[16];

	/**
	 * Whether the corresponding register definition and value comes from the
	 * merging performed before the start of the current block.
	 */
	uint16_t merged;

	/**
	 * Whether the corresponding register has a valid value.
	 */
	uint16_t defined;

	/**
	 * Whether the corresponding register has a relative value on it.
	 *
	 * Bits from 4 to 15 are reserved for registers WORD registers + segments.
	 * If the corresponding flag is set, it means that the value is relative from the segment_start.
	 * This is only relevant if the curresponding register is defined as well.
	 *
	 * Bit 0 is set, it means that the value at SP register is relative from BP.
	 * This bit will only be considered if SP is undefined.
	 */
	uint16_t relative;
};

int is_register_al_defined(const struct Registers *regs);
int is_register_ah_defined(const struct Registers *regs);
int is_register_cl_defined(const struct Registers *regs);
int is_register_ch_defined(const struct Registers *regs);
int is_register_dl_defined(const struct Registers *regs);
int is_register_dh_defined(const struct Registers *regs);
int is_register_bl_defined(const struct Registers *regs);
int is_register_bh_defined(const struct Registers *regs);
int is_byte_register_defined(const struct Registers * regs, unsigned int index);

int is_register_ax_defined(const struct Registers *regs);
int is_register_cx_defined(const struct Registers *regs);
int is_register_dx_defined(const struct Registers *regs);
int is_register_bx_defined(const struct Registers *regs);
int is_register_sp_defined(const struct Registers *regs);
int is_register_bp_defined(const struct Registers *regs);
int is_register_si_defined(const struct Registers *regs);
int is_register_di_defined(const struct Registers *regs);
int is_register_ax_defined_absolute(const struct Registers *regs);
int is_register_cx_defined_absolute(const struct Registers *regs);
int is_register_dx_defined_absolute(const struct Registers *regs);
int is_register_bx_defined_absolute(const struct Registers *regs);
int is_register_sp_defined_absolute(const struct Registers *regs);
int is_register_bp_defined_absolute(const struct Registers *regs);
int is_register_si_defined_absolute(const struct Registers *regs);
int is_register_di_defined_absolute(const struct Registers *regs);
int is_register_ax_defined_relative(const struct Registers *regs);
int is_register_cx_defined_relative(const struct Registers *regs);
int is_register_dx_defined_relative(const struct Registers *regs);
int is_register_bx_defined_relative(const struct Registers *regs);
int is_register_sp_defined_relative(const struct Registers *regs);
int is_register_bp_defined_relative(const struct Registers *regs);
int is_register_si_defined_relative(const struct Registers *regs);
int is_register_di_defined_relative(const struct Registers *regs);
int is_word_register_defined(const struct Registers * regs, unsigned int index);
int is_word_register_defined_relative(const struct Registers * regs, unsigned int index);

int is_register_es_defined(const struct Registers *regs);
int is_register_cs_defined(const struct Registers *regs);
int is_register_ss_defined(const struct Registers *regs);
int is_register_ds_defined(const struct Registers *regs);
int is_register_es_defined_relative(const struct Registers *regs);
int is_register_cs_defined_relative(const struct Registers *regs);
int is_register_ss_defined_relative(const struct Registers *regs);
int is_register_ds_defined_relative(const struct Registers *regs);
int is_segment_register_defined(const struct Registers *regs, unsigned int index);
int is_segment_register_defined_absolute(const struct Registers *regs, unsigned int index);
int is_segment_register_defined_relative(const struct Registers *regs, unsigned int index);

int is_register_sp_relative_from_bp(const struct Registers *regs);

int is_register_cx_merged(const struct Registers *regs);
int is_register_dx_merged(const struct Registers *regs);
int is_register_ds_merged(const struct Registers *regs);

const char *get_register_ax_value_origin(const struct Registers *regs);
const char *get_register_dx_value_origin(const struct Registers *regs);
const char *get_register_bp_value_origin(const struct Registers *regs);
const char *get_word_register_value_origin(const struct Registers *regs, unsigned int index);
const char *get_segment_register_value_origin(const struct Registers *regs, unsigned int index);

unsigned int get_register_al(const struct Registers *regs);
unsigned int get_register_ah(const struct Registers *regs);
unsigned int get_register_cl(const struct Registers *regs);
unsigned int get_register_ch(const struct Registers *regs);
unsigned int get_register_dl(const struct Registers *regs);
unsigned int get_register_dh(const struct Registers *regs);
unsigned int get_register_bl(const struct Registers *regs);
unsigned int get_register_bh(const struct Registers *regs);

unsigned int get_register_ax(const struct Registers *regs);
unsigned int get_register_cx(const struct Registers *regs);
unsigned int get_register_dx(const struct Registers *regs);
unsigned int get_register_bx(const struct Registers *regs);
unsigned int get_register_sp(const struct Registers *regs);
unsigned int get_register_bp(const struct Registers *regs);
unsigned int get_register_si(const struct Registers *regs);
unsigned int get_register_di(const struct Registers *regs);

unsigned int get_register_es(const struct Registers *regs);
unsigned int get_register_cs(const struct Registers *regs);
unsigned int get_register_ss(const struct Registers *regs);
unsigned int get_register_ds(const struct Registers *regs);

unsigned int get_byte_register(const struct Registers *regs, unsigned int index);
unsigned int get_word_register(const struct Registers *regs, unsigned int index);
unsigned int get_segment_register(const struct Registers *regs, unsigned int index);

void set_byte_register(struct Registers *regs, unsigned int index, const char *last_update, const char *value_origin, unsigned char value);
void set_word_register(struct Registers *regs, unsigned int index, const char *last_update, const char *value_origin, uint16_t value);
void set_word_register_relative(struct Registers *regs, unsigned int index, const char *last_update, const char *value_origin, uint16_t value);
void set_register_ax_undefined(struct Registers *regs, const char *last_update);
void set_register_cx_undefined(struct Registers *regs, const char *last_update);
void set_register_dx_undefined(struct Registers *regs, const char *last_update);
void set_register_bx_undefined(struct Registers *regs, const char *last_update);
void set_register_es_undefined(struct Registers *regs, const char *last_update);
void set_register_ds_undefined(struct Registers *regs, const char *last_update);
void set_word_register_undefined(struct Registers *regs, unsigned int index, const char *last_update);

void set_register_al_undefined(struct Registers *regs, const char *last_update);
void set_register_ax_undefined(struct Registers *regs, const char *last_update);

void set_register_sp(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value);
void set_register_es(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value);
void set_register_cs(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value);
void set_register_ss(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value);
void set_register_ds(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value);
void set_segment_register(struct Registers *regs, unsigned int index, const char *last_update, const char *value_origin, uint16_t value);

void set_register_sp_relative(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value);

void set_register_es_relative(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value);
void set_register_cs_relative(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value);
void set_register_ss_relative(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value);
void set_register_ds_relative(struct Registers *regs, const char *last_update, const char *value_origin, uint16_t value);
void set_segment_register_relative(struct Registers *regs, unsigned int index, const char *last_update, const char *value_origin, uint16_t value);
void set_segment_register_undefined(struct Registers *regs, unsigned int index, const char *last_update);

void set_register_sp_relative_from_bp(struct Registers *regs, const char *last_update, int value);

void copy_registers(struct Registers *target_regs, const struct Registers *source_regs);
void merge_registers(struct Registers *regs, const struct Registers *other_regs);
int changes_on_merging_registers(const struct Registers *regs, const struct Registers *other_regs);
void set_all_registers_undefined(struct Registers *regs);
void set_all_registers_undefined_except_cs(struct Registers *regs);

#endif /* _REGISTER_H_ */
