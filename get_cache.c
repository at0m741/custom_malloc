#include "include.h"

void get_cache_info() {
    unsigned int eax, ebx, ecx, edx;

    for (int i = 1; i < 4; i++) {
        ecx = i;

        asm volatile (
            "cpuid"
            : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
            : "a" (4), "c" (ecx)
        );

        unsigned int cache_type = eax & 0x1F;
        if (cache_type == 0) continue;

        unsigned int cache_level = (eax >> 5) & 0x7;
        unsigned int cache_size = ((ebx >> 22) + 1) * ((ebx >> 12 & 0x3FF) + 1) * ((ebx & 0xFFF) + 1) * (ecx + 1);

        printf("Cache level: L%d\n", cache_level);
        printf("Cache size: %u KB\n", cache_size / 1024);
		printf("\n");
    }
}
