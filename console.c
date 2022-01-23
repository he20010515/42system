#include "bootpack.h"

void console_task(struct SHEET *sheet, int memtotal)
{
    struct TIMER *cursor_timer;
    struct TASK *task = task_now();
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    struct CONSOLE console;
    console.sht = sheet;
    console.cur_x = 8;
    console.cur_y = 28;
    console.cur_c = -1;

    char s[100];
    int i, fifobuf[128];
    int *fat = (int *)memman_alloc_4k(memman, 4 * 2880);
    char cmdline[64];
    cursor_timer = timer_alloc();
    fifo32_init(&task->fifo, 128, fifobuf, task);
    timer_init(cursor_timer, &task->fifo, 1);
    timer_settime(cursor_timer, 50); // 0.5s光标闪烁
    file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));

    // TEMP
    *((int *)0xfec) = (int)&console;
    // cursor
    cons_putchar(&console, '>', 1);
    for (;;)
    {
        io_cli();
        if (fifo32_status(&task->fifo) == 0)
        {
            task_sleep(task);
            io_sti();
        }
        else
        {
            i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1) // 控制光标闪烁
            {
                switch (i)
                {
                case 0:
                    timer_init(cursor_timer, &task->fifo, 1);
                    if (console.cur_c >= 0)
                    {
                        console.cur_c = COL8_000000;
                    }
                    break;
                case 1:
                    timer_init(cursor_timer, &task->fifo, 0);
                    if (console.cur_c >= 0)
                    {
                        console.cur_c = COL8_FFFFFF;
                    }
                    break;
                }
                timer_settime(cursor_timer, 50);
            }
            if (i == 2 OR i == 3) // 控制光标开启或者关闭
            {
                if (i == 2)
                {
                    console.cur_c = COL8_FFFFFF;
                }
                else if (i == 3)
                {
                    boxfill8(sheet->buf, sheet->bxsize, COL8_000000, console.cur_x, 28, console.cur_x + 7, 43);
                    console.cur_c = -1;
                }
            }
            if (256 <= i AND i <= 511) //来自taska的键盘数据
            {
                i -= 256;
                if (i == 8) //退格键
                {
                    if (console.cur_x > 16)
                    {
                        cons_putchar(&console, ' ', 0);
                        console.cur_x -= 8;
                    }
                }
                else if (i == 10) // 回车键
                {
                    cons_putchar(&console, ' ', 0);
                    cmdline[console.cur_x / 8 - 2] = 0;
                    cons_newline(&console);
                    cons_runcmd(cmdline, &console, fat, memtotal);
                    console.cur_x = 8;
                    cons_putchar(&console, '>', 1);
                }
                else // 一般字符
                {
                    if (console.cur_x < 240)
                    {
                        cmdline[console.cur_x / 8 - 2] = i;
                        cons_putchar(&console, i, 1);
                    }
                }
            }
            if (console.cur_c >= 0) // 刷新光标
            {
                boxfill8(sheet->buf, sheet->bxsize, console.cur_c, console.cur_x, console.cur_y, console.cur_x + 7, console.cur_y + 16);
            }
            sheet_refresh(sheet, console.cur_x, console.cur_y, console.cur_x + 8, console.cur_y + 16);
        }
    }
}

void cons_putchar(struct CONSOLE *console, int chr, char move)
{
    char s[2];
    s[0] = chr;
    s[1] = 0;
    if (s[0] == 0x09) //制表符
    {
        for (;;)
        {
            putfonts8_asc_sht(console->sht, console->cur_x, console->cur_y, COL8_FFFFFF, COL8_000000, " ");
            console->cur_x += 8;
            if (console->cur_x == 8 + 240)
            {
                cons_newline(console);
            }
            if (((console->cur_x - 8) & 0x1f) == 0)
            {
                break; //如果被32整除则break
            }
        }
    }
    else if (s[0] == 0x0a)
    {
        cons_newline(console);
    }
    else if (s[0] == 0x0d)
    {
        ;
    }
    else
    {
        putfonts8_asc_sht(console->sht, console->cur_x, console->cur_y, COL8_FFFFFF, COL8_000000, s);
        if (move != 0)
        {
            console->cur_x += 8;
            if (console->cur_x == 8 + 240)
            {
                cons_newline(console);
            }
        }
    }
    return;
}

int cons_newline(struct CONSOLE *console)
{
    int x, y;
    struct SHEET *sheet = console->sht;
    if (console->cur_y < 28 + 112)
    {
        console->cur_y += 16; //换行
    }
    else
    {
        for (y = 28; y < 28 + 112; y++)
        {
            for (x = 8; x < 8 + 240; x++)
            {
                sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
            }
        }
        for (y = 28 + 112; y < 28 + 128; y++)
        {
            for (x = 8; x < 8 + 240; x++)
            {
                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
            }
        }
        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    }
    console->cur_x = 8;
}

void cons_runcmd(char *cmdline, struct CONSOLE *console, int *fat, unsigned int memtotal)
{

    if (strcmp(cmdline, "mem") == 0)
    {
        cmd_mem(console, memtotal);
    }
    else if (strcmp(cmdline, "clear") == 0)
    {
        cmd_clear(console);
    }
    else if (strcmp(cmdline, "ls") == 0) // ls
    {
        cmd_ls(console);
    }
    else if (strncmp(cmdline, "cat ", 4) == 0) // cat 命令
    {
        cmd_cat(console, fat, cmdline);
    }
    else if (cmdline[0] != 0) //未知命令
    {
        if (cmd_app(console, fat, cmdline) == 0)
        {
            putfonts8_asc_sht(console->sht, 8, console->cur_y, COL8_FFFFFF, COL8_000000, "Command Not Fond");
            cons_newline(console);
        }
    }
    return;
}

