/* Host-only test of the ffd_nx two-address relocation model.
 * Compile: cc -std=c11 -Wall -Wextra -Werror -pedantic
 *          platforms/switch/test_staging_hooks.c -o /tmp/ffd-nx-hooks
 */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct so_module {
    void *load_base;      /* relocated, writable staging image */
    void *load_virtbase;  /* not yet mapped */
    size_t load_size;
    void *syms;
    char *dynstrtab;
} so_module;
static uintptr_t so_try_find_addr_rx(so_module *mod, const char *sym);

#include "ffd_switch_profile.h"

static uintptr_t so_try_find_addr_rx(so_module *mod, const char *sym)
{
    for (unsigned i = 0; i < FFD_SWITCH_SITE_COUNT; i++)
        if (!strcmp(sym, ffd_switch_sites[i].symbol))
            return (uintptr_t)mod->load_virtbase + ffd_switch_sites[i].entry_offset;
    return 0;
}

int main(void)
{
    const size_t size = 0x500000;
    unsigned char *staging = calloc(1, size);
    assert(staging);
    so_module mod = { staging, (void *)(uintptr_t)0x200000000ULL,
                      size, (void *)1, (char *)1 };
    const unsigned menu = FFD_SWITCH_MENU_DRAW;
    const unsigned gfx = FFD_SWITCH_GFX_VIEWPORT;
    uintptr_t *a = (uintptr_t *)(staging + ffd_switch_sites[menu].got_offset);
    uintptr_t *b = (uintptr_t *)(staging + ffd_switch_sites[gfx].got_offset);
    const uintptr_t original_a = (uintptr_t)mod.load_virtbase +
                                 ffd_switch_sites[menu].entry_offset;
    const uintptr_t original_b = (uintptr_t)mod.load_virtbase +
                                 ffd_switch_sites[gfx].entry_offset;
    *a = original_a;
    *b = original_b;
    assert(ffd_switch_validate_site(&mod, menu));
    assert(ffd_switch_validate_site(&mod, gfx));

    uintptr_t saved_a = 0, saved_b = 0;
    FfdSwitchPatch group[] = {
        { menu, (uintptr_t)0x12345678u, &saved_a },
        { gfx, (uintptr_t)0x99887766u, &saved_b }
    };

    /* A stale second GOT must prevent ALL writes. */
    *b = 0xdeadbeefu;
    assert(!ffd_switch_install_patches(&mod, group, 2));
    assert(*a == original_a && saved_a == 0 && saved_b == 0);
    *b = original_b;

    assert(ffd_switch_install_patches(&mod, group, 2));
    assert(*a == 0x12345678u && *b == 0x99887766u);
    assert(saved_a == original_a && saved_b == original_b);
    assert(!ffd_switch_validate_site(&mod, menu)); /* already hooked */
    *a = original_a;
    *b = original_b;

    group[1].site_index = menu; /* duplicate site */
    assert(!ffd_switch_install_patches(&mod, group, 2));
    assert(*a == original_a);

    free(staging);
    puts("Switch pre-finalize relocation and all-or-nothing guards: PASS");
    return 0;
}
