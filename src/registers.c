#include "registers.h"
#include <assert.h>

int is_register_al_defined(struct Registers *regs) {
    return regs->defined & 0x01;
}

int is_register_cl_defined(struct Registers *regs) {
    return regs->defined & 0x04;
}

int is_register_dl_defined(struct Registers *regs) {
    return regs->defined & 0x10;
}

int is_register_bl_defined(struct Registers *regs) {
    return regs->defined & 0x40;
}

int is_register_ah_defined(struct Registers *regs) {
    return regs->defined & 0x02;
}

int is_register_ch_defined(struct Registers *regs) {
    return regs->defined & 0x08;
}

int is_register_dh_defined(struct Registers *regs) {
    return regs->defined & 0x20;
}

int is_register_bh_defined(struct Registers *regs) {
    return regs->defined & 0x80;
}

int is_byte_register_defined(struct Registers * regs, unsigned int index) {
    assert(index < 8);
    return (index == 0)? is_register_al_defined(regs) :
            (index == 1)? is_register_cl_defined(regs) :
            (index == 2)? is_register_dl_defined(regs) :
            (index == 3)? is_register_bl_defined(regs) :
            (index == 4)? is_register_ah_defined(regs) :
            (index == 5)? is_register_ch_defined(regs) :
            (index == 6)? is_register_dh_defined(regs) :
            is_register_bh_defined(regs);
}

int is_register_ax_defined(struct Registers *regs) {
    return (regs->defined & 0x03) == 0x03;
}

int is_register_cx_defined(struct Registers *regs) {
    return (regs->defined & 0x0C) == 0x0C;
}

int is_register_dx_defined(struct Registers *regs) {
    return (regs->defined & 0x30) == 0x30;
}

int is_register_bx_defined(struct Registers *regs) {
    return (regs->defined & 0xC0) == 0xC0;
}

int is_register_sp_defined(struct Registers *regs) {
    return regs->defined & 0x100;
}

int is_register_bp_defined(struct Registers *regs) {
    return regs->defined & 0x200;
}

int is_register_si_defined(struct Registers *regs) {
    return regs->defined & 0x400;
}

int is_register_di_defined(struct Registers *regs) {
    return regs->defined & 0x800;
}

int is_register_ax_defined_and_absolute(struct Registers *regs) {
    return (regs->defined & 0x03) == 0x03 && (regs->relative & 0x10) == 0;
}

int is_register_cx_defined_and_absolute(struct Registers *regs) {
    return (regs->defined & 0x0C) == 0x0C && (regs->relative & 0x20) == 0;
}

int is_register_dx_defined_and_absolute(struct Registers *regs) {
    return (regs->defined & 0x30) == 0x30 && (regs->relative & 0x40) == 0;
}

int is_register_bx_defined_and_absolute(struct Registers *regs) {
    return (regs->defined & 0xC0) == 0xC0 && (regs->relative & 0x80) == 0;
}

int is_register_sp_defined_and_absolute(struct Registers *regs) {
    return regs->defined & 0x100 && (regs->relative & 0x100) == 0;
}

int is_register_bp_defined_and_absolute(struct Registers *regs) {
    return regs->defined & 0x200 && (regs->relative & 0x200) == 0;
}

int is_register_si_defined_and_absolute(struct Registers *regs) {
    return regs->defined & 0x400 && (regs->relative & 0x400) == 0;
}

int is_register_di_defined_and_absolute(struct Registers *regs) {
    return regs->defined & 0x800 && (regs->relative & 0x800) == 0;
}

int is_register_ax_defined_and_relative(struct Registers *regs) {
    return (regs->defined & 0x03) == 0x03 && regs->relative & 0x10;
}

int is_register_cx_defined_and_relative(struct Registers *regs) {
    return (regs->defined & 0x0C) == 0x0C && regs->relative & 0x20;
}

int is_register_dx_defined_and_relative(struct Registers *regs) {
    return (regs->defined & 0x30) == 0x30 && regs->relative & 0x40;
}

int is_register_bx_defined_and_relative(struct Registers *regs) {
    return (regs->defined & 0xC0) == 0xC0 && regs->relative & 0x80;
}

