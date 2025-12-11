#include<stdio.h>
#include<stdlib.h>

void printc(int len, int buf[]) {
    printf("|");
    for (int i =0; i < len; i++) {
        if (buf[i] >= 0x20 && buf[i] <= 0x7F) printf("%c", buf[i]);
        else printf(".");
    }
    printf("|");
}

int main() {
    int c;
    int mem_id = 0;
    int buf[0x10];
    while ((c = getchar()) != EOF){
        buf[mem_id % 0x10] = c;
        if (mem_id % 0x10 == 0x0) {
            printf("%08x ", mem_id);

        }
        
        printf("%02x ", buf[mem_id % 0x10]);
        mem_id++;
            
        if (mem_id % 0x10 == 0x0) {
            printc(0x10, buf);
            printf("\n");
        }
    }
    printf("%*s", 3*(0x10-(mem_id%0x10)), " ");
    printc(mem_id % 0x10, buf);
    printf("\n");
    printf("%08x ", mem_id);
    printf("\n");
    return 0;
}
