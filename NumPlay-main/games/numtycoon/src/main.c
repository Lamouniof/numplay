
/* NumTycoon - a compact but deep business/management tycoon for NumWorks.
 * Designed for the 320x240 display: one backbuffer, integer-only simulation,
 * dirty-ish redraws and a small persistent save. */
#include <eadk.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "../../common/epsilon_app.h"
#include "../../common/epsilon_files.h"

#ifdef __ELF__
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "NumTycoon";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;
#endif

typedef uint16_t color;
#define RGB(c) (color)((((c)>>8)&0xF800)|(((c)>>5)&0x07E0)|(((c)>>3)&0x1F))
#define BLACK 0
#define WHITE 0xFFFF
#define BG RGB(0x0E1726)
#define PANEL RGB(0x18263A)
#define PANEL2 RGB(0x21344D)
#define TEXT RGB(0xEAF2FF)
#define MUTED RGB(0x8EA5BE)
#define GREEN RGB(0x35D07F)
#define RED RGB(0xFF5C68)
#define GOLD RGB(0xFFC857)
#define BLUE RGB(0x48A8FF)
#define CYAN RGB(0x4EE4E4)
#define PURPLE RGB(0xA982FF)
#define ORANGE RGB(0xFF8D4D)

#define KEY(k) (1ull << (k))
#define SAVE "numtycoon.sav"
#define W 320
#define H 240

enum { MENU, PLAY, BUILD, FINANCE, RESEARCH, INFO, EVENT };
enum { SHOP, CAFE, HOTEL, ARCADE, OFFICE };
enum { K_UP=1,K_DOWN=2,K_LEFT=4,K_RIGHT=8,K_OK=16,K_BACK=32,K_HOME=64 };

typedef struct {
  uint32_t magic;
  uint32_t cash, revenue, expenses, day, xp, prestige;
  uint16_t level, reputation, marketing, research, loan_level;
  uint8_t buildings[48];
  uint8_t upgrades[8];
  uint8_t selected;
  uint8_t reserved[3];
} Save;

static Save s;
static int state=MENU, cursor=0, build_kind=SHOP, menu_sel=0;
static uint32_t seed=0xA341316Cu, last_ms, tick_ms;
static bool dirty=true, event_open=false;
static int event_id=0;
static char msg[64]; static uint32_t msg_until;

static uint32_t rnd(void){ seed^=seed<<13; seed^=seed>>17; seed^=seed<<5; return seed; }
static int clampi(int v,int a,int b){return v<a?a:v>b?b:v;}
static int keys(void){
  uint64_t k=eadk_keyboard_scan(); int r=0;
  if(k&(KEY(eadk_key_up)|KEY(eadk_key_eight)))r|=K_UP;
  if(k&(KEY(eadk_key_down)|KEY(eadk_key_two)))r|=K_DOWN;
  if(k&(KEY(eadk_key_left)|KEY(eadk_key_four)))r|=K_LEFT;
  if(k&(KEY(eadk_key_right)|KEY(eadk_key_six)))r|=K_RIGHT;
  if(k&(KEY(eadk_key_ok)|KEY(eadk_key_exe)|KEY(eadk_key_five)))r|=K_OK;
  if(k&(KEY(eadk_key_back)|KEY(eadk_key_backspace)))r|=K_BACK;
  if(k&(KEY(eadk_key_home)|KEY(eadk_key_on_off)))r|=K_HOME;
  return r;
}
static void fill(int x,int y,int w,int h,color c){
  if(w<=0||h<=0)return;
  eadk_display_push_rect_uniform((eadk_rect_t){(uint16_t)clampi(x,0,W),(uint16_t)clampi(y,0,H),
    (uint16_t)clampi(w,0,W-x),(uint16_t)clampi(h,0,H-y)},c);
}
static void txt(const char *p,int x,int y,color fg,color bg){
  eadk_display_draw_string(p,(eadk_point_t){(uint16_t)x,(uint16_t)y},false,fg,bg);
}
static int slen(const char*p){int n=0;while(p[n])n++;return n;}
static void ctext(const char*p,int x,int y,color fg,color bg){txt(p,x-slen(p)*3,y,fg,bg);}
static void numstr(char *o,uint32_t v){
  char t[16]; int n=0; do{t[n++]=(char)('0'+v%10);v/=10;}while(v&&n<15);
  for(int i=0;i<n;i++)o[i]=t[n-i-1];o[n]=0;
}
static void money(char*o,uint32_t v){o[0]='$';numstr(o+1,v);}
static void banner(const char *a,const char*b){
  fill(0,0,W,H,BG); fill(0,0,W,28,PANEL2); ctext(a,160,7,TEXT,PANEL2);
  if(b)ctext(b,160,218,MUTED,BG);
}
static void toast(const char*p){strncpy(msg,p,sizeof(msg)-1);msg[sizeof(msg)-1]=0;msg_until=eadk_timing_millis()+2200;}

