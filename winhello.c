int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_end(void);

char buf[150 * 50];

void HariMain(void)
{
    int win;
    win = api_openwin(buf,150,50,-1,"hello window");
    api_end();
}
