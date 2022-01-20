#include "bootpack.h"

void console_task(struct SHEET *sheet, int memtotal)
{
    struct TIMER *cursor_timer;
    char s[100];
    char *p;
    int i, fifobuf[128], cursor_x = 16, cursor_y = 28, cursor_c = -1;
    int x, y;
    struct TASK *task = task_now();
    struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);

    fifo32_init(&task->fifo, 128, fifobuf, task);
    // cursor
    cursor_timer = timer_alloc();
    timer_init(cursor_timer, &task->fifo, 1);
    timer_settime(cursor_timer, 50); // 0.5s光标闪烁
    char cmdline[100];
    struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;

    int *fat = (int *)memman_alloc_4k(memman, 4 * 2880);
    file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));

    putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">");
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
                    if (cursor_c >= 0)
                    {
                        cursor_c = COL8_000000;
                    }
                    break;
                case 1:
                    timer_init(cursor_timer, &task->fifo, 0);
                    if (cursor_c >= 0)
                    {
                        cursor_c = COL8_FFFFFF;
                    }
                    break;
                }
                timer_settime(cursor_timer, 50);
            }
            if (i == 2 OR i == 3) // 控制光标开启或者关闭
            {
                if (i == 2)
                {
                    cursor_c = COL8_FFFFFF;
                }
                else if (i == 3)
                {
                    boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, 28, cursor_x + 7, 43);
                    cursor_c = -1;
                }
            }
            if (256 <= i AND i <= 511) //来自taska的键盘数据
            {
                i -= 256;
                if (i == 8) //退格键
                {
                    if (cursor_x > 16)
                    {
                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
                        cursor_x -= 8;
                    }
                }
                else if (i == 10) // 回车键
                {
                    putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " "); //用空格将光标擦除
                    cmdline[cursor_x / 8 - 2] = 0;
                    cursor_y = cons_newline(cursor_y, sheet);
                    //执行命令
                    if (strcmp(cmdline, "mem") == 0)
                    {
                        sprintf(s, "total    %dMB", memtotal / (1024 * 1024));
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        cursor_y = cons_newline(cursor_y, sheet);
                        sprintf(s, "free     %dKB", memman_total(memman) / 1024);
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    else if (strcmp(cmdline, "clear") == 0)
                    {
                        for (y = 28; y < 28 + 128; y++)
                        {
                            for (x = 8; x < 8 + 240; x++)
                            {
                                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                            }
                        }
                        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
                        cursor_y = 28;
                    }
                    else if (strcmp(cmdline, "ls") == 0) // ls
                    {
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
                                    putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s);
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                            }
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    else if (strncmp(cmdline, "cat", 3) == 0) // cat 命令
                    {
                        //准备文件名
                        for (y = 0; y < 11; y++)
                        {
                            s[y] = ' ';
                        }
                        y = 0;
                        for (x = 4; y < 11 AND cmdline[x] != 0; x++)
                        {
                            if (cmdline[x] == '.' AND y <= 8)
                            {
                                y = 8;
                            }
                            else
                            {
                                s[y] = cmdline[x];
                                if ('a' <= s[y] AND s[y] <= 'z')
                                {
                                    s[y] -= 'a' - 'A'; // 转换为大写
                                }
                                y++;
                            }
                        }
                        //寻找文件
                        for (x = 0; x < 224;)
                        {
                            if (finfo[x].name[0] == 0x00)
                            {
                                break;
                            }
                            if ((finfo[x].type & 0x18) == 0)
                            {
                                for (y = 0; y < 11; y++)
                                {
                                    if (finfo[x].name[y] != s[y])
                                    {
                                        goto type_next_file;
                                    }
                                }
                                break; // 找到文件
                            }
                        type_next_file:
                            x++;
                        }
                        if (x < 224 AND finfo[x].name[0] != 0x00) // 找到文件的情况
                        {
                            y = finfo[x].size;
                            p = (char *)memman_alloc_4k(memman, finfo[x].size);
                            file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
                            cursor_x = 8;
                            for (y = 0; y < finfo[x].size; y++) //逐个输出
                            {
                                s[0] = p[y];
                                s[1] = 0;
                                if (s[0] == 0x09) // 制表符
                                {
                                    putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
                                    cursor_x += 8;
                                    if (cursor_x == 8 + 240)
                                    {
                                        cursor_x = 8;
                                        cursor_y = cons_newline(cursor_y, sheet);
                                    }
                                    if (((cursor_x - 8) & 0x1f) == 0)
                                    {
                                        break; //如果被32整除则break
                                    }
                                }
                                else if (s[0] == 0x0a) //换行
                                {
                                    cursor_x = 8;
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                                else if (s[0] == 0x0d) //回车
                                {
                                    ;
                                }
                                else
                                {
                                    putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s);
                                    cursor_x += 8;
                                    if (cursor_x == 8 + 240)
                                    {
                                        cursor_x = 8;
                                        cursor_y = cons_newline(cursor_y, sheet);
                                    }
                                }
                            }
                            memman_free_4k(memman, (int)p, finfo[x].size);
                        }
                        else //没有找到文件
                        {
                            putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File Not Found.");
                            cursor_y = cons_newline(cursor_y, sheet);
                        }
                    }
                    else if (cmdline[0] != 0) //未知命令
                    {
                        putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "Command Not Fond");
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">"); // 显示提示符
                    cursor_x = 16;
                }
                else // 一般字符
                {
                    if (cursor_x < 240)
                    {
                        s[0] = i;
                        s[1] = 0;
                        cmdline[cursor_x / 8 - 2] = i;
                        putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        cursor_x += 8;
                    }
                }
            }
            if (cursor_c >= 0) // 刷新光标
            {
                boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 16);
            }
            sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
        }
    }
}

int cons_newline(int cursor_y, struct SHEET *sheet)
{
    int x, y;
    if (cursor_y < 28 + 112)
    {
        cursor_y += 16; //换行
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
    return cursor_y;
}