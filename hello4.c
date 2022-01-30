void api_putstring(char *s);
void api_end(void);

void HariMain(void)
{
    api_putstring("hello,world");
    api_end();
}