int is_register_sp_defined_and_relative(struct Registers *regs) {
    return regs->defined & regs->relative & 0x100;
}

int is_register_bp_defined_and_relative(struct Registers *regs) {
    return regs->defined & regs->relative & 0x200;
}

int is_register_si_defined_and_relative(struct Registers *regs) {
    return regs->defined & regs->relative & 0x400;
}

int is_register_di_defined_and_relative(struct Registers *regs) {
    return regs->defined & regs->relative & 0x800;
}

int is_word_register_defined(struct Registers * regs, unsigned int index) {
    assert(index < 8);
    return (index == 0)? is_register_ax_defined(regs) :
            (index == 1)? is_register_cx_defined(regs) :
            (index == 2)? is_register_dx_defined(regs) :
            (index == 3)? is_register_bx_defined(regs) :
            (index == 4)? is_register_sp_defined(regs) :
            (index == 5)? is_register_bp_defined(regs) :
            (index == 6)? is_register_si_defined(regs) :
            is_register_di_defined(regs);
}

int is_word_register_defined_and_relative(struct Registers * regs, unsigned int index) {
    assert(index < 8);
    return (index == 0)? is_register_ax_defined_and_relative(regs) :
            (index == 1)? is_register_cx_defined_and_relative(regs) :
            (index == 2)? is_register_dx_defined_and_relative(regs) :
            (index == 3)? is_register_bx_defined_and_relative(regs) :
            (index == 4)? is_register_sp_defined_and_relative(regs) :
            (index == 5)? is_register_bp_defined_and_relative(regs) :
            (index == 6)? is_register_si_defined_and_relative(regs) :
            is_register_di_defined_and_relative(regs);
}

int is_register_es_defined(struct Registers *regs) {
    return regs->defined & 0x1000;
}

int is_register_cs_defined(struct Registers *regs) {
    return regs->defined & 0x2000;
}

int is_register_ss_defined(struct Registers *regs) {
    return regs->defined & 0x4000;
}

int is_register_ds_defined(struct Registers *regs) {
    return regs->defined & 0x8000;
}

int is_segment_register_defined(struct Registers *regs, unsigned int index) {
    assert(index < 4);
    return regs->defined & (0x1000 << index);
}

int is_segment_register_defined_and_absolute(struct Registers *regs, unsigned int index) {
    assert(index < 4);
    const int mask = 0x1000 << index;
    return (regs->defined & mask) && (regs->relative & mask) == 0;
}

int is_register_es_defined_and_relative(struct Registers *regs) {
    return regs->defined & regs->relative & 0x1000;
}

int is_register_cs_defined_and_relative(struct Registers *regs) {
    return regs->defined & regs->relative & 0x2000;
}

int is_register_ss_defined_and_relative(struct Registers *regs) {
    return regs->defined & regs->relative & 0x4000;
}

int is_register_ds_defined_and_relative(struct Registers *regs) {
    return regs->defined & regs->relative & 0x8000;
}

int is_segment_register_defined_and_relative(struct Registers *regs, unsigned int index) {
    assert(index < 4);
    const int mask = 0x1000 << index;
    return regs->defined & regs->relative & mask;
}

unsigned int get_register_al(struct Registers *regs) {
    return regs->al;
}

unsigned int get_register_ah(struct Registers *regs) {
    return regs->ah;
}

unsigned int get_register_cl(struct Registers *regs) {
    return regs->cl;
}

unsigned int get_register_ch(struct Registers *regs) {
    return regs->ch;
}

unsigned int get_register_dl(struct Registers *regs) {
    return regs->dl;
}

unsigned int get_register_dh(struct Registers *regs) {
    return regs->dh;
}

unsigned int get_register_bl(struct Registers *regs) {
    return regs->bl;
}

unsigned int get_register_bh(struct Registers *regs) {
    return regs->bh;
}

unsigned int get_register_ax(struct Registers *regs) {
    return (regs->ah << 8) + regs->al;
}

unsigned int get_register_cx(struct Registers *regs) {
    return (regs->ch << 8) + regs->cl;
}

