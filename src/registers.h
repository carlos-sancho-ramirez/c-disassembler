#ifndef _REGISTERS_H_
#define _REGISTERS_H_
#include <stdint.h>

#define REGISTER_DEFINED_OUTSIDE ((const char *) 1)

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
	 * Points to the last opcode that modified the register value.
	 * NULL if the register is not defined.
	 * REGISTER_DEFINED_OUTSIDE if the register comes registered by the OS.
	 */
	const char *defined[16];
	uint16_t relative;
};

int is_register_al_defined(struct Registers *regs);
int is_register_ah_defined(struct Registers *regs);
int is_register_cl_defined(struct Registers *regs);
int is_register_ch_defined(struct Registers *regs);
int is_register_dl_defined(struct Registers *regs);
int is_register_dh_defined(struct Registers *regs);
int is_register_bl_defined(struct Registers *regs);
int is_register_bh_defined(struct Registers *regs);
int is_byte_register_defined(struct Registers * regs, unsigned int index);

int is_register_ax_defined(struct Registers *regs);
int is_register_cx_defined(struct Registers *regs);
int is_register_dx_defined(struct Registers *regs);
int is_register_bx_defined(struct Registers *regs);
int is_register_sp_defined(struct Registers *regs);
int is_register_bp_defined(struct Registers *regs);
int is_register_si_defined(struct Registers *regs);
int is_register_di_defined(struct Registers *regs);
int is_register_ax_defined_and_absolute(struct Registers *regs);
int is_register_cx_defined_and_absolute(struct Registers *regs);
int is_register_dx_defined_and_absolute(struct Registers *regs);
int is_register_bx_defined_and_absolute(struct Registers *regs);
int is_register_sp_defined_and_absolute(struct Registers *regs);
int is_register_bp_defined_and_absolute(struct Registers *regs);
int is_register_si_defined_and_absolute(struct Registers *regs);
int is_register_di_defined_and_absolute(struct Registers *regs);
int is_register_ax_defined_and_relative(struct Registers *regs);
int is_register_cx_defined_and_relative(struct Registers *regs);
int is_register_dx_defined_and_relative(struct Registers *regs);
int is_register_bx_defined_and_relative(struct Registers *regs);
int is_register_sp_defined_and_relative(struct Registers *regs);
int is_register_bp_defined_and_relative(struct Registers *regs);
int is_register_si_defined_and_relative(struct Registers *regs);
int is_register_di_defined_and_relative(struct Registers *regs);
int is_word_register_defined(struct Registers * regs, unsigned int index);
int is_word_register_defined_and_relative(struct Registers * regs, unsigned int index);

int is_segment_register_defined(struct Registers *regs, unsigned int index);
int is_segment_register_defined_and_absolute(struct Registers *regs, unsigned int index);

int is_register_es_defined(struct Registers *regs);
int is_register_cs_defined(struct Registers *regs);
int is_register_ss_defined(struct Registers *regs);
int is_register_ds_defined(struct Registers *regs);
int is_register_es_defined_and_relative(struct Registers *regs);
int is_register_cs_defined_and_relative(struct Registers *regs);
int is_register_ss_defined_and_relative(struct Registers *regs);
int is_register_ds_defined_and_relative(struct Registers *regs);
int is_segment_register_defined(struct Registers *regs, unsigned int index);
int is_segment_register_defined_and_relative(struct Registers *regs, unsigned int index);

const char *where_register_ax_defined(struct Registers *regs);
const char *where_register_dx_defined(struct Registers *regs);

unsigned int get_register_al(struct Registers *regs);
unsigned int get_register_ah(struct Registers *regs);
unsigned int get_register_cl(struct Registers *regs);
unsigned int get_register_ch(struct Registers *regs);
unsigned int get_register_dl(struct Registers *regs);
unsigned int get_register_dh(struct Registers *regs);
unsigned int get_register_bl(struct Registers *regs);
unsigned int get_register_bh(struct Registers *regs);

unsigned int get_register_ax(struct Registers *regs);
unsigned int get_register_cx(struct Registers *regs);
unsigned int get_register_dx(struct Registers *regs);
unsigned int get_register_bx(struct Registers *regs);
unsigned int get_register_sp(struct Registers *regs);
unsigned int get_register_bp(struct Registers *regs);
unsigned int get_register_si(struct Registers *regs);
unsigned int get_register_di(struct Registers *regs);

unsigned int get_register_es(struct Registers *regs);
unsigned int get_register_cs(struct Registers *regs);
unsigned int get_register_ss(struct Registers *regs);
unsigned int get_register_ds(struct Registers *regs);

unsigned int get_byte_register(struct Registers *regs, unsigned int index);
unsigned int get_word_register(struct Registers *regs, unsigned int index);
unsigned int get_segment_register(struct Registers *regs, unsigned int index);

void set_byte_register(struct Registers *regs, unsigned int index, const char *where, unsigned char value);
void set_word_register(struct Registers *regs, unsigned int index, const char *where, uint16_t value);
void set_word_register_relative(struct Registers *regs, unsigned int index, const char *where, uint16_t value);
void mark_register_ax_undefined(struct Registers *regs);
void mark_register_cx_undefined(struct Registers *regs);
void mark_register_bx_undefined(struct Registers *regs);
void mark_word_register_undefined(struct Registers *regs, unsigned int index);

void set_register_al_undefined(struct Registers *regs);
void set_register_ax_undefined(struct Registers *regs);

void set_register_es(struct Registers *regs, const char *where, uint16_t value);
void set_register_cs(struct Registers *regs, const char *where, uint16_t value);
void set_register_ss(struct Registers *regs, const char *where, uint16_t value);
void set_register_ds(struct Registers *regs, const char *where, uint16_t value);
void set_segment_register(struct Registers *regs, unsigned int index, const char *where, uint16_t value);

void set_register_es_relative(struct Registers *regs, const char *where, uint16_t value);
void set_register_cs_relative(struct Registers *regs, const char *where, uint16_t value);
void set_register_ss_relative(struct Registers *regs, const char *where, uint16_t value);
void set_register_ds_relative(struct Registers *regs, const char *where, uint16_t value);
void set_segment_register_relative(struct Registers *regs, unsigned int index, const char *where, uint16_t value);
void mark_segment_register_undefined(struct Registers *regs, unsigned int index);

void copy_registers(struct Registers *target_regs, const struct Registers *source_regs);
void merge_registers(struct Registers *regs, const struct Registers *other_regs);
int changes_on_merging_registers(const struct Registers *regs, const struct Registers *other_regs);
void make_all_registers_undefined(struct Registers *regs);
void make_all_registers_undefined_except_cs(struct Registers *regs);
#endif // _REGISTERS_H_
