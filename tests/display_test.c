/* Exercise the actual display hooks with mock graphics/touch memory. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char u8;
#define RD(T,p,o) (*(T *)((uintptr_t)(p)+(o)))
#define RDP(p,o) ((void *)RD(uintptr_t,p,o))
#define LOGI(...) ((void)0)
#define LOGE(...) ((void)0)
#define PROT_READ 1
#define PROT_WRITE 2
#define TP_SCALE 0x674
#define TP_OFFSET 0x67c
#define FB_BASE 0xfbd8
#define W_HEAD 0x20
#define I_POS 0x28
#define I_SIZE 0x30
static uintptr_t g_base, vt_slidepad;
static float g_wideE, g_wideS, g_tallE;
static void *g_field, *g_messageField;
static int g_messageAnchor;
static u8 touch[0x700], *app;
static int destinationW, destinationH, matrixMode, viewport[4], original2DCalls;
static float farWidth, farHeight, messageEdge, mapHeight;
static int drawCalls, farCalls, fieldCalls, fill[4];
static float image[4];
static void *get_touch(void) { return touch; }
static void *(*p_GetTouchInfo)(void)=get_touch;
static int mprotect(void *p,size_t n,int flags) { (void)p;(void)n;(void)flags;return 0; }
static void *dlsym(void *h,const char *s) { (void)h;(void)s;return NULL; }
static int english_import(void *h,const char *s,uintptr_t hook,void **old)
{ (void)h;(void)s;(void)hook;(void)old;return 0; }
static uintptr_t display_test_function(unsigned off);
#define DISPLAY_GFX_CALL(T,off) ((T)display_test_function(off))
#include "../src/display_runtime.h"
static void flush_insn(volatile unsigned *p) { (void)p; }
static int near(float a,float b) { float d=a-b;return d<0.003f&&d>-0.003f; }
static void identity(void *g)
{
    float *m=(float *)((u8 *)g+(matrixMode?GFX_PROJ:GFX_MODELVIEW));
    memset(m,0,16*sizeof(float)); m[0]=m[5]=m[10]=m[15]=1.0f;
}
static void mode(void *g,int m) { (void)g;matrixMode=m; }
static void ortho(void *g,float l,float r,float b,float t,float n,float f)
{
    float *m=(float *)((u8 *)g+GFX_PROJ);
    memset(m,0,16*sizeof(float));
    m[0]=2/(r-l);m[5]=2/(t-b);m[10]=-2/(f-n);m[15]=1;
    m[12]=-(r+l)/(r-l);m[13]=-(t+b)/(t-b);m[14]=-(f+n)/(f-n);
}
static float pixel_y(void *g,float y)
{ return (1-(RD(float,g,GFX_PROJ+20)*y+RD(float,g,GFX_PROJ+52)))*0.5f*viewport[3]; }
static float pixel_x(void *g,float x)
{ return (RD(float,g,GFX_PROJ)*x+RD(float,g,GFX_PROJ+48)+1)*0.5f*viewport[2]; }
static void field_2d(void *field,void *g)
{
    float w=RD(float,field,FIELD_W),h=RD(float,field,FIELD_H);
    ortho(g,-w/2,w/2,h/2,-h/2,0,1024);
}
static uintptr_t display_test_function(unsigned off)
{
    switch(off) {
    case OFF_SETMMODE:return (uintptr_t)mode;
    case OFF_LOADIDENT:return (uintptr_t)identity;
    case OFF_GORTHO:return (uintptr_t)ortho;
    case OFF_FSET2D:return (uintptr_t)field_2d;
    default:assert(0);return 0;
    }
}
static int get_width(void *g) { (void)g;return destinationW; }
static int get_height(void *g) { (void)g;return destinationH; }
static void viewport_original(void *g,int x,int y,int w,int h)
{
    viewport[0]=x;viewport[1]=y;viewport[2]=w;viewport[3]=h;
    RD(float,g,GFX_VP)=(float)x;RD(float,g,GFX_VP+4)=(float)y;
    RD(float,g,GFX_VP+8)=(float)w;RD(float,g,GFX_VP+12)=(float)h;
}
static void main_viewport_original(void *scene,void *g)
{
    (void)scene;
    float sx=destinationW/640.0f,sy=destinationH/384.0f,s=sx<sy?sx:sy;
    hook_ViewPort(g,(int)((destinationW-s*640)/2),(int)((destinationH-s*384)/2),(int)(s*640),(int)(s*384));
    sx=RD(float,g,GFX_VP+8)/640;sy=RD(float,g,GFX_VP+12)/384;s=sx<sy?sx:sy;
    RD(float,touch,TP_SCALE)=1/sx;RD(float,touch,TP_SCALE+4)=1/sy;
    RD(float,touch,TP_OFFSET)=RD(float,g,GFX_VP);RD(float,touch,TP_OFFSET+4)=RD(float,g,GFX_VP+4);
    RD(float,app,0x178)=sx;RD(float,app,0x17c)=sy;RD(float,app,0x184)=s;
    RD(float,app,APP_IMGFIX_X)=s/sx;RD(float,app,APP_IMGFIX_Y)=s/sy;
}
static void original_2d(void *g,float w,float h)
{ original2DCalls++;ortho(g,0,w,h,0,0,1024);matrixMode=0;identity(g); }
static void original_far(void *field,void *g)
{ (void)g;farWidth=RD(float,field,FIELD_W);farHeight=RD(float,field,FIELD_H);farCalls++; }
static void original_message(void *field,void *g)
{
    drawCalls++;messageEdge=pixel_y(g,RD(int,field,0x368)==1?384.0f:0.0f);
    float scissorY=RD(float,g,GFX_VP+4)+viewport[3]-(300+80)*g_wideS;
    assert(near(scissorY,viewport[3]-pixel_y(g,380)));
}
static void original_map(void *field,void *g,float zoom)
{ (void)g;assert(zoom==1);fieldCalls++;mapHeight=RD(float,field,FIELD_H); }
static void original_image(void *g,void *tex,float x,float y,float w,float h,float u,float v,float uw,float vh,int flags,int color,int render)
{ (void)g;(void)tex;(void)u;(void)v;(void)uw;(void)vh;(void)flags;(void)color;(void)render;image[0]=x;image[1]=y;image[2]=w;image[3]=h; }
static void original_fill(void *g,int x,int y,int w,int h)
{ (void)g;fill[0]=x;fill[1]=y;fill[2]=w;fill[3]=h; }
static void original_fill8(void *g,int x,int y,int w,int h,int r,int green,int b,int a)
{ (void)r;(void)green;(void)b;(void)a;original_fill(g,x,y,w,h); }
static int original_hit(void *v,float x,float y,float w,float h)
{ float *p=v;return p[0]>=x&&p[0]<=x+w&&p[1]>=y&&p[1]<=y+h; }
static void original_filter(void *field,void *g)
{
    (void)field;
    assert(near(RD(float,g,GFX_PROJ+20),-2/384.0f));
    assert(RD(float,g,GFX_VP)==0&&RD(float,g,GFX_VP+4)==0);
    assert(near(RD(float,g,GFX_SCREENRATIO+4),384.0f/viewport[3]));
}
static void test_resolution(void *g,int w,int h)
{
    destinationW=w;destinationH=h;hook_MSVP(NULL,g);
    DisplayGeometry d=display_geometry((float)w,(float)h);
    assert(near(g_wideE,d.x)&&near(g_tallE,d.y));
    if (!d.active) {assert(!wide_2d(g));return;}
    assert(viewport[0]==0&&viewport[1]==0&&viewport[2]==w&&viewport[3]==h);
    assert(near(RD(float,app,APP_IMGFIX_X),1)&&near(RD(float,app,APP_IMGFIX_Y),1));
    assert(near(RD(float,touch,TP_SCALE),RD(float,touch,TP_SCALE+4)));
    assert(near(((float)w/2-RD(float,touch,TP_OFFSET))*RD(float,touch,TP_SCALE),320));
    assert(near(((float)h/2-RD(float,touch,TP_OFFSET+4))*RD(float,touch,TP_SCALE+4),192));
    hook_S2DM(g,640,384);assert(wide_2d_plain(g));
    assert(near(pixel_x(g,320),(float)w/2)&&near(pixel_y(g,192),(float)h/2));
    assert(near(pixel_x(g,32)-pixel_x(g,0),pixel_y(g,32)-pixel_y(g,0)));
    assert(near(RD(float,g,GFX_VP+4)+(float)h-384*g_wideS,(float)h-pixel_y(g,384)));
    /* A standard projection with matching X but different Y must not match. */
    float m5=RD(float,g,GFX_PROJ+20);RD(float,g,GFX_PROJ+20)=m5*2;
    assert(!wide_2d(g));RD(float,g,GFX_PROJ+20)=m5;
}
static void test_4x3(void *g)
{
    test_resolution(g,1536,1152);assert(g_tallE==48&&g_wideE==0);
    float p[2]={10,-40};assert(hook_IsPointIn(p,0,0,640,384));
    p[1]=430;assert(hook_IsPointIn(p,0,0,640,384));p[1]=433;assert(!hook_IsPointIn(p,0,0,640,384));
    hook_DIEx(g,NULL,0,0,640,384,0,0,640,384,0,0,0);
    assert(near(image[0],0)&&near(image[1],-48)&&near(image[2],640)&&near(image[3],480));
    hook_DIEx(g,NULL,100,100,32,48,0,0,32,48,0,0,0);
    assert(image[0]==100&&image[1]==100&&image[2]==32&&image[3]==48);
    hook_DIEx(g,NULL,0,0,640,384,0,0,640,384,1,0,0);assert(image[1]==0&&image[3]==384);
    hook_Fill4(g,0,0,640,384);assert(fill[0]==0&&fill[1]<=-48&&fill[3]>=480);
    hook_Fill8(g,0,292,640,92,0,0,0,0);assert(fill[1]==292&&fill[1]+fill[3]>=432);
    hook_ScrFilter(NULL,g);assert(wide_2d(g));assert(near(RD(float,g,GFX_SCREENRATIO+4),1/g_wideS));

    u8 *field=calloc(1,0x10000);assert(field);g_messageAnchor=1;
    RD(int,field,0x368)=1;hook_DrawMessage(field,g);assert(near(messageEdge,1152)&&wide_2d(g));
    RD(int,field,0x368)=0;hook_DrawMessage(field,g);assert(near(messageEdge,0)&&wide_2d(g));
    RD(float,field,FIELD_W)=640;RD(float,field,FIELD_H)=479;
    hook_MapField(field,g,1);assert(mapHeight==480);
    hook_DrawFar(field,g);assert(farWidth==640&&farHeight==384);
    assert(RD(float,field,FIELD_W)==640&&RD(float,field,FIELD_H)==480);
    hook_S2DM(g,640,384);

    u8 widget[0x80]={0}, choice[0x80]={0}, button[0xc0]={0}, slider[0xc0]={0}, nodes[2][0x18]={{0}};
    g_field=field;RD(void *,field,FB_BASE)=widget;RD(void *,field,0xfc00)=choice;
    RD(void *,widget,W_HEAD)=nodes[0];RD(void *,nodes[0],0)=nodes[1];
    RD(void *,nodes[0],0x10)=button;RD(void *,nodes[1],0x10)=slider;
    RD(float,button,I_POS)=228;RD(float,button,I_POS+4)=6;RD(float,button,I_SIZE)=96;RD(float,button,I_SIZE+4)=40;
    RD(float,slider,I_SIZE)=640;RD(float,slider,I_SIZE+4)=384;
    assert(widget_vertical_shift(widget)==-48);
    hud_shift(widget,0,-48);assert(RD(float,button,I_POS+4)==-42&&RD(float,slider,I_SIZE+4)==480);
    hud_shift(widget,0,48);assert(RD(float,button,I_POS+4)==6&&RD(float,slider,I_SIZE+4)==384);
    RD(int,field,0x368)=1;assert(widget_vertical_shift(choice)==48);
    RD(int,field,0x368)=0;assert(widget_vertical_shift(choice)==-48);
    g_messageAnchor=0;assert(widget_vertical_shift(choice)==0);
    g_field=NULL;g_messageField=NULL;free(field);

    /* Square subviews and render textures retain their own projection. */
    int before=original2DCalls;hook_S2DM(g,384,384);assert(original2DCalls==before+1&&!wide_2d(g));
    RD(void *,g,0x10)=g;hook_S2DM(g,640,384);assert(original2DCalls==before+2&&!wide_2d(g));
    RD(void *,g,0x10)=NULL;hook_S2DM(g,640,384);
    hook_ViewPort(g,0,50,640,384);assert(viewport[1]==50&&viewport[3]==384);
    hook_ViewPort(g,100,0,400,384);assert(viewport[0]==100&&viewport[2]==400&&!wide_2d(g));
    g_tallReady=0;hook_MSVP(NULL,g);
    assert(!display_active()&&viewport[3]<destinationH);
    g_tallReady=1;
}
int main(void)
{
    u8 g[0x600]={0};app=calloc(1,0x200);u8 *base=calloc(1,0x470000);assert(app&&base);g_base=(uintptr_t)base;
    void *appSlot=app;RD(void *,base,APP_PTR_GOT)=&appSlot;
    g_origViewPort=viewport_original;g_origMSVP=main_viewport_original;g_origS2DM=original_2d;
    p_DestinationWidth=get_width;p_DestinationHeight=get_height;
    g_tallReady=1;
    g_origDIEx=original_image;g_origFill4=original_fill;g_origFill8=original_fill8;
    g_origIsPointIn=original_hit;g_origScrFilter=original_filter;g_origDrawMessage=original_message;
    g_origDrawFar=original_far;g_origMapField=original_map;
    const int sizes[][2]={{1536,1152},{640,480},{800,600},{1024,768},{1600,960},{1920,1080},{1280,720},{1920,1200},{2560,1080},{1080,1920},{1080,1080}};
    for(unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++)test_resolution(g,sizes[i][0],sizes[i][1]);
    test_4x3(g);
    test_resolution(g,1920,1080);assert(g_tallE==0&&g_wideE>0);
    hook_DIEx(g,NULL,0,0,640,384,0,0,640,384,0,0,0);assert(image[0]<0&&image[1]==0&&image[2]>640&&image[3]==384);
    test_resolution(g,1080,1920);assert(!display_active());
    assert(!display_geometry(0,1152).active&&!display_geometry(-1,1152).active);
    free(base);free(app);
    puts("PASS: 11 resolutions; Nova 640x480 field; viewport/touch/scissor; dialogue/HUD; sprites/backgrounds; render targets; aspect transitions");
    return 0;
}
