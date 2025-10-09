#include "registers.h"
#include <assert.h>
#include <stdlib.h>

int is_register_al_defined(struct Registers *regs) {
    return regs->defined[0] != NULL;
}

int is_register_cl_defined(struct Registers *regs) {
    return regs->defined[2] != NULL;
}

int is_register_dl_defined(struct Registers *regs) {
    return regs->defined[4] != NULL;
}

int is_register_bl_defined(struct Registers *regs) {
    return regs->defined[6] != NULL;
}

int is_register_ah_defined(struct Registers *regs) {
    return regs->defined[1] != NULL;
}

int is_register_ch_defined(struct Registers *regs) {
    return regs->defined[3] != NULL;
}

int is_register_dh_defined(struct Registers *regs) {
    return regs->defined[5] != NULL;
}

int is_register_bh_defined(struct Registers *regs) {
    return regs->defined[7] != NULL;
}

int is_byte_register_defined(struct Registers * regs, unsigned int index) {
    assert(index < 8);
    if (index < 4) {
        return regs->defined[index * 2] != NULL;
    }
    else {
        return regs->defined[(index & 3) * 2 + 1] != NULL;
    }
}

int is_register_ax_defined(struct Registers *regs) {
    return regs->defined[0] != NULL && regs->defined[1] != NULL;
}

int is_register_cx_defined(struct Registers *regs) {
    return regs->defined[2] != NULL && regs->defined[3] != NULL;
}

int is_register_dx_defined(struct Registers *regs) {
    return regs->defined[4] != NULL && regs->defined[5] != NULL;
}

int is_register_bx_defined(struct Registers *regs) {
    return regs->defined[6] != NULL && regs->defined[7] != NULL;
}

int is_register_sp_defined(struct Registers *regs) {
    return regs->defined[8] != NULL;
}

int is_register_bp_defined(struct Registers *regs) {
    return regs->defined[9] != NULL;
}

int is_register_si_defined(struct Registers *regs) {
    return regs->defined[10] != NULL;
}

int is_register_di_defined(struct Registers *regs) {
    return regs->defined[11] != NULL;
}

int is_register_ax_defined_and_absolute(struct Registers *regs) {
    return regs->defined[0] != NULL && regs->defined[1] != NULL && (regs->relative & 0x10) == 0;
}

int is_register_cx_defined_and_absolute(struct Registers *regs) {
    return regs->defined[2] != NULL && regs->defined[3] != NULL && (regs->relative & 0x20) == 0;
}

int is_register_dx_defined_and_absolute(struct Registers *regs) {
    return regs->defined[4] != NULL && regs->defined[5] != NULL && (regs->relative & 0x40) == 0;
}

int is_register_bx_defined_and_absolute(struct Registers *regs) {
    return regs->defined[6] != NULL && regs->defined[7] != NULL && (regs->relative & 0x80) == 0;
}

int is_register_sp_defined_and_absolute(struct Registers *regs) {
    return regs->defined[8] != NULL && (regs->relative & 0x100) == 0;
}

int is_register_bp_defined_and_absolute(struct Registers *regs) {
    return regs->defined[9] != NULL && (regs->relative & 0x200) == 0;
}

int is_register_si_defined_and_absolute(struct Registers *regs) {
    return regs->defined[10] != NULL && (regs->relative & 0x400) == 0;
}

int is_register_di_defined_and_absolute(struct Registers *regs) {
    return regs->defined[11] != NULL && (regs->relative & 0x800) == 0;
}

int is_register_ax_defined_and_relative(struct Registers *regs) {
    return regs->defined[0] != NULL && regs->defined[1] != NULL && regs->relative & 0x10;
}

int is_register_cx_defined_and_relative(struct Registers *regs) {
    return regs->defined[2] != NULL && regs->defined[3] != NULL && regs->relative & 0x20;
}

int is_register_dx_defined_and_relative(struct Registers *regs) {
    return regs->defined[4] != NULL && regs->defined[5] != NULL && regs->relative & 0x40;
}

int is_register_bx_defined_and_relative(struct Registers *regs) {
    return regs->defined[6] != NULL && regs->defined[7] != NULL && regs->relative & 0x80;
}

int is_register_sp_defined_and_relative(struct Registers *regs) {
    return regs->defined[8] != NULL && regs->relative & 0x100;
}

int is_register_bp_defined_and_relative(struct Registers *regs) {
    return regs->defined[9] != NULL && regs->relative & 0x200;
}

