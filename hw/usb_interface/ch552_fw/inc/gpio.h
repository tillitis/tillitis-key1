#ifndef __GPIO_H__
#define __GPIO_H__

#include <stdint.h>

#include "main.h"

// Set pin p1.4 and p1.5 to GPIO output mode.
void gpio_init(void);
void gpio_set(uint8_t pin);
void gpio_unset(uint8_t pin);
uint8_t gpio_get(uint8_t pin);

void gpio_init_p1_4_in(void);
void gpio_init_p1_5_out(void);
uint8_t gpio_p1_4_get(void);
void gpio_p1_5_set(void);
void gpio_p1_5_unset(void);

#endif