/* ---------- persistence ---------- */
static void defaults(void){
  memset(&s,0,sizeof s); s.magic=0x4E545931; s.cash=1200; s.revenue=0;
  s.expenses=0;s.day=1;s.xp=0;s.level=1;s.reputation=50;s.marketing=1;
  s.research=0;s.loan_level=0;s.prestige=0;s.selected=0;
}
static void save(void){ef_write(SAVE,&s,sizeof s);dirty=false;}
static void load(void){
  defaults(); uint32_t n=0; const uint8_t*p=ef_read(SAVE,&n);
  if(p&&n==sizeof s&&((const Save*)p)->magic==0x4E545931)memcpy(&s,p,sizeof s);
}

/* ---------- business model ---------- */
static const char*bn(int k){
  static const char*n[]={"SHOP","CAFE","HOTEL","ARCADE","OFFICE"};return n[k];
}
static uint32_t cost(int k){
  static const uint32_t c[]={180,420,1100,2400,5000};
  return c[k]*(1u+(s.day/35));
}
static uint32_t base_income(int k){
  static const uint32_t v[]={24,58,150,310,720};
  return v[k];
}
static int build_count(int k){int n=0;for(int i=0;i<48;i++)if(s.buildings[i]==k+1)n++;return n;}
static uint32_t total_income(void){
  uint32_t r=0; for(int k=0;k<5;k++)r+=build_count(k)*base_income(k);
  r += (uint32_t)s.reputation*2 + (uint32_t)s.marketing*18 + (uint32_t)s.research*12;
  r += (uint32_t)s.prestige*35;
  return r;
}
static uint32_t upgrade_cost(int i){return 600u*(i+1u)*(i+1u);}
static void level_check(void){
  uint32_t need=400u*s.level;
  while(s.xp>=need){s.xp-=need;s.level++;s.reputation=clampi(s.reputation+2,0,100);need=400u*s.level;toast("LEVEL UP!");}
}
static void simulate_day(void){
  uint32_t inc=total_income();
  uint32_t upkeep=0;
  for(int k=0;k<5;k++)upkeep += build_count(k)*(8u+k*7u);
  upkeep += (uint32_t)s.marketing*10u + (uint32_t)s.research*5u;
  int demand=(int)s.reputation + s.marketing*4 + s.level*2 + (int)s.prestige*3;
  int variation=(int)(rnd()%21)-10;
  int efficiency=clampi(demand+variation,25,140);
  inc=inc*(uint32_t)efficiency/100u;
  s.revenue+=inc; s.expenses+=upkeep; 
  if(s.cash>=upkeep)s.cash-=upkeep;else{s.cash=0;s.reputation=clampi(s.reputation-4,0,100);}
  s.cash+=inc; s.xp+=inc/3; s.day++;
  if(s.loan_level){uint32_t pay=80u*s.loan_level;if(s.cash>=pay)s.cash-=pay;else s.reputation=clampi(s.reputation-3,0,100);}
  if((rnd()%100)<9){
    event_id=(int)(rnd()%4); state=EVENT; event_open=true;
  }
  if(s.day%7==0)save();
  level_check(); dirty=true;
}
static void buy_building(void){
  int x=cursor%8,y=cursor/8,idx=y*8+x;uint32_t c=cost(build_kind);
  if(s.buildings[idx]){toast("Plot occupied");return;}
  if(s.cash<c){toast("Not enough cash");return;}
  s.cash-=c;s.buildings[idx]=(uint8_t)(build_kind+1);s.xp+=c/3;s.reputation=clampi(s.reputation+1,0,100);
  level_check();dirty=true;toast("Building constructed!");
}
static void take_loan(void){
  if(s.loan_level>=5){toast("Credit limit reached");return;}
  s.cash+=1200u*(s.loan_level+1u);s.loan_level++;s.reputation=clampi(s.reputation-1,0,100);dirty=true;toast("Credit secured");
}
static void buy_marketing(void){
  uint32_t c=300u*s.marketing;
  if(s.cash<c){toast("Marketing needs cash");return;}
  s.cash-=c;s.marketing++;s.reputation=clampi(s.reputation+3,0,100);dirty=true;toast("Campaign launched");
}
static void buy_upgrade(int i){
  uint32_t c=upgrade_cost(i);
  if(s.upgrades[i]){toast("Already researched");return;}
  if(s.cash<c){toast("Need more cash");return;}
  s.cash-=c;s.upgrades[i]=1;s.research++;s.reputation=clampi(s.reputation+5,0,100);dirty=true;
}
static void prestige(void){
  if(s.level<10){toast("Reach level 10");return;}
  s.prestige++;s.level=1;s.xp=0;s.cash=1500;s.revenue=s.expenses=0;
  memset(s.buildings,0,sizeof s.buildings);s.loan_level=0;s.marketing=1;s.research=0;
  memset(s.upgrades,0,sizeof s.upgrades);s.reputation=55;dirty=true;toast("Prestige acquired!");
}

