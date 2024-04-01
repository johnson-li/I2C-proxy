#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>

#define BCM2711_PERI_BASE        0xFE000000
#define GPIO_BASE                (BCM2711_PERI_BASE + 0x200000) /* GPIO controller */
#define PAGE_SIZE                (4*1024)
#define BLOCK_SIZE               (4*1024)
#define SCL_PIN 24
#define SDA_PIN 25
#define BUFFER_LIMIT (1024 * 1024)
#define PRINT_INTERVAL 2000

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3)) // set as input

struct timeval start_ts;

int main(int argc, char **argv) {
    gettimeofday(&start_ts, NULL);
    int mem_fd;
    void *gpio_map;
    char buffer[BUFFER_LIMIT];
    int buffer_size = 0;
    int print_index = 0;

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
    INP_GPIO(SCL_PIN);
    INP_GPIO(SDA_PIN);

    int data = 0;
    int condition = -1; // -1, uninitialized, 0: normal data, 1: START, 2: STOP
    // Continuously read SCL and SDA 
    while(1) {
        unsigned int gpio_value = *(gpio + 13); // Read entire GPIO level register 0
        int scl = (gpio_value & (1 << SCL_PIN)) != 0; // Check if SCL is high
        int sda = (gpio_value & (1 << SDA_PIN)) != 0; // Check if SDA is high

        // Refer to state_inference.ino for the I2C monitor logic.

        // Do not process SDA if the SCL is low
        if (!scl) {
            // SCL is low, optionally record SDA/condition and reset the condition 
            if (condition >= 0) {
                if (buffer_size >= BUFFER_LIMIT) {
                    printf("ERROR: Buffer overflow");
                } else {
                    buffer[buffer_size++] = (condition << 4) | data;
                }
            }
            condition = -1;
        } else {
            // SCL is high, read SDA
            if (condition >= 0) {
                if (data != sda) {
                    if (condition > 0) {
                        // SDA should not change twice during a high SCL
                        printf("ERROR: SDA changes twice during a high SCL");
                    } else {
                        if (data) {
                            condition = 1;
                        } else {
                            condition = 2;
                        }
                    }
                }
            } else {
                condition = 0;
            }
            data = sda;
        }

        // Print buffer size every PRINT_INTERVAL ms
        struct timeval ts;
        gettimeofday(&ts, NULL);
        int diff = (ts.tv_sec - start_ts.tv_sec) * 1000 + (ts.tv_usec - start_ts.tv_usec) / 1000;
        if (diff - print_index * PRINT_INTERVAL >= PRINT_INTERVAL) {
            printf("Buffer size: %d\n", buffer_size);
            for (int i = 0; i < buffer_size; i++) {
                int condition = buffer[i] >> 4;
                int data = buffer[i] & 0x01;
                if (condition == 0) {
                    printf("Data: %d\n", data);
                } else if (condition == 1) {
                    printf("START\n");
                } else if (condition == 2) {
                    printf("STOP\n");
                } else {
                    printf("ERROR: Unknown condition %d\n", condition);
                }
            }
            buffer_size = 0;
            print_index++;
        }
    }

    return 0;
}