unsigned int get_register_dx(struct Registers *regs) {
    return (regs->dh << 8) + regs->dl;
}

unsigned int get_register_bx(struct Registers *regs) {
    return (regs->bh << 8) + regs->bl;
}

unsigned int get_register_sp(struct Registers *regs) {
    return regs->sp;
}

unsigned int get_register_bp(struct Registers *regs) {
    return regs->bp;
}

unsigned int get_register_si(struct Registers *regs) {
    return regs->si;
}

unsigned int get_register_di(struct Registers *regs) {
    return regs->di;
}

unsigned int get_register_es(struct Registers *regs) {
    return regs->es;
}

unsigned int get_register_cs(struct Registers *regs) {
    return regs->cs;
}

unsigned int get_register_ss(struct Registers *regs) {
    return regs->ss;
}

unsigned int get_register_ds(struct Registers *regs) {
    return regs->ds;
}

unsigned int get_byte_register(struct Registers *regs, unsigned int index) {
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
        // Assuming index == 7
        return regs->bh;
    }
}

unsigned int get_word_register(struct Registers *regs, unsigned int index) {
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
        // Assuming index == 7
        return regs->di;
    }
}

unsigned int get_segment_register(struct Registers *regs, unsigned int index) {
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
        // Assuming index == 3
        return get_register_ds(regs);
    }
}

void set_byte_register(struct Registers *regs, unsigned int index, unsigned char value) {
    assert(index < 8);

    // This should be optimised using the regs pointer plus index to determinate the byte within
    // the struct to change. But for now I implement it in a way that I can ensure it works in any
    // architecture.
    if (index == 0) {
        regs->al = value;
        regs->defined |= 0x01;
    }
    else if (index == 1) {
        regs->cl = value;
        regs->defined |= 0x04;
    }
    else if (index == 2) {
        regs->dl = value;
        regs->defined |= 0x10;
    }
    else if (index == 3) {
        regs->bl = value;
        regs->defined |= 0x40;
    }
    else if (index == 4) {
        regs->ah = value;
        regs->defined |= 0x02;
    }
    else if (index == 5) {
        regs->ch = value;
        regs->defined |= 0x08;
    }
    else if (index == 6) {
        regs->dh = value;
        regs->defined |= 0x20;
    }
    else {
        // Assuming index == 7
        regs->bh = value;
        regs->defined |= 0x80;
    }
}

void set_word_register(struct Registers *regs, unsigned int index, uint16_t value) {
    assert(index < 8);

    // This should be optimised using the regs pointer plus index to determinate the byte within
    // the struct to change. But for now I implement it in a way that I can ensure it works in any
    // architecture.
    if (index == 0) {
        regs->al = value & 0xFF;
        regs->ah = (value >> 8) & 0xFF;
        regs->defined |= 0x03;
        regs->relative &= ~0x10;
    }
    else if (index == 1) {
        regs->cl = value & 0xFF;
        regs->ch = (value >> 8) & 0xFF;
        regs->defined |= 0x0C;
        regs->relative &= ~0x20;
    }
    else if (index == 2) {
        regs->dl = value & 0xFF;
        regs->dh = (value >> 8) & 0xFF;
        regs->defined |= 0x30;
        regs->relative &= ~0x40;
    }
    else if (index == 3) {
        regs->bl = value & 0xFF;
        regs->bh = (value >> 8) & 0xFF;
        regs->defined |= 0xC0;
        regs->relative &= ~0x80;
    }
    else if (index == 4) {
        regs->sp = value;
        regs->defined |= 0x100;
        regs->relative &= ~0x100;
    }
    else if (index == 5) {
        regs->bp = value;
        regs->defined |= 0x200;
        regs->relative &= ~0x200;
    }
    else if (index == 6) {
        regs->si = value;
        regs->defined |= 0x400;
        regs->relative &= ~0x400;
    }
    else {
        // Assuming index == 7
        regs->di = value;
        regs->defined |= 0x800;
        regs->relative &= ~0x800;
    }
}

