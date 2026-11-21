#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../portable/ffd_nav.h"
#include "../portable/ffd_display_transform.h"

static int closef(float a, float b) { return fabsf(a-b) < 0.003f; }

int main(void) {
    const struct { float w,h,x,y,scale; } samples[] = {
        {1280,720,21.333333f,0,1.875f},
        {1920,1080,21.333333f,0,2.8125f},
        {960,544,18.823529f,0,1.4166667f},
        {1536,1152,0,48,2.4f},
        {1920,1200,0,8,3.0f},
        {1600,960,0,0,2.5f}
    };
    for(unsigned i=0; i<sizeof(samples)/sizeof(samples[0]); ++i) {
        DisplayGeometry g=display_geometry(samples[i].w,samples[i].h);
        assert(closef(g.x,samples[i].x) && closef(g.y,samples[i].y));
        assert(closef(g.scale,samples[i].scale));
        FfdPoint p=ffd_game_to_surface(320,192,samples[i].w,samples[i].h);
        assert(closef(p.x,samples[i].w/2) && closef(p.y,samples[i].h/2));
        p=ffd_surface_to_game(p.x,p.y,samples[i].w,samples[i].h);
        assert(closef(p.x,320) && closef(p.y,192));
    }
    DisplayGeometry square=display_geometry(1024,1024);
    assert(!square.active);
    DisplayRect rect={0,0,640,384};
    rect=display_expand_image(rect,21.333333f,0);
    assert(rect.x < -21.3f && rect.w > 682.6f);
    int x=0,y=0,w=640,h=384;
    display_expand_fill(&x,&y,&w,&h,20,0);
    assert(x==-21 && w==682 && y==0 && h==384);

    FfdNavCandidate c[] = {
        {1,42,0,1,0,10,10,80,30},
        {1,42,1,1,0,10,50,80,30},
        {1,42,2,1,0,10,90,80,30},
        {2,42,-1,0,0,150,10,80,30},
        {3,42,-1,0,0,150,50,80,30},
        {4,42,-1,0,0,150,90,80,30}
    };
    FfdNavState state={0};
    FfdNavResult r=ffd_nav_step(&state,c,6,42,0);
    assert(r.selected_index==0);
    r=ffd_nav_step(&state,c,6,42,FFD_NAV_DOWN); assert(r.selected_index==1);
    r=ffd_nav_step(&state,c,6,42,FFD_NAV_RIGHT); assert(r.selected_index==4);
    r=ffd_nav_step(&state,c,6,42,FFD_NAV_LEFT); assert(r.selected_index==1);
    r=ffd_nav_step(&state,c,6,42,FFD_NAV_UP); assert(r.selected_index==0);
    r=ffd_nav_step(&state,c,6,42,FFD_NAV_UP); assert(r.selected_index==2);
    r=ffd_nav_step(&state,c,6,42,FFD_NAV_ACCEPT);
    assert(r.selected_index==2 && r.activated && closef(r.touch_x,50) && closef(r.touch_y,105));
    FfdNavCandidate tmp=c[0]; c[0]=c[2]; c[2]=tmp;
    r=ffd_nav_step(&state,c,6,42,0);
    assert(r.selected_index==0 && closef(r.touch_y,105));
    r=ffd_nav_step(&state,c,0,42,FFD_NAV_ACCEPT);
    assert(r.selected_index==-1 && !r.activated && !state.has_selection);
    FfdNavCandidate simple[]={{8,99,-1,0,0,0,0,30,30},{9,99,-1,2,390,100,0,30,30}};
    r=ffd_nav_step(&state,simple,2,99,0); assert(r.selected_index==1);
    simple[1].item=999;
    r=ffd_nav_step(&state,simple,2,99,0); assert(r.selected_index==1);
    FfdNavCandidate invalid[]={{1,2,-1,0,0,0,0,0,0}};
    r=ffd_nav_step(&state,invalid,1,2,FFD_NAV_ACCEPT);
    assert(r.selected_index==-1 && !r.activated);
    puts("Portable navigation/display tests: PASS");
    return 0;
}