void cmd_mem(struct CONSOLE *console, unsigned int memtotal)
{
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    char s[30];
    sprintf(s, "total    %dMB", memtotal / (1024 * 1024));
    putfonts8_asc_sht(console->sht, 8, console->cur_y, COL8_FFFFFF, COL8_000000, s);
    cons_newline(console);
    sprintf(s, "free     %dKB", memman_total(memman) / 1024);
    putfonts8_asc_sht(console->sht, 8, console->cur_y, COL8_FFFFFF, COL8_000000, s);
    cons_newline(console);
    return;
}

void cmd_clear(struct CONSOLE *console)
{
    int x, y;
    struct SHEET *sheet = console->sht;
    for (y = 28; y < 28 + 128; y++)
    {
        for (x = 8; x < 8 + 240; x++)
        {
            sheet->buf[x + y * sheet->bxsize] = COL8_000000;
        }
    }
    sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    console->cur_y = 28;
}

void cmd_ls(struct CONSOLE *console)
{
    int x, y;
    struct SHEET *sheet = console->sht;
    struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);
    char s[30];
    for (x = 0; x < 224; x++)
    {
        if (finfo[x].name[0] == 0x00)
        {
            break;
        }
        if (finfo[x].name[0] != 0xe5)
        {
            if ((finfo[x].type & 0x18) == 0)
            {
                sprintf(s, "filename.ext %7d", finfo[x].size);
                for (y = 0; y < 8; y++)
                {
                    s[y] = finfo[x].name[y];
                }
                s[9] = finfo[x].ext[0];
                s[10] = finfo[x].ext[1];
                s[11] = finfo[x].ext[2];
                putfonts8_asc_sht(sheet, 8, console->cur_y, COL8_FFFFFF, COL8_000000, s);
                cons_newline(console);
            }
        }
    }
    cons_newline(console);
}

void cmd_cat(struct CONSOLE *console, int *fat, char *cmdline)
{
    int i;
    char s[20];
    struct FILEINFO *finfo = file_search(cmdline + 4, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
    char *p;
    struct SHEET *sheet = console->sht;
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    //准备文件名
    if (finfo != 0)
    {
        p = (char *)memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
        for (i = 0; i < finfo->size; i++)
        {
            cons_putchar(console, p[i], 1);
        }
    }
    else
    {
        putfonts8_asc_sht(sheet, 8, console->cur_y, COL8_FFFFFF, COL8_000000, "File Not Found.");
        cons_newline(console);
    }
}

// void cmd_hlt(struct CONSOLE *console, int *fat)
// {
//     char s[20];
//     //启动应用程序hlt.hrb
//     int x, y;
//     struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);
//     struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
//     struct SHEET *sheet = console->sht;
//     struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
//     char *p;
//     for (y = 0; y < 11; y++)
//     {
//         s[y] = ' ';
//     }
//     s[0] = 'H';
//     s[1] = 'L';
//     s[2] = 'T';
//     s[8] = 'H';
//     s[9] = 'R';
//     s[10] = 'B';
//     for (x = 0; x < 224;)
//     {
//         if (finfo[x].name[0] == 0x00)
//         {
//             break;
//         }
//         if ((finfo[x].type & 0x18) == 0)
//         {
//             for (y = 0; y < 11; y++)
//             {
//                 if (finfo[x].name[y] != s[y])
//                 {
//                     goto hlt_next_file;
//                 }
//             }
//             break;
//         }
//     hlt_next_file:
//         x++;
//     }
//     if (x < 224 AND finfo[x].name[0] != 0x00)
//     {
//         p = (char *)memman_alloc_4k(memman, finfo[x].size);
//         file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
//         set_segmdesc(gdt + 1003, finfo[x].size - 1, (int)p, AR_CODE32_ER);
//         farcall(0, 1003 * 8);
//         memman_free_4k(memman, (int)*p, finfo[x].size);
//     }
//     else
//     {
//         putfonts8_asc_sht(sheet, 8, console->cur_y, COL8_FFFFFF, COL8_000000, "File not fond");
//         cons_newline(console);
//     }
//     cons_newline(console);
// }

int cmd_app(struct CONSOLE *console, int *fat, char *cmdline)
{
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
    struct FILEINFO *finfo;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
    char name[18], *p;
    int i;
    for (i = 0; i < 13; i++)
    {
        if (cmdline[i] <= ' ')
        {
            break;
        }
        name[i] = cmdline[i];
    }
    name[i] = 0;
    finfo = file_search(name, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
    if (finfo == 0 AND name[i - 1] != '.')
    {
        name[i] = '.';
        name[i + 1] = 'H';
        name[i + 2] = 'R';
        name[i + 3] = 'B';
        name[i + 4] = 0;
        finfo = file_search(name, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
    }
    if (finfo != 0)
    {
        p = (char *)memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
        set_segmdesc(gdt + 1003, finfo->size - 1, (int)p, AR_CODE32_ER);
        farcall(0, 1003 * 8);
        memman_free_4k(memman, (int)p, finfo->size);
        cons_newline(console);
        return 1;
    }

    return 0;
}