void set_word_register_relative(struct Registers *regs, unsigned int index, uint16_t value) {
    assert(index < 8);

    // This should be optimised using the regs pointer plus index to determinate the byte within
    // the struct to change. But for now I implement it in a way that I can ensure it works in any
    // architecture.
    if (index == 0) {
        regs->al = value & 0xFF;
        regs->ah = (value >> 8) & 0xFF;
        regs->defined |= 0x03;
        regs->relative |= 0x10;
    }
    else if (index == 1) {
        regs->cl = value & 0xFF;
        regs->ch = (value >> 8) & 0xFF;
        regs->defined |= 0x0C;
        regs->relative |= 0x20;
    }
    else if (index == 2) {
        regs->dl = value & 0xFF;
        regs->dh = (value >> 8) & 0xFF;
        regs->defined |= 0x30;
        regs->relative |= 0x40;
    }
    else if (index == 3) {
        regs->bl = value & 0xFF;
        regs->bh = (value >> 8) & 0xFF;
        regs->defined |= 0xC0;
        regs->relative |= 0x80;
    }
    else if (index == 4) {
        regs->sp = value;
        regs->defined |= 0x100;
        regs->relative |= 0x100;
    }
    else if (index == 5) {
        regs->bp = value;
        regs->defined |= 0x200;
        regs->relative |= 0x200;
    }
    else if (index == 6) {
        regs->si = value;
        regs->defined |= 0x400;
        regs->relative |= 0x400;
    }
    else {
        // Assuming index == 7
        regs->di = value;
        regs->defined |= 0x800;
        regs->relative |= 0x800;
    }
}

void set_register_al_undefined(struct Registers *regs) {
    regs->defined &= ~0x01;
}

void set_register_ax_undefined(struct Registers *regs) {
    regs->defined &= ~0x03;
}

void set_register_es(struct Registers *regs, uint16_t value) {
    regs->es = value;
    regs->defined |= 0x1000;
    regs->relative &= ~0x1000;
}

void set_register_cs(struct Registers *regs, uint16_t value) {
    regs->cs = value;
    regs->defined |= 0x2000;
    regs->relative &= ~0x2000;
}

void set_register_ss(struct Registers *regs, uint16_t value) {
    regs->ss = value;
    regs->defined |= 0x4000;
    regs->relative &= ~0x4000;
}

void set_register_ds(struct Registers *regs, uint16_t value) {
    regs->ds = value;
    regs->defined |= 0x8000;
    regs->relative &= ~0x8000;
}

void set_segment_register(struct Registers *regs, unsigned int index, uint16_t value) {
    assert(index < 4);

    if (index == 0) {
        set_register_es(regs, value);
    }
    else if (index == 1) {
        set_register_cs(regs, value);
    }
    else if (index == 2) {
        set_register_ss(regs, value);
    }
    else {
        // Assuming index == 3
        set_register_ds(regs, value);
    }
}

void set_register_es_relative(struct Registers *regs, uint16_t value) {
    regs->es = value;
    regs->defined |= 0x1000;
    regs->relative |= 0x1000;
}

void set_register_cs_relative(struct Registers *regs, uint16_t value) {
    regs->cs = value;
    regs->defined |= 0x2000;
    regs->relative |= 0x2000;
}

void set_register_ss_relative(struct Registers *regs, uint16_t value) {
    regs->ss = value;
    regs->defined |= 0x4000;
    regs->relative |= 0x4000;
}

void set_register_ds_relative(struct Registers *regs, uint16_t value) {
    regs->ds = value;
    regs->defined |= 0x8000;
    regs->relative |= 0x8000;
}

void set_segment_register_relative(struct Registers *regs, unsigned int index, uint16_t value) {
    assert(index < 4);

    if (index == 0) {
        set_register_es_relative(regs, value);
    }
    else if (index == 1) {
        set_register_cs_relative(regs, value);
    }
    else if (index == 2) {
        set_register_ss_relative(regs, value);
    }
    else {
        // Assuming index == 3
        set_register_ds_relative(regs, value);
    }
}

void make_all_registers_undefined(struct Registers *regs) {
    regs->defined = 0;
}
