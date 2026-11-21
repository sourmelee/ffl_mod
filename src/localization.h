/* English overlays for the supported universal game. Never change the owning
 * MyLocalStringArray: it frees its own strings when the story bank reloads. */
#include "english_text.h"
static volatile int g_displayLanguage = -1;
static int patch_slot(uintptr_t slot, uintptr_t value);

static int text_equal(const char *a, const char *b)
{
    if (!a || !b) return a == b;
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}
static int text_english(void *game) { return game && RD(int, game, 0x0c) == 1; }
static const char *native_text(void *game, const char *sjis, const char *utf8)
{
    if (text_english(game)) {
        for (unsigned i = 0; i < sizeof(k_nativeEnglish) / sizeof(k_nativeEnglish[0]); i++)
            if (text_equal(utf8, k_nativeEnglish[i].japanese)) return k_nativeEnglish[i].english;
    }
    return sjis;
}
/* The caller supplies its Japanese encoding choice as the fallback. */
#define LOCAL_TEXT(game, sjis, utf8, sj) native_text((game), (sj) ? (sjis) : (utf8), (utf8))

static void english_story_apply(void *game, void *field)
{
    if (!game || !field) return;
    int bank = RD(int, game, 0x19fe0);
    if (bank < 0 || bank >= 16) return;
    const char **owned = (const char **)RDP(game, 0x25358);
    const char **text = (const char **)RDP(game, 0x25370);
    const char **names = (const char **)RDP(game, 0x25378);
    int count = RD(int, game, 0x25368);
    if (!owned || !text || !names || count != 2 * k_englishBanks[bank].count ||
        RDP(field, 0x380) != text || RDP(field, 0x388) != names) return;
    int english = text_english(game);
    if (english) {
        for (int k = 0; k < 2; k++)
            if (!text_equal(owned[2 * k_englishBanks[bank].id[k]], k_englishBanks[bank].english[k])) return;
    }
    for (unsigned i = 0; i < sizeof(k_englishStory) / sizeof(k_englishStory[0]); i++) {
        const EnglishStory *r = &k_englishStory[i];
        if (r->bank != bank || r->id < 0 || r->id >= count / 2) continue;
        const char **slot = (r->speaker ? names : text) + r->id;
        const char *original = owned[2 * r->id + r->speaker];
        if (english) {
            if (*slot == r->english) continue;
            if (*slot == original && text_equal(original, r->expected)) *slot = r->english;
        } else if (*slot == r->english) *slot = original;
    }
}

typedef void (*MessageListFn)(void *, const char **, const char **);
typedef const char *(*MapNameFn)(void *, int);
static MessageListFn g_origMessageList;
static MapNameFn g_origMapName;
static void hook_MessageList(void *field, const char **text, const char **names)
{
    g_origMessageList(field, text, names);
    void *game = game_inst();
    english_story_apply(game, field);
}
static const char *hook_MapName(void *game, int id)
{
    const char *name = g_origMapName(game, id);
    if (text_english(game) && id == 165 && RD(int, game, 0x1a030) > id && name && !*name)
        return k_challengeNameEnglish;
    return name;
}

/* Resolve the import by its ELF dynamic symbol, rather than introducing more
 * version-specific GOT offsets. Existing install() binary checks still apply.
 * All pointers and record sizes are checked against this image's load segments. */
static int image_range(uintptr_t p, size_t n, uintptr_t end)
{
    return p >= g_base && p <= end && n <= end - p;
}
static uintptr_t image_pointer(uintptr_t v) { return v < g_base ? g_base + v : v; }
static int english_import(void *handle, const char *name, uintptr_t hook, void **original)
{
    const u8 *elf = (const u8 *)g_base;
    if (RD(unsigned, elf, 0) != 0x464c457f || elf[4] != 2 || elf[5] != 1 ||
        RD(unsigned short, elf, 18) != 183 || RD(unsigned short, elf, 54) != 56) return 0;
    uintptr_t phoff = RD(uintptr_t, elf, 32), end = g_base, dynamic = 0; size_t dynsize = 0;
    unsigned nph = RD(unsigned short, elf, 56);
    if (phoff > 4096 || nph > 64) return 0;
    for (unsigned i = 0; i < nph; i++) {
        const u8 *ph = elf + phoff + 56 * i;
        uintptr_t vaddr = RD(uintptr_t, ph, 16), memsize = RD(uintptr_t, ph, 40);
        if (vaddr > 0x10000000 || memsize > 0x10000000) return 0;
        if (RD(unsigned, ph, 0) == 1 && g_base + vaddr + memsize > end) end = g_base + vaddr + memsize;
        if (RD(unsigned, ph, 0) == 2) { dynamic = g_base + vaddr; dynsize = memsize; }
    }
    if (!dynamic || !image_range(dynamic, dynsize, end) || dynsize / 16 > 4096) return 0;
    uintptr_t strings = 0, symbols = 0, relocs = 0; size_t strsize = 0, relsize = 0, syment = 0; int rela = 0;
    for (size_t i = 0; i < dynsize / 16; i++) {
        uintptr_t tag = RD(uintptr_t, dynamic, 16 * i), value = RD(uintptr_t, dynamic, 16 * i + 8);
        if (!tag) break;
        switch (tag) {
        case 5: strings = image_pointer(value); break;
        case 6: symbols = image_pointer(value); break;
        case 10: strsize = value; break;
        case 11: syment = value; break;
        case 23: relocs = image_pointer(value); break;
        case 2: relsize = value; break;
        case 20: rela = value == 7; break;
        }
    }
    if (!rela || syment != 24 || relsize % 24 || !image_range(strings, strsize, end) ||
        !image_range(symbols, 24, end) || !image_range(relocs, relsize, end)) return 0;
    uintptr_t expected = (uintptr_t)dlsym(handle, name);
    if (!expected) return 0;
    for (size_t p = 0; p < relsize; p += 24) {
        uintptr_t info = RD(uintptr_t, relocs, p + 8), index = info >> 32;
        if ((unsigned)info != 1026 || index > (end - symbols) / 24 || !image_range(symbols + index * 24, 24, end)) continue;
        unsigned offset = RD(unsigned, symbols + index * 24, 0);
        if (offset >= strsize) continue;
        const char *candidate = (const char *)(strings + offset); size_t k = 0;
        while (k < strsize - offset && candidate[k] && name[k] && candidate[k] == name[k]) k++;
        if (k >= strsize - offset || candidate[k] || name[k]) continue;
        uintptr_t slot = image_pointer(RD(uintptr_t, relocs, p));
        if (!image_range(slot, sizeof(uintptr_t), end) || RD(uintptr_t, slot, 0) != expected) return 0;
        *original = (void *)expected;
        return patch_slot(slot, hook);
    }
    return 0;
}
static void english_install(void *handle)
{
    int messages = english_import(handle, "_ZN10FieldClass14SetMessageListEPPKcS2_", (uintptr_t)hook_MessageList, (void **)&g_origMessageList);
    int maps = english_import(handle, "_ZN9GameClass10GetMapNameEi", (uintptr_t)hook_MapName, (void **)&g_origMapName);
    LOGI("English: message hook %d map-name hook %d", messages, maps);
}
