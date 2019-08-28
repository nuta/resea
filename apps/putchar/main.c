#include <resea.h>

int main(void) {
    int i = 0;
    while (1) {
        for (char ch = 'A'; ch <= 'Z'; ch++) {
            putchar(ch);
            if (++i % 100 == 0) {
                putchar('\n');
            }

            for (volatile int delay = 0x10000; delay > 0; delay--)
                ;
        }
    }

    return 0;
}