/* ---------- graphics ---------- */
static void header(void){
  char a[24],b[24];money(a,s.cash);numstr(b,s.day);
  txt(a,8,5,GOLD,PANEL2);txt("DAY",116,5,MUTED,PANEL2);txt(b,145,5,TEXT,PANEL2);
  txt("LV",190,5,MUTED,PANEL2);numstr(b,s.level);txt(b,207,5,TEXT,PANEL2);
  txt("REP",242,5,MUTED,PANEL2);numstr(b,s.reputation);txt(b,270,5,GREEN,PANEL2);
}
static void building(int k,int x,int y,int w,int h){
  color roof[]={BLUE,CYAN,PURPLE,ORANGE,GOLD};
  color body[]={RGB(0x335D89),RGB(0x356F65),RGB(0x5D4A8C),RGB(0x824D32),RGB(0x74633A)};
  fill(x,y,w,h,body[k]);fill(x,y,w,5,roof[k]);
  int cols= k==2?2:3, rows=h>45?2:1;
  for(int j=0;j<rows;j++)for(int i=0;i<cols;i++)
    fill(x+7+i*((w-10)/cols),y+13+j*17,7,9,RGB(0xDDF4FF));
  if(k==2)fill(x+w/2-7,y+h-12,14,12,roof[k]);
}
static void map_draw(void){
  fill(0,28,W,184,RGB(0x14263A));
  /* roads */
  fill(28,28,32,184,RGB(0x33404D));fill(124,28,32,184,RGB(0x33404D));fill(220,28,32,184,RGB(0x33404D));
  fill(0,72,W,22,RGB(0x33404D));fill(0,138,W,22,RGB(0x33404D));
  for(int x=0;x<W;x+=18){fill(x,82,9,2,GOLD);fill(x,148,9,2,GOLD);}
  for(int y=28;y<212;y+=18){fill(42,y,2,9,GOLD);fill(138,y,2,9,GOLD);fill(234,y,2,9,GOLD);}
  for(int i=0;i<48;i++){
    int x=i%8,y=i/8,px=x*40+2,py=30+y*30;
    fill(px,py,36,27,RGB(0x203C52));
    if(s.buildings[i])building(s.buildings[i]-1,px+3,py+2,30,22);
    if(state==BUILD&&i==cursor){fill(px,py,36,2,GOLD);fill(px,py+25,36,2,GOLD);fill(px,py,2,27,GOLD);fill(px+34,py,2,27,GOLD);}
  }
  /* animated visitors */
  for(int i=0;i<10;i++){
    int px=(int)((i*47+s.day*3)%300)+5, py=98+(i%3)*17;
    fill(px,py,3,3,i&1?CYAN:GOLD);
  }
}
static void stat_bar(const char*label,int v,int x,int y,color c){
  txt(label,x,y,TEXT,PANEL);fill(x+70,y+2,110,8,PANEL2);fill(x+70,y+2,v,8,c);
}
static void panel_button(const char*p,int y,bool on){
  fill(184,y,128,24,on?PANEL2:PANEL);txt(p,194,y+7,on?TEXT:MUTED,on?PANEL2:PANEL);
}
static void draw_play(void){
  banner("NUMTYCOON","Build • optimize • dominate");
  header();map_draw();
  fill(0,212,W,28,PANEL);
  txt("OK Build",8,220,TEXT,PANEL);txt("VAR Finance",82,220,TEXT,PANEL);
  txt("X Research",172,220,TEXT,PANEL);txt("BACK Menu",252,220,TEXT,PANEL);
  if(msg_until>eadk_timing_millis())ctext(msg,160,190,GOLD,BG);
}
static void draw_build(void){
  draw_play();fill(176,31,140,178,PANEL);txt("BUILD MENU",188,39,TEXT,PANEL);
  for(int i=0;i<5;i++){char c[24],m[16];money(m,cost(i));strcpy(c,bn(i));strcat(c,"  ");strcat(c,m);
    panel_button(c,57+i*27,i==build_kind);}
  txt("UP/DOWN select",186,194,MUTED,PANEL);txt("OK place",186,205,GOLD,PANEL);
}
static void draw_finance(void){
  banner("FINANCE","Cash flow and credit");
  char a[24];money(a,s.cash);ctext("CASH",50,42,MUTED,BG);ctext(a,50,58,GOLD,BG);
  ctext("DAILY INCOME",150,42,MUTED,BG);money(a,total_income());ctext(a,150,58,GREEN,BG);
  ctext("LOAN LEVEL",250,42,MUTED,BG);numstr(a,s.loan_level);ctext(a,250,58,RED,BG);
  stat_bar("REVENUE",clampi((int)(s.revenue%1000)/10,0,100),20,88,GREEN);
  stat_bar("EXPENSES",clampi((int)(s.expenses%1000)/10,0,100),20,112,RED);
  txt("1,200 credit / level",20,148,TEXT,BG);txt("Interest: 80 / day",20,168,MUTED,BG);
  panel_button("TAKE LOAN",176,menu_sel==0);panel_button("MARKETING",176,menu_sel==1);panel_button("PRESTIGE",203,menu_sel==2);
  txt("OK action • BACK return",20,222,MUTED,BG);
}
static void draw_research(void){
  banner("RESEARCH","Permanent upgrades");
  static const char*n[]={"Staff training","Premium branding","Automation","Analytics","Global network","AI pricing","Franchise"};
  for(int i=0;i<7;i++){int y=35+i*25;fill(12,y,296,21,i==menu_sel?PANEL2:PANEL);
    txt(n[i],20,y+6,TEXT,i==menu_sel?PANEL2:PANEL);if(s.upgrades[i])txt("DONE",260,y+6,GREEN,i==menu_sel?PANEL2:PANEL);
    else {char c[12];money(c,upgrade_cost(i));txt(c,240,y+6,GOLD,i==menu_sel?PANEL2:PANEL);}
  }
  txt("OK buy • BACK return",20,222,MUTED,BG);
}
static void draw_info(void){
  banner("DASHBOARD","Your company at a glance");
  char a[24];
  ctext("PORTFOLIO",80,42,MUTED,BG);numstr(a,build_count(0));txt("shops",24,62,TEXT,BG);
  txt("cafes",24,82,TEXT,BG);numstr(a,build_count(1));txt(a,90,82,CYAN,BG);
  txt("hotels",24,102,TEXT,BG);numstr(a,build_count(2));txt(a,90,102,PURPLE,BG);
  txt("arcades",24,122,TEXT,BG);numstr(a,build_count(3));txt(a,90,122,ORANGE,BG);
  txt("offices",24,142,TEXT,BG);numstr(a,build_count(4));txt(a,90,142,GOLD,BG);
  txt("MARKETING",170,62,TEXT,BG);numstr(a,s.marketing);txt(a,250,62,BLUE,BG);
  txt("RESEARCH",170,82,TEXT,BG);numstr(a,s.research);txt(a,250,82,CYAN,BG);
  txt("PRESTIGE",170,102,TEXT,BG);numstr(a,s.prestige);txt(a,250,102,PURPLE,BG);
  txt("XP",170,122,TEXT,BG);numstr(a,s.xp);txt(a,220,122,GREEN,BG);
  txt("Next level",170,142,MUTED,BG);numstr(a,400u*s.level);txt(a,240,142,MUTED,BG);
  txt("OK / BACK return",20,222,MUTED,BG);
}
static void draw_event(void){
  banner("BREAKING NEWS","A market event is changing today");
  const char* title[]={"Tourism boom!","Power shortage!","Viral campaign!","Economic slowdown!"};
  int e=event_id;
  ctext(title[e],160,62,GOLD,BG);
  if(e==0)ctext("+25% hotel revenue",160,92,GREEN,BG);
  if(e==1)ctext("-15% daily profit",160,92,RED,BG);
  if(e==2)ctext("+8 reputation",160,92,CYAN,BG);
  if(e==3)ctext("-5 reputation",160,92,RED,BG);
  txt("OK acknowledge",104,180,TEXT,BG);
}

