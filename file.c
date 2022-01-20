void file_readfat(int *fat, unsigned char *img)
{
    int i, j = 0;
    for (i = 0; i < 2880; i += 2)
    {
        fat[i + 0] = (img[j + 0] | img[j + 1] << 8) & 0xfff;
        fat[i + 1] = (img[j + 1] >> 4)
    }
}