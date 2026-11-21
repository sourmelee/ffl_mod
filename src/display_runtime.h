#ifndef FFLMOD_DISPLAY_RUNTIME_H
#define FFLMOD_DISPLAY_RUNTIME_H
#include "display_geometry.h"
/* ---------- 1.0-pre3: 本当のワイド表示 ----------
   ゲームは MainScene::SetViewPort で 640x384 を縦横同じ倍率 s で真ん中に置き、Set2DModeMatrix(640,384) で描く。
   横長の画面では：
   ① 描く範囲（glViewport）は窓いっぱいにする（hook_ViewPort。縦長の画面は元のまま）
   ② 640x384 の画面は ortho(-e, 640+e, 384, 0) で描く（e = 左右の余白を論理座標で）→ 縦横同じ倍率のまま真ん中に。
      文字（DrawString は論理座標で中央寄せしてから画素に直す）も絵もゆがまない
   ③ MainScene::SetViewPort の後で、タッチの換算（+0x674 倍率、+0x67c ずれ）をこの置き方に合わせ、
      App の絵の補正（+0x188/+0x18c、引き伸ばしの打ち消し）を 1 にする
   ④ フィールドはゲームが画面の比率から横幅を出す（W/s）ので、そのまま広く描かせる。遠景（DrawFar）だけは幅 640 の絵なので、
      横幅 640 の座標で描いて画面いっぱいに伸ばす
   ⑤ 640 の幅いっぱいに描く絵と塗り（戦闘の背景、メニューの背景、暗転など。左端 x<=0 かつ右端 >=640）は、
      真ん中を軸に横へ (640+2e)/640 倍に伸ばして左右の余白を埋める */
#define OFF_VIEWPORT_FUNC 0x370f3cUL   /* MtxGraphics3D::SetViewPort(int,int,int,int) */
#define OFF_VIEWPORT_GOT  0x45d8c0UL
#define OFF_S2DM_FUNC     0x371b98UL   /* MtxGraphics3D::Set2DModeMatrix(float,float) */
#define OFF_S2DM_GOT      0x45d960UL
#define OFF_MSVP_FUNC     0x3f6680UL   /* MainScene::SetViewPort(MtxGraphics3D&) */
#define OFF_MSVP_GOT      0x460360UL
#define OFF_DIEX_FUNC     0x373aa4UL   /* MtxGraphics3D::DrawImageEx(MtxTexture&, 8 float, int, int, int)（DrawImage もここを通る） */
#define OFF_DIEX_GOT      0x45d3a0UL
#define OFF_FILL4_FUNC    0x372c4cUL   /* MtxGraphics3D::FillRect(int,int,int,int) */
#define OFF_FILL4_GOT     0x45d590UL
#define OFF_FILL8_FUNC    0x372d44UL   /* MtxGraphics3D::FillRect(int,int,int,int,int,int,int,int) */
#define OFF_FILL8_GOT     0x45d9f0UL
#define OFF_DRAWFAR_FUNC  0x3c97dcUL   /* FieldClass::DrawFar(MtxGraphics3D*) */
#define OFF_DRAWFAR_GOT   0x45f7a8UL
#define OFF_FSET2D        0x3b5a58UL   /* FieldClass::Set2DMode(MtxGraphics3D*)（直に呼ぶ） */
#define OFF_SETMMODE      0x370c98UL   /* MtxGraphics3D::SetMatrixMode(int) */
#define OFF_LOADIDENT     0x370cd4UL   /* MtxGraphics3D::LoadIdentiry() */
#define OFF_GORTHO        0x370d04UL   /* MtxGraphics3D::Ortho(l,r,b,t,n,f)（今の行列に掛けて読み込む） */
#define APP_PTR_GOT       0x45ca58UL   /* App のインスタンスへ（2回たどる） */
#define APP_IMGFIX_X      0x188        /* App: 絵の横の補正（min/sx） */
#define APP_IMGFIX_Y      0x18c
#define GFX_VP            0x4c0        /* MtxGraphics3D: 描く範囲 x, y, w, h（float） */
#define GFX_PROJ          0x5c         /* MtxGraphics3D: 射影の行列（float 16、列優先） */
#define FIELD_W           0xc          /* FieldClass: フィールドの横幅（float） */
#define FIELD_H           0x10
#define WIDE_HOOKS        9            /* フックの表の末尾にあるワイド表示のフックの数 */
#define GFX_SCREENRATIO   0x5b4        /* MtxGraphics3D: 論理座標 / 画素（SetScissor が使う） */
#define OFF_MINIMAP_FUNC  0x3caf38UL   /* FieldClass::DrawMiniMap(MtxGraphics3D*) */
#define OFF_MINIMAP_GOT   0x45f370UL
#define OFF_ESC_X1        0x391350UL   /* BattleClass::BattleScaduleEscape：逃げる行き先 x 680.0（mov w8, #0x442a0000）×2 */
#define OFF_ESC_X2        0x391408UL
#define INSN_MOV_680      0x52a88548U  /* movz w8, #0x442a, lsl #16 */
#define INSN_MOV_800      0x52a88908U  /* movz w8, #0x4448, lsl #16（800.0） */
/* 戦闘の AUTO/PAUSE/ESCAPE の欄（BattleClass の JobsMenuItemListControl、[battle+0x6e40]）。
   部品は ▶ のつまみ（2,2 54x105）とボタン3つ（56,2/37/72 101x35）。
   1.0-pre10: つまみを消し、欄を x 536・幅 104 にして、ボタンを欄の左端に置く（ボタンの画面上の位置は今までと同じ） */
