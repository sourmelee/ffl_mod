/* Host tests exercise the actual overlay code against synthetic game layouts.
 * No APK, OBB, or game strings/files are required. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char u8;
static uintptr_t g_base;
static void *test_game;
static void *game_inst(void) { return test_game; }
static void *expected_import;
static void *dlsym(void *handle, const char *name) { (void)handle; (void)name; return expected_import; }
#define LOGI(...) ((void)0)
#define RD(T,p,o) (*(T *)((uintptr_t)(p)+(o)))
#define RDP(p,o) ((void *)RD(uintptr_t,p,o))
#include "../src/localization.h"
static int patch_slot(uintptr_t slot, uintptr_t value) { RD(uintptr_t,slot,0)=value; return 1; }

static const char *test_map_name(void *game, int id) { (void)game; return id == 165 ? "" : "Other map"; }

static void test_story_bank(int bank)
{
    int n = k_englishBanks[bank].count;
    u8 *game = calloc(1, 0x25400), *field = calloc(1, 0x400);
    const char **owned = malloc(2 * n * sizeof(char *));
    const char **text = malloc(n * sizeof(char *)), **names = malloc(n * sizeof(char *));
    const char **snapshot = malloc(2 * n * sizeof(char *));
    assert(game && field && owned && text && names && snapshot);
    for (int i = 0; i < 2 * n; i++) owned[i] = "";
    for (int k = 0; k < 2; k++) owned[2 * k_englishBanks[bank].id[k]] = k_englishBanks[bank].english[k];
    int changes = 0;
    for (unsigned i = 0; i < sizeof(k_englishStory)/sizeof(k_englishStory[0]); i++) {
        const EnglishStory *r = &k_englishStory[i];
        if (r->bank == bank) { owned[2*r->id+r->speaker] = r->expected; changes++; }
    }
    for (int i = 0; i < n; i++) { text[i]=owned[2*i]; names[i]=owned[2*i+1]; }
    memcpy(snapshot, owned, 2*n*sizeof(char *));
    RD(int,game,0x0c)=1; RD(int,game,0x19fe0)=bank;
    RD(int,game,0x25368)=2*n;
    RD(const char **,game,0x25358)=owned;
    RD(const char **,game,0x25370)=text; RD(const char **,game,0x25378)=names;
    RD(const char **,field,0x380)=text; RD(const char **,field,0x388)=names;
    test_game=game;

    /* Japanese is untouched; English must match the expected source bank. */
    RD(int,game,0x0c)=0; english_story_apply(game,field);
    for(int i=0;i<n;i++) { assert(text[i]==owned[2*i]); assert(names[i]==owned[2*i+1]); }
    RD(int,game,0x0c)=1;
    const char *fingerprint=owned[2*k_englishBanks[bank].id[0]];
    owned[2*k_englishBanks[bank].id[0]]="incompatible bank";
    english_story_apply(game,field);
    for(int i=0;i<n;i++) { assert(text[i]==snapshot[2*i]); assert(names[i]==snapshot[2*i+1]); }
    owned[2*k_englishBanks[bank].id[0]]=fingerprint;
    RD(int,game,0x25368)=2*n-1; english_story_apply(game,field);
    for(int i=0;i<n;i++) { assert(text[i]==owned[2*i]); assert(names[i]==owned[2*i+1]); }
    RD(int,game,0x25368)=2*n;
    RD(const char **,field,0x380)=names; english_story_apply(game,field);
    for(int i=0;i<n;i++) { assert(text[i]==owned[2*i]); assert(names[i]==owned[2*i+1]); }
    RD(const char **,field,0x380)=text;

    english_story_apply(game,field);
    english_story_apply(game,field); /* Repeated ticks are idempotent. */
    int actual=0;
    for(int i=0;i<n;i++) { actual+=text[i]!=owned[2*i]; actual+=names[i]!=owned[2*i+1]; }
    assert(actual==changes);
    for(unsigned i=0;i<sizeof(k_englishStory)/sizeof(k_englishStory[0]);i++) {
        const EnglishStory *r=&k_englishStory[i];
        if(r->bank==bank) assert((r->speaker?names:text)[r->id]==r->english);
    }
    assert(!memcmp(snapshot,owned,2*n*sizeof(char *)));
    RD(int,game,0x0c)=0; english_story_apply(game,field);
    for(int i=0;i<n;i++) { assert(text[i]==owned[2*i]); assert(names[i]==owned[2*i+1]); }

    /* A different overlay's nonempty entry must not be overwritten. */
    RD(int,game,0x0c)=1;
    for(unsigned i=0;i<sizeof(k_englishStory)/sizeof(k_englishStory[0]);i++) {
        const EnglishStory *r=&k_englishStory[i];
        if(r->bank!=bank) continue;
        const char **slot=(r->speaker?names:text)+r->id;
        *slot="another mod"; english_story_apply(game,field);
        assert(text_equal(*slot,"another mod"));
        break;
    }
    free(snapshot); free(names); free(text); free(owned); free(field); free(game);
    test_game=NULL;
}

