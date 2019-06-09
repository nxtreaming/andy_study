/* skeleton code for the conversion of decimal to other based number */
/* gcc -o calc calc.c */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int x, y, z, base, i = 0;
    char c, vals[64] = { 0 };

    /* input */
    printf("Please input the positive integer which is based on decimal:");
    scanf("%d", &x);
    if ( x < 0) {
        printf("Only positive integer is allowed, exit\n");
        exit(1);
    }

    printf("Please input the conversion base <between 2 and 37>:");
    scanf("%d", &base);
    if (base < 2 || base > 37) {
        printf("Out of range, exit\n");
        exit(1);
    }

    printf("------------------------------\nThe decimal number is: %d\n", x);
    do {
        /* quotient by 'base' */
        y = x / base;

        /* mod by 'base' */
        /* z = x % base */
        z = x - y * base;

        if (z < 10)
            c = '0' + z;
        else
            c = 'A' + (z - 10);
        vals[i] = c;
        i = i + 1;
        if (i >= 64) {
            printf("Too huge, exit\n");
            exit(1);
        }

        /* reload 'x' for next loop */
        x = y;
    } while (y != 0);

    /* output */
    printf("The %d based number is: ", base);
    do {
        i = i - 1;
        printf("%c", vals[i]);
    } while (i > 0);
    printf("\n");

    return 0;
}