static const struct { unsigned off, orig, now; } k_panelPatch[] = {
    { 0x3842cc, 0x5290000a, 0x5280000a },   /* BattleMain：閉じたときの x 586.0 → 536.0（movz w10,#0x8000 → #0） */
    { 0x3842d4, 0x72a8824a, 0x72a880ca },   /*   movk w10,#0x4412 → #0x4406 */
    { 0x3842ec, 0x5290000a, 0x5280000a },
    { 0x3842f4, 0x72a8824a, 0x72a880ca },
    { 0x384288, 0x52a87e0a, 0x52a880ca },   /* BattleMain：開いたときの x 480.0 → 536.0 */
    { 0x384834, 0x52a87e0a, 0x52a880ca },
    { 0x382914, 0x52804941, 0x52804301 },   /* BattleSceneInit：欄の SetSize の x 586 → 536 */
    { 0x38291c, 0x52801403, 0x52800d03 },   /*   幅 160 → 104 */
    { 0x382958, 0x52800041, 0x52800001 },   /* つまみの部品：種類 2 → 0（JobsMenuButton::Draw が描かない） */
    { 0x382970, 0x528006c7, 0x52800007 },   /*   幅 54 → 0（触っても当たらない） */
    { 0x3829e8, 0x52800705, 0x52800005 },   /* ボタン3つの x 56 → 0 */
    { 0x382a30, 0x52800705, 0x52800005 },
    { 0x382a78, 0x52800705, 0x52800005 },
};
typedef void (*ViewPortFn)(void *g, int x, int y, int w, int h);
typedef void (*S2DMFn)(void *g, float w, float h);
typedef void (*MSVPFn)(void *scene, void *g);
typedef void (*DIExFn)(void *g, void *tex, float x, float y, float w, float h, float u, float v, float uw, float vh,
                       int flags, int color, int mode);
typedef void (*Fill4Fn)(void *g, int x, int y, int w, int h);
typedef void (*Fill8Fn)(void *g, int x, int y, int w, int h, int r, int gg, int b, int a);
typedef void (*DrawFarFn)(void *field, void *g);
typedef void (*MiniMapFn)(void *field, void *g);
typedef void (*FSet2DFn)(void *field, void *g);
typedef void (*SetMModeFn)(void *g, int mode);
typedef void (*LoadIdentFn)(void *g);
typedef void (*GOrthoFn)(void *g, float l, float r, float b, float t, float n, float f);
static ViewPortFn g_origViewPort;
static S2DMFn g_origS2DM;
static MSVPFn g_origMSVP;
static DIExFn g_origDIEx;
static Fill4Fn g_origFill4;
static Fill8Fn g_origFill8;
static DrawFarFn g_origDrawFar;
static MiniMapFn g_origMiniMap;
static float g_wideM0, g_wideM5, g_wideM12, g_wideM13;
typedef int (*DestinationFn)(void *g);
static DestinationFn p_DestinationWidth, p_DestinationHeight;
static int g_inMainViewport, g_destinationW, g_destinationH;
typedef void (*FieldDrawMessageFn)(void *field, void *g);
static FieldDrawMessageFn g_origDrawMessage;
typedef void (*MapFieldFn)(void *field, void *g, float zoom);
static MapFieldFn g_origMapField;
static int g_tallReady;

