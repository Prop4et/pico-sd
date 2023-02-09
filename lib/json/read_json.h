#include <stdio.h>
#include "pico/stdlib.h"

#define DATECREATED     "dateCreated"
#define APPVERSION      "appVersion"
#define BOARDTYPE       "boardType"
#define BOARDMODE       "boardMode"
#define BOARDLAYOUT     "boardLayout"
struct configHeader {
    char dateCreated[24];
    char appVersion[5];
    char boardType[7];
    char boardMode[125];
    char boardLayout[125];
};

struct heaterProfile{
    char id[10];
    uint32_t timeBase;
    uint16_t temp_prof[10];
    uint16_t mul_prof[10];

};

struct dutyCycleProfile{
    char id[8];
    uint8_t numberScanningCycles;
    uint8_t numberSleepingCycles;
};

struct sensorConfigurations{
    uint8_t index;
    char heater_prof[10]; //same value as in id in heaterProfile
    char id[8]; //same value as id in dutyCycleProfile
};

struct mainConfig{ 
    struct configHeader config_header;
    struct heaterProfile heater_profile[4];
    struct dutyCycleProfile duty_cycle_profile;
    struct sensorConfigurations sensor_configurations[8];
};

void read_json_file(char* filename, struct mainConfig* config);