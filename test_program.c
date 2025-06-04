#include <stdio.h>
#include <string.h>
#include "src/file_utils.h"

int main(void)
{
    // Test tee: read from stdin; but for automated test we'll skip.
    // Instead, we will open a string as input using /bin/echo.
    printf("Testing reverse_file...\n");
    FILE *f = fopen("input.txt", "w");
    if (!f) return 1;
    fputs("abcdef", f);
    fclose(f);

    if (reverse_file("input.txt", "output.txt") == 0) {
        char buf[7] = {0};
        FILE *rf = fopen("output.txt", "r");
        fread(buf, 1, 6, rf);
        fclose(rf);
        if (strcmp(buf, "fedcba") == 0)
            printf("reverse_file OK\n");
        else
            printf("reverse_file failed: %s\n", buf);
    } else {
        printf("reverse_file returned error\n");
    }

    if (reverse_file_optimized("input.txt", "output2.txt") == 0) {
        char buf[7] = {0};
        FILE *rf = fopen("output2.txt", "r");
        fread(buf, 1, 6, rf);
        fclose(rf);
        if (strcmp(buf, "fedcba") == 0)
            printf("reverse_file_optimized OK\n");
        else
            printf("reverse_file_optimized failed: %s\n", buf);
    } else {
        printf("reverse_file_optimized returned error\n");
    }

    return 0;
}