/* Indirection is overridable by the host harness; Android uses the same
 * validated function offsets as the original widescreen implementation. */
#ifndef DISPLAY_GFX_CALL
#define DISPLAY_GFX_CALL(T, off) ((T)(g_base + (off)))
#endif

static int display_active(void) { return g_wideE > 0.0f || g_tallE > 0.0f; }
static int display_main_surface(void *g)
{
    return !RDP(g, 0x10) && RD(float, g, GFX_VP + 8) == (float)g_destinationW &&
        RD(float, g, GFX_VP + 12) == (float)g_destinationH;
}

static void hook_ViewPort(void *g, int x, int y, int w, int h)
{
    /* Use destination dimensions, not 2*y+h: integer viewport rounding would
     * otherwise leave a one-pixel border on the Nova's 1536x1152 surface. */
    DisplayGeometry requested = display_geometry((float)g_destinationW, (float)g_destinationH);
    if (g_inMainViewport && !RDP(g, 0x10) && requested.active &&
        (requested.y == 0.0f || g_tallReady)) {
        g_origViewPort(g, 0, 0, g_destinationW, g_destinationH);
        return;
    }
    if (g_tallE > 0.0f && !g_inMainViewport) { g_origViewPort(g, x, y, w, h); return; }
    /* 横に余白があるとき（横長の画面）だけ窓いっぱいにする。縦に余白があるとき（縦長）は元のまま */
    if (x <= 0 || y != 0 || w <= 0 || h <= 0) { g_origViewPort(g, x, y, w, h); return; }
    g_origViewPort(g, 0, 0, w + 2 * x, h);
}

static void hook_MSVP(void *scene, void *g)
{
    g_destinationW = p_DestinationWidth ? p_DestinationWidth(g) : 0;
    g_destinationH = p_DestinationHeight ? p_DestinationHeight(g) : 0;
    g_inMainViewport = 1;
    g_origMSVP(scene, g);
    g_inMainViewport = 0;
    float W = RD(float, g, GFX_VP + 8), H = RD(float, g, GFX_VP + 12);
    DisplayGeometry layout = display_geometry(W, H);
    if (g_destinationW > 0 && g_destinationH > 0) {
        DisplayGeometry requested = display_geometry((float)g_destinationW, (float)g_destinationH);
        if (!requested.active) layout = requested;
        else if (W != (float)g_destinationW || H != (float)g_destinationH || RDP(g, 0x10)) {
            DisplayGeometry original = { 1.0f, 0.0f, 0.0f, 0 }; layout = original;
        }
    }
    float e = layout.x, t = layout.y, s = layout.scale;
    if (e != g_wideE || t != g_tallE) {
        static int told;
        if (told < 8) { told++; LOGI("display: screen %dx%d, margins %d,%d (logical)", (int)W, (int)H, (int)e, (int)t); }
    }
    g_wideE = e;
    g_tallE = t;
    g_wideS = s;
    if (!layout.active) return;
    void *tp = p_GetTouchInfo ? p_GetTouchInfo() : 0;
    if (tp) {
        *(volatile float *)((uintptr_t)tp + TP_SCALE) = 1.0f / s;
        *(volatile float *)((uintptr_t)tp + TP_SCALE + 4) = 1.0f / s;
        *(volatile float *)((uintptr_t)tp + TP_OFFSET) = e * s;
        *(volatile float *)((uintptr_t)tp + TP_OFFSET + 4) = t * s;
    }
    uintptr_t v = *(volatile uintptr_t *)(g_base + APP_PTR_GOT);
    void *app = v ? *(void **)v : 0;
    if (app) {
        *(volatile float *)((uintptr_t)app + APP_IMGFIX_X) = 1.0f;
        *(volatile float *)((uintptr_t)app + APP_IMGFIX_Y) = 1.0f;
    }
    /* 1.0-pre4: 切り抜き（SetScissor）は「論理座標 / 倍率 + 描く範囲の左上」で画素に直すので、この置き方に合わせる
       （描く範囲の左上 +0x4c0 は、ほかには MainScene::SetViewPort がタッチのずれに使うだけ。上で書き直している） */
    *(volatile float *)((uintptr_t)g + GFX_VP) = e * s;
    /* Scissor uses bottom-origin OpenGL coordinates, unlike top-origin touch. */
    *(volatile float *)((uintptr_t)g + GFX_VP + 4) = -t * s;
    *(volatile float *)((uintptr_t)g + GFX_SCREENRATIO) = 1.0f / s;
    *(volatile float *)((uintptr_t)g + GFX_SCREENRATIO + 4) = 1.0f / s;
}

