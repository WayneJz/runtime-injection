#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

__attribute__ ((aligned(8)))
__attribute__ ((noinline))

/*
 * Author: WayneJz
 * Demo: A runtime patch with modifying memory pages
 *
 * To compile: gcc -std=c11 runtime_patch.c -o runtime_patch
 * Expected output: "hello world!" then "hi hacker!" as the first function was patched by the second one in runtime;
 *
 * NOTE: IF RUNNING ON MacOS 10.15 OR ABOVE
 *
 * Please run command "printf '\x07' | dd of=runtime_patch bs=1 seek=160 count=1 conv=notrunc" AFTER successfully compiled
 * Otherwise MacOS will fail to modify pages
 * As is a protection mechanism by default since MacOS 10.15
 *
 * ref: https://stackoverflow.com/questions/60654834/using-mprotect-to-make-text-segment-writable-on-macos
 * ref: https://nullprogram.com/blog/2016/03/31/
 */

void helloworld() {
    printf("hello world!\n");
}

void hihacker() {
    printf("hi hacker!\n");
}

void* unprotect_page(void *address, size_t size) {
    address = (void *)((uintptr_t)address & ~0xfff);

    int ret = mprotect(address, size, PROT_READ | PROT_WRITE | PROT_EXEC);

    if (ret == 0) {
        return address;
    } else {
        printf("unprotect error, ret=%d, errno=%d\n", ret, errno);
        return NULL;
    }
}

void reprotect_page(void *address, size_t size) {
    int ret = mprotect(address, size, PROT_READ | PROT_EXEC);
    if (ret == 0) {
        return;
    }
    printf("reprotect error, ret=%d, errno=%d\n", ret, errno);
}

void patch(void* source, void* replacement) {
    uintptr_t relative = replacement - source - 5;
    union {
        uint8_t bytes[8];
        uint64_t value;
    } instruction = { {0xe9, relative, relative >> 8, relative >> 16, relative >> 24} };
    *(uint64_t *)source = instruction.value;
}

int main() {
    helloworld();
    void* p = unprotect_page(helloworld, 4096);
    patch(helloworld, hihacker);
    reprotect_page(p, 4096);
    helloworld();
}