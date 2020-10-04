#include <stdio.h>
#include <string.h>

static int strrevcmp(char *l, char *r)
{
    // Needle from the end of strings
    char *nl = l + strlen(l);
    char *nr = r + strlen(r);
    for (; *nl == *nr && nl != l && nr != r; nl--, nr--);

    return *nl - *nr;
}

static int endswith(char *str, char *tail)
{
    int str_len = strlen(str);
    int tail_len = strlen(tail);

    if (str_len < tail_len)
        return 0;

    char *needle = str + str_len - tail_len;

    return 0 == strcmp(needle, tail);
}


int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: %s s1 s2\n", argv[0]);
        return 1;
    }

    printf("cmp: %d\n", strrevcmp(argv[1], argv[2]));
    printf("end: %d\n", endswith(argv[1], argv[2]));


    return 0;
}