/* 2D の射影を ortho(-e - shift, 640 + e - shift, 384, 0) にする（shift > 0 で中身が右へずれる） */
static void wide_proj(void *g, float shiftX, float shiftY)
{
    float e = g_wideE, t = g_tallE;
    DISPLAY_GFX_CALL(SetMModeFn, OFF_SETMMODE)(g, 1);
    DISPLAY_GFX_CALL(LoadIdentFn, OFF_LOADIDENT)(g);
    DISPLAY_GFX_CALL(GOrthoFn, OFF_GORTHO)(g, -e - shiftX, 640.0f + e - shiftX, 384.0f + t - shiftY, -t - shiftY, 0.0f, 1024.0f);
    DISPLAY_GFX_CALL(SetMModeFn, OFF_SETMMODE)(g, 0);
    RD(float, g, GFX_VP) = (e + shiftX) * g_wideS;
    RD(float, g, GFX_VP + 4) = -(t + shiftY) * g_wideS;
}

static void hook_S2DM(void *g, float w, float h)
{
    if (!display_active() || (g_tallE > 0.0f && !display_main_surface(g)) || w != 640.0f || h != 384.0f) { g_origS2DM(g, w, h); return; }
    wide_proj(g, 0.0f, 0.0f);
    DISPLAY_GFX_CALL(LoadIdentFn, OFF_LOADIDENT)(g);
    g_wideM0 = RD(float, g, GFX_PROJ);
    g_wideM5 = RD(float, g, GFX_PROJ + 20);
    g_wideM12 = RD(float, g, GFX_PROJ + 48);
    g_wideM13 = RD(float, g, GFX_PROJ + 52);
}

/* 今ワイドの 2D の射影で描いているか */
static int wide_2d(void *g)
{
    return display_active() && !(g_tallE > 0.0f && !display_main_surface(g)) &&
        RD(float, g, GFX_PROJ) == g_wideM0 && RD(float, g, GFX_PROJ + 20) == g_wideM5 &&
        RD(float, g, GFX_PROJ + 48) == g_wideM12 && RD(float, g, GFX_PROJ + 52) == g_wideM13;
}

/* 1.0-pre9: 背景を伸ばすのは、ワイドの 2D の射影で、しかも動かしたり拡大したりしていない（モデルビューの行列が単位行列の）ときだけ。
   世界地図は 384x384 の窓の中で地図の絵を拡大して描くので、絵の大きさが 640 を超えても伸ばしてはいけない */
#define GFX_MODELVIEW 0x1c
static int wide_2d_plain(void *g)
{
    if (!wide_2d(g)) return 0;
    const float *m = (const float *)((uintptr_t)g + GFX_MODELVIEW);
    return m[0] == 1.0f && m[5] == 1.0f && m[12] == 0.0f && m[13] == 0.0f && m[1] == 0.0f && m[4] == 0.0f;
}

