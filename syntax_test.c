// Test syntax - zkontroluj pouze strukturu závorek
#include <windows.h>
#include <stdio.h>

void test() {
    for (int i = 0; i < 5; i++) {
        if (i == 1) {
            printf("test");
        } else {
            if (i == 2) {
                printf("test2");
            }
        }
    }
}

int main() {
    test();
    return 0;
}