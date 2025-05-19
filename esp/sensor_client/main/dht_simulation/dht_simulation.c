#include <stdio.h>
#include "dht_simulation.h"

int temperature = 20;
int humidity = 50;

int get_temperature(void) {
    bool is_increase = rand() % 2;

    if (is_increase) {
        temperature = temperature + (rand() % 10 - 5);
    } else {
        temperature = temperature - (rand() % 10 - 5);
    }

    temperature = temperature < 0 ? 0 : temperature;
    temperature = temperature > 50 ? 50 : temperature;

    return temperature;
}

int get_humidity(void) {
    bool is_increase = rand() % 2;

    if (is_increase) {
        humidity = humidity + (rand() % 10 - 5);
    } else {
        humidity = humidity - (rand() % 10 - 5);
    }

    humidity = humidity < 0 ? 0 : humidity;
    humidity = humidity > 100 ? 100 : humidity;

    return humidity;
}