static int wide_shift_begin(void *g)
{
    if (!display_active() || !g || !wide_2d(g)) return 0;
    wide_proj(g, g_wideE, -g_tallE);
    /* 1.0-pre5: 切り抜き（SetScissor）も同じだけ右へずらす（描く範囲の左上 +0x4c0 を e·s → 2e·s） */
    *(volatile float *)((uintptr_t)g + GFX_VP) = 2.0f * g_wideE * g_wideS;
    return 1;
}

static void wide_shift_end(void *g)
{
    wide_proj(g, 0.0f, 0.0f);
}

static int is_field_hud(void *w)
{
    return g_field && w && w == RDP(g_field, FB_BASE);
}

/* 1.0-pre6: フィールドのボタン（メニュー・ダッシュ・台本ボタン）の位置を横に dx ずらす。
   1.0-pre7: 全画面のスワイプ受け（スライドの移動パッド）は、左右の余白まで広げる（パッドは受けの範囲で切り抜かれるため） */
static void hud_shift(void *w, float dx, float dy)
{
    void *node = RDP(w, W_HEAD);
    for (int guard = 0; node && guard < 64; guard++) {
        void *it = RDP(node, 0x10);
        node = RDP(node, 0);
        if (!it) continue;
        if (is_field_hud(w) && ((vt_slidepad && (uintptr_t)RDP(it, 0) == vt_slidepad + 0x10) || RD(float, it, I_SIZE) >= 600.0f)) {
            *(volatile float *)((uintptr_t)it + I_POS) = RD(float, it, I_POS) - dx;
            *(volatile float *)((uintptr_t)it + I_SIZE) = RD(float, it, I_SIZE) + 2.0f * dx;
            RD(float, it, I_POS + 4) += dy;
            RD(float, it, I_SIZE + 4) -= 2.0f * dy;
            continue;
        }
        *(volatile float *)((uintptr_t)it + I_POS) = RD(float, it, I_POS) + dx;
        RD(float, it, I_POS + 4) += dy;
    }
}

/* 1.0-pre6: MenuWidget::Update（1.2.0）は、タッチが画面 (0,0,640,384) の中かを IsPointIn で確かめる。
   ワイド表示では左右の余白も画面の中として扱う（右へ置いたボタンと、余白から始めるスライド移動のため） */
#define OFF_ISPOINTIN_FUNC 0x363a10UL   /* vector2::IsPointIn(float,float,float,float) */
#define OFF_ISPOINTIN_GOT  0x45d1d0UL
typedef int (*IsPointInFn)(void *v, float x, float y, float w, float h);
static IsPointInFn g_origIsPointIn;
/* 1.0-pre9: フィールドの画面の覆い（北の洞窟などの暗がり。640x384 を四隅の4枚に分けて描く）は、
   640 幅を画面いっぱいに引き伸ばして描く（左右の余白まで暗くする） */
#define OFF_SCRFILTER_FUNC 0x3ca2f8UL   /* FieldClass::DrawScreenFilter(MtxGraphics3D*) */
#define OFF_SCRFILTER_GOT  0x45f7d0UL
typedef void (*ScrFilterFn)(void *field, void *g);
static ScrFilterFn g_origScrFilter;
static void hook_ScrFilter(void *field, void *g)
{
    if (!wide_2d(g)) { g_origScrFilter(field, g); return; }
    DISPLAY_GFX_CALL(SetMModeFn, OFF_SETMMODE)(g, 1);
    DISPLAY_GFX_CALL(LoadIdentFn, OFF_LOADIDENT)(g);
    DISPLAY_GFX_CALL(GOrthoFn, OFF_GORTHO)(g, 0.0f, 640.0f, 384.0f, 0.0f, 0.0f, 1024.0f);
    DISPLAY_GFX_CALL(SetMModeFn, OFF_SETMMODE)(g, 0);
    RD(float, g, GFX_VP) = 0.0f; RD(float, g, GFX_VP + 4) = 0.0f;
    RD(float, g, GFX_SCREENRATIO) = 640.0f / RD(float, g, GFX_VP + 8);
    RD(float, g, GFX_SCREENRATIO + 4) = 384.0f / RD(float, g, GFX_VP + 12);
    g_origScrFilter(field, g);
    wide_proj(g, 0.0f, 0.0f);
    RD(float, g, GFX_SCREENRATIO) = RD(float, g, GFX_SCREENRATIO + 4) = 1.0f / g_wideS;
}

