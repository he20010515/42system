// bootpack.h
#include "struct.h"
// naskfunc.nas
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
void io_out8(int port, int data);
int io_in8(int port);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
void asm_inthandler20(void);
void asm_inthandler0d(void);
void asm_inthandler0c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
int load_cr0(void);
void store_cr0(int cr0);
void load_tr(int tr);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void asm_cons_putchar(void);
void asm_hrb_api(void);
void start_app(int eip, int cs, int esp, int ds, int loc_tss_esp0);
void asm_end_app(void);
// dsctbl.c

void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
/* int.c */
void init_pic(void);
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);
int *inthandler0d(int *esp);
int *inthandler0c(int *esp);

// fifo.c
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
int fifo8_get(struct FIFO8 *fifo);
int fifo8_status(struct FIFO8 *fifo);
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

// mouse.c
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *dec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);

// keyboard.c
void init_keyboard(struct FIFO32 *fifo, int data0);
void wait_KBC_sendready(void);

// memory.c
unsigned int memtest(unsigned int start, unsigned int end);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc(struct MEMMAN *self, unsigned int size);
unsigned int memman_total(struct MEMMAN *self);
void memman_init(struct MEMMAN *self);
unsigned memman_alloc_4k(struct MEMMAN *man, unsigned int size);
unsigned memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

// sheet.c:

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);

// timer.c

void init_pit(void);
void inthandler20(int *esp);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, unsigned char data);
void timer_settime(struct TIMER *timer, unsigned int timeout);

// graphic.c

void init_screen8(char *vram, int xsize, int ysize);
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s);
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0);

// mtask.c
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_now(void);
void task_add(struct TASK *task);
void task_remove(struct TASK *task);
void task_switchsub(void);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
// file.c
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
// console.c
void cons_putchar(struct CONSOLE *console, int chr, char move);
int cons_newline(struct CONSOLE *console);
void cons_runcmd(char *cmdline, struct CONSOLE *console, int *fat, unsigned int memtotal);
void cmd_mem(struct CONSOLE *console, unsigned int memtotal);
void cmd_clear(struct CONSOLE *console);
void cmd_ls(struct CONSOLE *console);
void cmd_cat(struct CONSOLE *console, int *fat, char *cmdline);
void cmd_hlt(struct CONSOLE *console, int *fat);
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
int cmd_app(struct CONSOLE *console, int *fat, char *cmdline);
void cons_putstring(struct CONSOLE *console, char *s);
// taskFunction !! 不能调用!!
void console_task(struct SHEET *sheet, int memtotal);
void HariMain(void);