int is_register_si_defined_and_relative(struct Registers *regs) {
    return regs->defined[10] != NULL && regs->relative & 0x400;
}

int is_register_di_defined_and_relative(struct Registers *regs) {
    return regs->defined[11] != NULL && regs->relative & 0x800;
}

int is_word_register_defined(struct Registers *regs, unsigned int index) {
    assert(index < 8);
    return (index < 4)? regs->defined[index * 2] != NULL && regs->defined[index * 2 + 1] != NULL : regs->defined[index + 4] != NULL;
}

int is_word_register_defined_and_relative(struct Registers *regs, unsigned int index) {
    assert(index < 8);
    return regs->relative & (0x10 << index) && is_word_register_defined(regs, index);
}

int is_register_es_defined(struct Registers *regs) {
    return regs->defined[12] != NULL;
}

int is_register_cs_defined(struct Registers *regs) {
    return regs->defined[13] != NULL;
}

int is_register_ss_defined(struct Registers *regs) {
    return regs->defined[14] != NULL;
}

int is_register_ds_defined(struct Registers *regs) {
    return regs->defined[15] != NULL;
}

int is_segment_register_defined(struct Registers *regs, unsigned int index) {
    assert(index < 4);
    return regs->defined[index + 12] != NULL;
}

int is_segment_register_defined_and_absolute(struct Registers *regs, unsigned int index) {
    assert(index < 4);
    return (regs->relative & (0x1000 << index)) == 0 && is_segment_register_defined(regs, index);
}

int is_register_es_defined_and_relative(struct Registers *regs) {
    return regs->relative & 0x1000 && is_register_es_defined(regs);
}

int is_register_cs_defined_and_relative(struct Registers *regs) {
    return regs->relative & 0x2000 && is_register_cs_defined(regs);
}

int is_register_ss_defined_and_relative(struct Registers *regs) {
    return regs->relative & 0x4000 && is_register_ss_defined(regs);
}

int is_register_ds_defined_and_relative(struct Registers *regs) {
    return regs->relative & 0x8000 && is_register_ds_defined(regs);
}

int is_segment_register_defined_and_relative(struct Registers *regs, unsigned int index) {
    assert(index < 4);
    return regs->relative & (0x1000 << index) && is_segment_register_defined(regs, index);
}

const char *where_register_ax_defined(struct Registers *regs) {
    const char *al_defined = regs->defined[0];
    return (al_defined == regs->defined[1])? al_defined : NULL;
}