static int hook_IsPointIn(void *v, float x, float y, float w, float h)
{
    if (display_active() && x == 0.0f && y == 0.0f && w == 640.0f && h == 384.0f) {
        x = -g_wideE;
        y = -g_tallE;
        w = 640.0f + 2.0f * g_wideE;
        h = 384.0f + 2.0f * g_tallE;
    }
    return g_origIsPointIn(v, x, y, w, h);
}

static void hook_MiniMap(void *field, void *g)
{
    int sh = wide_shift_begin(g);
    g_origMiniMap(field, g);
    if (sh) wide_shift_end(g);
}

static void flush_insn(volatile unsigned *ip);

/* 1.0-pre4: 逃げるときの行き先（640 のすぐ外の 680）を、広げた画面でも見えない 800 にする */
static void wide_escape_patch(void)
{
    uintptr_t a[2] = { g_base + OFF_ESC_X1, g_base + OFF_ESC_X2 };
    for (int i = 0; i < 2; i++) {
        volatile unsigned *ip = (volatile unsigned *)a[i];
        if (*ip == INSN_MOV_800) continue;
        if (*ip != INSN_MOV_680) { LOGE("wide: escape insn %08x, expected %08x, not patched", *ip, INSN_MOV_680); return; }
    }
    for (int i = 0; i < 2; i++) {
        volatile unsigned *ip = (volatile unsigned *)a[i];
        if (mprotect((void *)(a[i] & ~0xfffUL), 0x1000, PROT_READ | PROT_WRITE | 4) != 0) { LOGE("wide: escape mprotect failed"); return; }
        *ip = INSN_MOV_800;
        flush_insn(ip);
    }
    LOGI("wide: escape target 680 -> 800");
}

/* 戦闘の AUTO/PAUSE/ESCAPE の欄をアイテムの下に固定し、▶ のつまみを消す（k_panelPatch） */
static void wide_panel_patch(void)
{
    int n = (int)(sizeof k_panelPatch / sizeof k_panelPatch[0]);
    for (int i = 0; i < n; i++) {
        unsigned v = *(volatile unsigned *)(g_base + k_panelPatch[i].off);
        if (v != k_panelPatch[i].orig && v != k_panelPatch[i].now) {
            LOGE("wide: panel insn %x = %08x, not patched", k_panelPatch[i].off, v);
            return;
        }
    }
    for (int i = 0; i < n; i++) {
        volatile unsigned *ip = (volatile unsigned *)(g_base + k_panelPatch[i].off);
        if (mprotect((void *)((uintptr_t)ip & ~0xfffUL), 0x1000, PROT_READ | PROT_WRITE | 4) != 0) { LOGE("wide: panel mprotect failed"); return; }
        *ip = k_panelPatch[i].now;
        flush_insn(ip);
    }
    LOGI("wide: battle panel fixed at 536, tab removed");
}

static void wide_note(const char *what, uintptr_t id, float x, float y, float w, float h)
{
    static uintptr_t seen[48];
    static int n;
    for (int i = 0; i < n; i++) if (seen[i] == id) return;
    if (n >= 48) return;
    seen[n++] = id;
    LOGI("wide: %s %lx (%d,%d %dx%d) widened", what, id, (int)x, (int)y, (int)w, (int)h);
}

static void hook_DIEx(void *g, void *tex, float x, float y, float w, float h, float u, float v, float uw, float vh,
                      int flags, int color, int mode)
{
    if ((flags & 7) == 0 && wide_2d_plain(g)) {
        DisplayRect before = { x, y, w, h };
        DisplayRect after = display_expand_image(before, g_wideE, g_tallE);
        if (after.x != x || after.y != y || after.w != w || after.h != h)
            wide_note("image", (uintptr_t)tex, x, y, w, h);
        x = after.x; y = after.y; w = after.w; h = after.h;
    }
    g_origDIEx(g, tex, x, y, w, h, u, v, uw, vh, flags, color, mode);
}