/* ---------- loop ---------- */
int main(void){
  np_app_begin();load();seed^=eadk_random();last_ms=eadk_timing_millis();tick_ms=last_ms;
  int prev=0;
  for(;;){
    uint32_t now=eadk_timing_millis();int k=keys(),edge=k&~prev;prev=k;
    if(k&K_HOME)break;
    if(state==MENU){
      if(edge&K_UP)menu_sel=(menu_sel+5)%6;
      if(edge&K_DOWN)menu_sel=(menu_sel+1)%6;
      if(edge&K_OK){
        if(menu_sel==0)state=PLAY;
        else if(menu_sel==1)state=BUILD;
        else if(menu_sel==2)state=FINANCE;
        else if(menu_sel==3)state=RESEARCH;
        else if(menu_sel==4)state=INFO;
        else {save();break;}
      }
    }else if(state==PLAY){
      if(edge&K_OK)state=BUILD;
      else if(edge&K_BACK)state=MENU;
      else if(edge&K_LEFT)state=FINANCE;
      else if(edge&K_RIGHT)state=RESEARCH;
      else if(edge&K_UP)cursor=(cursor+40)%48;
      else if(edge&K_DOWN)cursor=(cursor+8)%48;
      else if(edge&K_HOME)break;
      if(now-tick_ms>=700){tick_ms=now;simulate_day();}
    }else if(state==BUILD){
      if(edge&K_BACK)state=PLAY;
      else if(edge&K_LEFT)cursor=(cursor+47)%48;
      else if(edge&K_RIGHT)cursor=(cursor+1)%48;
      else if(edge&K_UP)build_kind=(build_kind+4)%5;
      else if(edge&K_DOWN)build_kind=(build_kind+1)%5;
      else if(edge&K_OK)buy_building();
    }else if(state==FINANCE){
      if(edge&K_BACK)state=PLAY;
      else if(edge&K_UP)menu_sel=(menu_sel+2)%3;
      else if(edge&K_DOWN)menu_sel=(menu_sel+1)%3;
      else if(edge&K_OK){if(menu_sel==0)take_loan();else if(menu_sel==1)buy_marketing();else prestige();}
    }else if(state==RESEARCH){
      if(edge&K_BACK)state=PLAY;
      else if(edge&K_UP)menu_sel=(menu_sel+6)%7;
      else if(edge&K_DOWN)menu_sel=(menu_sel+1)%7;
      else if(edge&K_OK)buy_upgrade(menu_sel);
    }else if(state==INFO){
      if(edge&K_BACK||edge&K_OK)state=PLAY;
    }else if(state==EVENT){
      if(edge&K_OK||edge&K_BACK){int e=event_id;
        if(e==0){s.revenue+=total_income()/4;}else if(e==1){s.cash=s.cash*85/100;}
        else if(e==2)s.reputation=clampi(s.reputation+8,0,100);else s.reputation=clampi(s.reputation-5,0,100);
        state=PLAY;event_open=false;dirty=true;
      }
    }
    eadk_display_wait_for_vblank();
    if(state==MENU){
      banner("NUMTYCOON","A management game for NumWorks");
      ctext("BUILD YOUR EMPIRE",160,42,GOLD,BG);
      static const char*items[]={"Continue","Build city","Finance","Research","Dashboard","Save & Quit"};
      for(int i=0;i<6;i++){int y=72+i*23;fill(55,y,210,20,i==menu_sel?PANEL2:PANEL);
        ctext(items[i],160,y+5,i==menu_sel?TEXT:MUTED,i==menu_sel?PANEL2:PANEL);}
      ctext("Arrows navigate • OK select",160,222,MUTED,BG);
    }else if(state==PLAY)draw_play();
    else if(state==BUILD)draw_build();
    else if(state==FINANCE)draw_finance();
    else if(state==RESEARCH)draw_research();
    else if(state==INFO)draw_info();
    else if(state==EVENT)draw_event();
    if(msg_until>now&&state!=MENU)ctext(msg,160,198,GOLD,BG);
    eadk_display_wait_for_vblank();
    uint32_t spent=eadk_timing_millis()-now;if(spent<33)eadk_timing_msleep(33-spent);
  }
  if(dirty)save();
  return np_app_end();
}
