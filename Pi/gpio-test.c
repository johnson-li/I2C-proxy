#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

#define BCM2711_PERI_BASE        0xFE000000
#define GPIO_BASE                (BCM2711_PERI_BASE + 0x200000) /* GPIO controller */
#define PAGE_SIZE                (4*1024)
#define BLOCK_SIZE               (4*1024)

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3)) // set as input

// Simple delay function
void delay_ms(unsigned int milliseconds){
    struct timespec req, rem;
    req.tv_sec = 0;
    req.tv_nsec = milliseconds * 1000000L;
    nanosleep(&req, &rem);
}

int main(int argc, char **argv) {
    int mem_fd;
    void *gpio_map;

    // volatile assures read/write ordering, important for hardware registers access
    volatile unsigned *gpio;

    // Open /dev/mem
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
        perror("can't open /dev/mem");
        exit(-1);
    }

    // Map GPIO into current process's address space
    gpio_map = mmap(
        NULL,                   // Any address in our space will do
        BLOCK_SIZE,             // Map length
        PROT_READ|PROT_WRITE,   // Enable reading & writing to mapped memory
        MAP_SHARED,             // Shared with other processes
        mem_fd,                 // File to map
        GPIO_BASE               // Offset to GPIO peripheral
    );

    close(mem_fd); // No need to keep mem_fd open after mmap

    if (gpio_map == MAP_FAILED) {
        perror("mmap error");
        exit(-1);
    }

    // Always use volatile pointer!
    gpio = (volatile unsigned *)gpio_map;

    // Configure GPIO24 and GPIO25 as input
    INP_GPIO(24);
    INP_GPIO(25);

    // Continuously read GPIO24 and GPIO25
    while(1) {
        unsigned int gpio_value = *(gpio + 13); // Read entire GPIO level register 0
        int state24 = (gpio_value & (1 << 24)) != 0; // Check if GPIO24 is high
        int state25 = (gpio_value & (1 << 25)) != 0; // Check if GPIO25 is high

        printf("GPIO24 is %s, GPIO25 is %s\n", state24 ? "HIGH" : "LOW", state25 ? "HIGH" : "LOW");

        delay_ms(1000); // Delay for 1 second
    }

    return 0;
}
