#ifndef RTM_X86INTRIN_H
#define RTM_X86INTRIN_H

#include <stdint.h>
#include_next <x86intrin.h>


static inline uint32_t log2u(const uint32_t x)
{
#if defined(__x86_64__) || defined(__i386__)
    uint32_t y;
    asm("bsr %1, %0\n" : "=r"(y) : "r"(x));
    return y;
#elif defined(__arm__) || defined(__aarch64__)
    return x ? (31 - __builtin_clz(x)) : 0;
#endif
}


#endif //RTM_X86INTRIN_H