const char *where_register_dx_defined(struct Registers *regs) {
    const char *dl_defined = regs->defined[4];
    return (dl_defined == regs->defined[5])? dl_defined : NULL;
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

void set_byte_register(struct Registers *regs, unsigned int index, const char *where, unsigned char value) {
    assert(index < 8);

    // This should be optimised using the regs pointer plus index to determinate the byte within
    // the struct to change. But for now I implement it in a way that I can ensure it works in any
    // architecture.
    if (index == 0) {
        regs->al = value;
        regs->defined[0] = where;
    }
    else if (index == 1) {
        regs->cl = value;
        regs->defined[2] = where;
    }
    else if (index == 2) {
        regs->dl = value;
        regs->defined[4] = where;
    }
    else if (index == 3) {
        regs->bl = value;
        regs->defined[6] = where;
    }
    else if (index == 4) {
        regs->ah = value;
        regs->defined[1] = where;
    }
    else if (index == 5) {
        regs->ch = value;
        regs->defined[3] = where;
    }
    else if (index == 6) {
        regs->dh = value;
        regs->defined[5] = where;
    }
    else {
        // Assuming index == 7
        regs->bh = value;
        regs->defined[7] = where;
    }
}

void set_word_register(struct Registers *regs, unsigned int index, const char *where, uint16_t value) {
    assert(index < 8);

    // This should be optimised using the regs pointer plus index to determinate the byte within
    // the struct to change. But for now I implement it in a way that I can ensure it works in any
    // architecture.
    if (index == 0) {
        regs->al = value & 0xFF;
        regs->ah = (value >> 8) & 0xFF;
        regs->defined[0] = where;
        regs->defined[1] = where;
        regs->relative &= ~0x10;
    }
    else if (index == 1) {
        regs->cl = value & 0xFF;
        regs->ch = (value >> 8) & 0xFF;
        regs->defined[2] = where;
        regs->defined[3] = where;
        regs->relative &= ~0x20;
    }
    else if (index == 2) {
        regs->dl = value & 0xFF;
        regs->dh = (value >> 8) & 0xFF;
        regs->defined[4] = where;
        regs->defined[5] = where;
        regs->relative &= ~0x40;
    }
    else if (index == 3) {
        regs->bl = value & 0xFF;
        regs->bh = (value >> 8) & 0xFF;
        regs->defined[6] = where;
        regs->defined[7] = where;
        regs->relative &= ~0x80;
    }
    else if (index == 4) {
        regs->sp = value;
        regs->defined[8] = where;
        regs->relative &= ~0x100;
    }
    else if (index == 5) {
        regs->bp = value;
        regs->defined[9] = where;
        regs->relative &= ~0x200;
    }
    else if (index == 6) {
        regs->si = value;
        regs->defined[10] = where;
        regs->relative &= ~0x400;
    }
    else {
        // Assuming index == 7
        regs->di = value;
        regs->defined[11] = where;
        regs->relative &= ~0x800;
    }
}

void set_word_register_relative(struct Registers *regs, unsigned int index, const char * where, uint16_t value) {
    assert(index < 8);

    // This should be optimised using the regs pointer plus index to determinate the byte within
    // the struct to change. But for now I implement it in a way that I can ensure it works in any
    // architecture.
    if (index == 0) {
        regs->al = value & 0xFF;
        regs->ah = (value >> 8) & 0xFF;
        regs->defined[0] = where;
        regs->defined[1] = where;
        regs->relative |= 0x10;
    }
    else if (index == 1) {
        regs->cl = value & 0xFF;
        regs->ch = (value >> 8) & 0xFF;
        regs->defined[2] = where;
        regs->defined[3] = where;
        regs->relative |= 0x20;
    }
    else if (index == 2) {
        regs->dl = value & 0xFF;
        regs->dh = (value >> 8) & 0xFF;
        regs->defined[4] = where;
        regs->defined[5] = where;
        regs->relative |= 0x40;
    }
    else if (index == 3) {
        regs->bl = value & 0xFF;
        regs->bh = (value >> 8) & 0xFF;
        regs->defined[6] = where;
        regs->defined[7] = where;
        regs->relative |= 0x80;
    }
    else if (index == 4) {
        regs->sp = value;
        regs->defined[8] = where;
        regs->relative |= 0x100;
    }
    else if (index == 5) {
        regs->bp = value;
        regs->defined[9] = where;
        regs->relative |= 0x200;
    }
    else if (index == 6) {
        regs->si = value;
        regs->defined[10] = where;
        regs->relative |= 0x400;
    }
    else {
        // Assuming index == 7
        regs->di = value;
        regs->defined[11] = where;
        regs->relative |= 0x800;
    }
}

void mark_word_register_undefined(struct Registers *regs, unsigned int index) {
    assert(index < 8);

    if (index < 4) {
        regs->defined[index * 2] = NULL;
        regs->defined[index * 2 + 1] = NULL;
    }
    else {
        regs->defined[index + 4] = NULL;
    }
}

void set_register_al_undefined(struct Registers *regs) {
    regs->defined[0] = NULL;
}

void set_register_ax_undefined(struct Registers *regs) {
    regs->defined[0] = NULL;
    regs->defined[1] = NULL;
}

void set_register_es(struct Registers *regs, const char *where, uint16_t value) {
    regs->es = value;
    regs->defined[12] = where;
    regs->relative &= ~0x1000;
}

void set_register_cs(struct Registers *regs, const char *where, uint16_t value) {
    regs->cs = value;
    regs->defined[13] = where;
    regs->relative &= ~0x2000;
}

void set_register_ss(struct Registers *regs, const char *where, uint16_t value) {
    regs->ss = value;
    regs->defined[14] = where;
    regs->relative &= ~0x4000;
}

void set_register_ds(struct Registers *regs, const char *where, uint16_t value) {
    regs->ds = value;
    regs->defined[15] = where;
    regs->relative &= ~0x8000;
}

void set_segment_register(struct Registers *regs, unsigned int index, const char *where, uint16_t value) {
    assert(index < 4);

    if (index == 0) {
        set_register_es(regs, where, value);
    }
    else if (index == 1) {
        set_register_cs(regs, where, value);
    }
    else if (index == 2) {
        set_register_ss(regs, where, value);
    }
    else {
        // Assuming index == 3
        set_register_ds(regs, where, value);
    }
}

void mark_segment_register_undefined(struct Registers *regs, unsigned int index) {
    assert(index < 4);
    regs->defined[index + 12] = NULL;
}

void set_register_es_relative(struct Registers *regs, const char *where, uint16_t value) {
    regs->es = value;
    regs->defined[12] = where;
    regs->relative |= 0x1000;
}

void set_register_cs_relative(struct Registers *regs, const char *where, uint16_t value) {
    regs->cs = value;
    regs->defined[13] = where;
    regs->relative |= 0x2000;
}

void set_register_ss_relative(struct Registers *regs, const char *where, uint16_t value) {
    regs->ss = value;
    regs->defined[14] = where;
    regs->relative |= 0x4000;
}

void set_register_ds_relative(struct Registers *regs, const char *where, uint16_t value) {
    regs->ds = value;
    regs->defined[15] = where;
    regs->relative |= 0x8000;
}

void set_segment_register_relative(struct Registers *regs, unsigned int index, const char *where, uint16_t value) {
    assert(index < 4);

    if (index == 0) {
        set_register_es_relative(regs, where, value);
    }
    else if (index == 1) {
        set_register_cs_relative(regs, where, value);
    }
    else if (index == 2) {
        set_register_ss_relative(regs, where, value);
    }
    else {
        // Assuming index == 3
        set_register_ds_relative(regs, where, value);
    }
}

void copy_registers(struct Registers *target_regs, const struct Registers *source_regs) {
    // TODO: Optimise this copy
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
    target_regs->relative = source_regs->relative;

    for (int i = 0; i < 16; i++) {
        target_regs->defined[i] = source_regs->defined[i];
    }
}

void merge_registers(struct Registers *regs, const struct Registers *other_regs) {
    // AX
    if (regs->defined[0] && (!other_regs->defined[0] || regs->al != other_regs->al)) {
        regs->defined[0] = NULL;
    }

    if (regs->defined[1] && (!other_regs->defined[1] || regs->ah != other_regs->ah)) {
        regs->defined[1] = NULL;
    }

    if (regs->defined[0] && regs->defined[1] && (regs->relative & 0x10) != (other_regs->relative & 0x10)) {
        regs->defined[0] = NULL;
        regs->defined[1] = NULL;
    }

    // CX
    if (regs->defined[2] && (!other_regs->defined[2] || regs->cl != other_regs->cl)) {
        regs->defined[2] = NULL;
    }

    if (regs->defined[3] && (!other_regs->defined[3] || regs->ch != other_regs->ch)) {
        regs->defined[3] = NULL;
    }

    if (regs->defined[2] && regs->defined[3] && (regs->relative & 0x20) != (other_regs->relative & 0x20)) {
        regs->defined[2] = NULL;
        regs->defined[3] = NULL;
    }

    // DX
    if (regs->defined[4] && (!other_regs->defined[4] || regs->dl != other_regs->dl)) {
        regs->defined[4] = NULL;
    }

    if (regs->defined[5] && (!other_regs->defined[5] || regs->dh != other_regs->dh)) {
        regs->defined[5] = NULL;
    }

    if (regs->defined[4] && regs->defined[5] && (regs->relative & 0x40) != (other_regs->relative & 0x40)) {
        regs->defined[4] = NULL;
        regs->defined[5] = NULL;
    }

    // BX
    if (regs->defined[6] && (!other_regs->defined[6] || regs->bl != other_regs->bl)) {
        regs->defined[6] = NULL;
    }

    if (regs->defined[7] && (!other_regs->defined[7] || regs->bh != other_regs->bh)) {
        regs->defined[7] = NULL;
    }

    if (regs->defined[6] && regs->defined[7] && (regs->relative & 0x80) != (other_regs->relative & 0x80)) {
        regs->defined[6] = NULL;
        regs->defined[7] = NULL;
    }

    // SP
    if (regs->defined[8] && (!other_regs->defined[8] || regs->sp != other_regs->sp || (regs->relative & 0x100) != (other_regs->relative & 0x100))) {
        regs->defined[8] = NULL;
    }

    // BP
    if (regs->defined[9] && (!other_regs->defined[9] || regs->bp != other_regs->bp || (regs->relative & 0x200) != (other_regs->relative & 0x200))) {
        regs->defined[9] = NULL;
    }

    // SI
    if (regs->defined[10] && (!other_regs->defined[10] || regs->si != other_regs->si || (regs->relative & 0x400) != (other_regs->relative & 0x400))) {
        regs->defined[10] = NULL;
    }

    // DI
    if (regs->defined[11] && (!other_regs->defined[11] || regs->di != other_regs->di || (regs->relative & 0x800) != (other_regs->relative & 0x800))) {
        regs->defined[11] = NULL;
    }

    // ES
    if (regs->defined[12] && (!other_regs->defined[12] || regs->es != other_regs->es || (regs->relative & 0x1000) != (other_regs->relative & 0x1000))) {
        regs->defined[12] = NULL;
    }

    // CS
    if (regs->defined[13] && (!other_regs->defined[13] || regs->cs != other_regs->cs || (regs->relative & 0x2000) != (other_regs->relative & 0x2000))) {
        regs->defined[13] = NULL;
    }

    // SS
    if (regs->defined[14] && (!other_regs->defined[14] || regs->ss != other_regs->ss || (regs->relative & 0x4000) != (other_regs->relative & 0x4000))) {
        regs->defined[14] = NULL;
    }

    // DS
    if (regs->defined[15] && (!other_regs->defined[15] || regs->ds != other_regs->ds || (regs->relative & 0x8000) != (other_regs->relative & 0x8000))) {
        regs->defined[15] = NULL;
    }
}

int changes_on_merging_registers(const struct Registers *regs, const struct Registers *other_regs) {
    // AX
    if (regs->defined[0] && (!other_regs->defined[0] || regs->al != other_regs->al)) {
        return 1;
    }

    if (regs->defined[1] && (!other_regs->defined[1] || regs->ah != other_regs->ah)) {
        return 1;
    }

    if (regs->defined[0] && regs->defined[1] && (regs->relative & 0x10) != (other_regs->relative & 0x10)) {
        return 1;
    }

    // CX
    if (regs->defined[2] && (!other_regs->defined[2] || regs->cl != other_regs->cl)) {
        return 1;
    }

    if (regs->defined[3] && (!other_regs->defined[3] || regs->ch != other_regs->ch)) {
        return 1;
    }

    if (regs->defined[2] && regs->defined[3] && (regs->relative & 0x20) != (other_regs->relative & 0x20)) {
        return 1;
    }

    // DX
    if (regs->defined[4] && (!other_regs->defined[4] || regs->dl != other_regs->dl)) {
        return 1;
    }

    if (regs->defined[5] && (!other_regs->defined[5] || regs->dh != other_regs->dh)) {
        return 1;
    }

    if (regs->defined[4] && regs->defined[5] && (regs->relative & 0x40) != (other_regs->relative & 0x40)) {
        return 1;
    }

    // BX
    if (regs->defined[6] && (!other_regs->defined[6] || regs->bl != other_regs->bl)) {
        return 1;
    }

    if (regs->defined[7] && (!other_regs->defined[7] || regs->bh != other_regs->bh)) {
        return 1;
    }

    if (regs->defined[6] && regs->defined[7] && (regs->relative & 0x80) != (other_regs->relative & 0x80)) {
        return 1;
    }

    // SP
    if (regs->defined[8] && (!other_regs->defined[8] || regs->sp != other_regs->sp || (regs->relative & 0x100) != (other_regs->relative & 0x100))) {
        return 1;
    }

    // BP
    if (regs->defined[9] && (!other_regs->defined[9] || regs->bp != other_regs->bp || (regs->relative & 0x200) != (other_regs->relative & 0x200))) {
        return 1;
    }

    // SI
    if (regs->defined[10] && (!other_regs->defined[10] || regs->si != other_regs->si || (regs->relative & 0x400) != (other_regs->relative & 0x400))) {
        return 1;
    }

    // DI
    if (regs->defined[11] && (!other_regs->defined[11] || regs->di != other_regs->di || (regs->relative & 0x800) != (other_regs->relative & 0x800))) {
        return 1;
    }

    // ES
    if (regs->defined[12] && (!other_regs->defined[12] || regs->es != other_regs->es || (regs->relative & 0x1000) != (other_regs->relative & 0x1000))) {
        return 1;
    }

    // CS
    if (regs->defined[13] && (!other_regs->defined[13] || regs->cs != other_regs->cs || (regs->relative & 0x2000) != (other_regs->relative & 0x2000))) {
        return 1;
    }

    // SS
    if (regs->defined[14] && (!other_regs->defined[14] || regs->ss != other_regs->ss || (regs->relative & 0x4000) != (other_regs->relative & 0x4000))) {
        return 1;
    }

    // DS
    if (regs->defined[15] && (!other_regs->defined[15] || regs->ds != other_regs->ds || (regs->relative & 0x8000) != (other_regs->relative & 0x8000))) {
        return 1;
    }

    return 0;
}

void make_all_registers_undefined(struct Registers *regs) {
    for (int i = 0; i < 16; i++) {
        regs->defined[i] = NULL;
    }
}
