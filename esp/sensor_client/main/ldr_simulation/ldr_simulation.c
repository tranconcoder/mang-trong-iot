#include <stdio.h>
#include "ldr_simulation.h"

int light_level = 0;

int get_light_level(void) {
    bool is_increase = rand() % 2;

    if (is_increase) {
        light_level = light_level + (rand() % 10 - 5);
    } else {
        light_level = light_level - (rand() % 10 - 5);
    }

    light_level = light_level < 0 ? 0 : light_level;
    light_level = light_level > 100 ? 100 : light_level;

    return light_level;
}