static void hook_Fill4(void *g, int x, int y, int w, int h)
{
    if (wide_2d_plain(g)) {
        display_expand_fill(&x, &y, &w, &h, g_wideE, g_tallE);
    }
    g_origFill4(g, x, y, w, h);
}

static void hook_Fill8(void *g, int x, int y, int w, int h, int r, int gg, int b, int a)
{
    if (wide_2d_plain(g)) {
        display_expand_fill(&x, &y, &w, &h, g_wideE, g_tallE);
    }
    g_origFill8(g, x, y, w, h, r, gg, b, a);
}

static void hook_DrawFar(void *field, void *g)
{
    volatile float *fw = (volatile float *)((uintptr_t)field + FIELD_W);
    volatile float *fh = (volatile float *)((uintptr_t)field + FIELD_H);
    float w0 = *fw, h0 = *fh;
    if (!display_active() || (w0 <= 640.5f && h0 <= 384.5f)) { g_origDrawFar(field, g); return; }
    FSet2DFn set2d = DISPLAY_GFX_CALL(FSet2DFn, OFF_FSET2D);
    *fw = 640.0f;
    *fh = 384.0f;
    set2d(field, g);
    g_origDrawFar(field, g);
    *fw = w0;
    *fh = h0;
    set2d(field, g);
}


/* Dialogue remains at its chosen top/bottom edge. Choices use the same offset
 * through the existing widget update/draw hooks, so touch and controller
 * coordinates move with the visible buttons. Other menus remain centered. */
static float message_vertical_shift(void *field)
{
    if (!g_messageAnchor || !field || g_tallE <= 0.0f) return 0.0f;
    return RD(int, field, 0x368) == 1 ? g_tallE : -g_tallE;
}
static float widget_vertical_shift(void *w)
{
    if (!w || g_tallE <= 0.0f) return 0.0f;
    if (is_field_hud(w)) return -g_tallE;
    if (g_messageAnchor && g_messageField && w == RDP(g_messageField, 0xfc00))
        return message_vertical_shift(g_messageField);
    return 0.0f;
}
static void hook_DrawMessage(void *field, void *g)
{
    g_messageField = field;
    float shift = message_vertical_shift(field);
    if (shift == 0.0f || !wide_2d(g)) { g_origDrawMessage(field, g); return; }
    wide_proj(g, 0.0f, shift);
    g_origDrawMessage(field, g);
    wide_proj(g, 0.0f, 0.0f);
}
static void hook_MapField(void *field, void *g, float zoom)
{
    /* SceneDraw truncates its computed dimensions to integers. For the Nova,
     * 1152/2.4 can truncate to 479; preserve the exact expanded logical canvas
     * before field projection, camera clamping, and tile culling read it. */
    if (g_tallE > 0.0f && display_main_surface(g)) {
        RD(float, field, FIELD_W) = 640.0f + 2.0f * g_wideE;
        RD(float, field, FIELD_H) = 384.0f + 2.0f * g_tallE;
    }
    g_origMapField(field, g, zoom);
}
static void display_install(void *handle)
{
    p_DestinationWidth = (DestinationFn)dlsym(handle, "_ZN13MtxGraphics3D19GetDestinationWidthEv");
    p_DestinationHeight = (DestinationFn)dlsym(handle, "_ZN13MtxGraphics3D20GetDestinationHeightEv");
    g_messageAnchor = english_import(handle, "_ZN10FieldClass11DrawMessageEP13MtxGraphics3D", (uintptr_t)hook_DrawMessage, (void **)&g_origDrawMessage);
    int field = english_import(handle, "_ZN10FieldClass12DrawMapFieldEP13MtxGraphics3Df", (uintptr_t)hook_MapField, (void **)&g_origMapField);
    g_tallReady = p_DestinationWidth && p_DestinationHeight && field;
    LOGI("display: tall view ready %d, dialogue anchor %d, field dimensions %d", g_tallReady, g_messageAnchor, field);
}
#endif