static void test_native_text(void)
{
    u8 game[32]={0};
    for(unsigned i=0;i<sizeof(k_nativeEnglish)/sizeof(k_nativeEnglish[0]);i++) {
        RD(int,game,0x0c)=1;
        assert(native_text(game,"SJIS",k_nativeEnglish[i].japanese)==k_nativeEnglish[i].english);
        RD(int,game,0x0c)=0;
        assert(text_equal(native_text(game,"SJIS",k_nativeEnglish[i].japanese),"SJIS"));
    }
    assert(text_equal(native_text(NULL,"JP","unknown"),"JP"));
    u8 *full=calloc(1,0x1a040); assert(full);
    g_origMapName=test_map_name; RD(int,full,0x1a030)=200;
    RD(int,full,0x0c)=1;
    assert(hook_MapName(full,165)==k_challengeNameEnglish);
    assert(text_equal(hook_MapName(full,2),"Other map"));
    RD(int,full,0x0c)=0; assert(text_equal(hook_MapName(full,165),""));
    free(full);
}

static void test_import_resolution(void)
{
    u8 *image=calloc(1,0x10000); assert(image); g_base=(uintptr_t)image;
    RD(unsigned,image,0)=0x464c457f; image[4]=2; image[5]=1;
    RD(unsigned short,image,18)=183;
    RD(uintptr_t,image,32)=64; RD(unsigned short,image,54)=56; RD(unsigned short,image,56)=2;
    RD(unsigned,image,64)=1; RD(uintptr_t,image,64+40)=0x10000;
    RD(unsigned,image,120)=2; RD(uintptr_t,image,120+16)=0x200; RD(uintptr_t,image,120+40)=128;
    const uintptr_t tags[]={5,6,10,11,23,2,20,0}, values[]={0x400,0x500,32,24,0x600,24,7,0};
    for(int i=0;i<8;i++) { RD(uintptr_t,image,0x200+i*16)=tags[i]; RD(uintptr_t,image,0x208+i*16)=values[i]; }
    memcpy(image+0x400,"test_symbol",sizeof("test_symbol"));
    RD(unsigned,image,0x500+24)=0;
    RD(uintptr_t,image,0x600)=0x700;
    RD(uintptr_t,image,0x608)=((uintptr_t)1<<32)|1026;
    expected_import=(void *)(image+0x800); RD(uintptr_t,image,0x700)=(uintptr_t)expected_import;
    void *original=NULL;
    assert(english_import(NULL,"test_symbol",g_base+0x900,&original));
    assert(original==expected_import); assert(RD(uintptr_t,image,0x700)==g_base+0x900);
    assert(!english_import(NULL,"test_symbol",g_base+0x900,&original));
    RD(uintptr_t,image,0x700)=(uintptr_t)expected_import;
    assert(!english_import(NULL,"wrong_symbol",g_base+0x900,&original));
    RD(uintptr_t,image,0x208+3*16)=25;
    assert(!english_import(NULL,"test_symbol",g_base+0x900,&original));
    free(image); g_base=0;
}

int main(void)
{
    for(int bank=0;bank<16;bank++) test_story_bank(bank);
    test_native_text(); test_import_resolution();
    puts("PASS: all 16 banks, 161 overlays, 23 native strings, language restoration, guards, map names and ELF import validation");
    return 0;
}
