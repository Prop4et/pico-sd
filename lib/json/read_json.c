#include "read_json.h"
#include "sd_card.h"
#include "ff.h"
#include <string.h>
#include <ctype.h>

FRESULT frjson;
FIL fil;

#include <ctype.h>
#include <string.h>

static void trim_whitespace(const char *str, char *result, int result_size) {
  int n = strlen(str);
  int i = 0;
  int j = n - 1;

  while (i <= j && isspace(str[i])) i++;
  while (j > i && isspace(str[j])) j--;

  int new_len = j - i + 1;
  if (new_len >= result_size) {
    new_len = result_size - 1;
  }
  strncpy(result, str + i, new_len);
  result[new_len] = '\0';
}

static int string_to_int(const char *str) {
    int result = 0;
    int sign = 1;
    int i = 0;

    if (str[0] == '-') {
        sign = -1;
        i++;
    }

    while (str[i] != '\0') {
        result = result * 10 + str[i] - '0';
        i++;
    }

    return result * sign;
}

/**
 * @brief reads a key 
 * 
 * @param str json string
 * @param result where to save the key
 * @param result_size len of the key
 * @return int the amount of characters already read
 */
static int read_key(const char* str, char* result, int result_size){
    int i = 0, j = 0;
    int len = strlen(str);
    bool in_string = false;
    bool in_key = true;
    while(i<len && str[i] != ':'){
            if(str[i] == '"'){
                in_string = !in_string;
                i++;
                continue;
            }

            if(str[i] == '{'){
                result[j] = str[i];
                i++;
                return i;
            }

            if(str[i] == '}'){
                result[j] = str[i];
                i++;
                return i;
            }      

            if(in_string && in_key){
                result[j] = str[i];
                j += 1;
            }

            i++;
        }
    result[j] = '\0';
    return i;
}

static void parse_header(const char* str, char* key, struct configHeader* ch, int i){
    bool in_value = false;
    int k = 0;
    int len = strlen(str);
    char value[100] = " ";
    while(i<len){
        if(str[i] == '"'){
            in_value = !in_value;
            i++;
            continue;
        }
        if(in_value){
            value[k] = str[i];
            k += 1;
        }
        i++;
    }
    if(strncmp(key, DATECREATED, strlen(DATECREATED)) == 0){
        strncpy(ch->dateCreated, value, strlen(ch->dateCreated));
        ch->dateCreated[strlen(value)] = '\0';
    }
    if(strncmp(key, APPVERSION, strlen(APPVERSION)) == 0){
        strncpy(ch->appVersion, value, strlen(value));
        ch->appVersion[strlen(value)] = '\0';
    }
    if(strncmp(key, BOARDTYPE, strlen(BOARDTYPE)) == 0){
        strncpy(ch->boardType, value, strlen(value));
        ch->boardType[strlen(value)] = '\0';
    }
    if(strncmp(key, BOARDMODE, strlen(BOARDMODE)) == 0){
        strncpy(ch->boardMode, value, strlen(value));
        ch->boardMode[strlen(value)] = '\0';
    }
    if(strncmp(key, BOARDLAYOUT, strlen(BOARDLAYOUT)) == 0){
        strncpy(ch->boardLayout, value, strlen(value));
        ch->boardLayout[strlen(value)] = '\0';
    }
}

static void parse_heater_profiles(const char* str, char* key, struct heaterProfile* hp, int i){
    bool in_value = false;
    int k = 0;
    int len = strlen(str);
    char value[100] = "";

    while(i<len){
        
        if(str[i] == '"'){
            in_value = !in_value;
            i++;
            continue;
        }
        if(in_value || isdigit(str[i])){
            value[k] = str[i];
            k += 1;
        }
        i++;
    }
    if(strncmp(key, "id", 2) == 0){
        strncpy(hp->id, value, k);
        hp->id[k] = '\0';
    }

    if(strncmp(key, "timeBase", strlen("timeBase")) == 0){
        hp->timeBase = string_to_int(value);
    }
}

static void parse_array(const char* str, struct heaterProfile* hp, int *list_pos, int* arr_idx){
    int k = 0;
    int i = 0;
    int len = strlen(str);
    char val[100] = "";
    if(strncmp(str, "],", 2) == 0){
        //inc index array
        *arr_idx += 1;
    }
    while(i<len && str[i] != ','){
        if(isdigit(str[i])){
            val[k] = str[i];
            k +=1 ;
        }
        i++;
    }
    if(*list_pos == 0 && k > 0){
        //save temperature
        hp->temp_prof[*arr_idx] = string_to_int(val);
        *list_pos += 1;
        return;
    }
    if(*list_pos == 1 && k > 0){
        //save duration
        hp->mul_prof[*arr_idx] = string_to_int(val);
        *list_pos -= 1;
        return;
    }
}

static void parse_dutyc_profiles(const char* str, char* key, struct dutyCycleProfile* dcp, int i){
    bool in_value = false;
    int k = 0;
    int len = strlen(str);
    char value[100] = "";

    while(i<len){
        
        if(str[i] == '"'){
            in_value = !in_value;
            i++;
            continue;
        }
        if(in_value || isdigit(str[i])){
            value[k] = str[i];
            k += 1;
        }
        i++;
    }
    if(strncmp(key, "id", 2) == 0){
        strncpy(dcp->id, value, k);
        dcp->id[k] = '\0';
    }

    if(strncmp(key, "numberScanningCycles", strlen("numberScanningCycles")) == 0){
        dcp->numberScanningCycles = string_to_int(value);
    }

    if(strncmp(key, "numberSleepingCycles", strlen("numberSleepingCycles")) == 0){
        dcp->numberSleepingCycles = string_to_int(value);
    }    
}


static void parse_config_profiles(const char* str, char* key, struct sensorConfigurations* sc, int i){
    bool in_value = false;
    int k = 0;
    int len = strlen(str);
    char value[100] = "";

    while(i<len){
        
        if(str[i] == '"'){
            in_value = !in_value;
            i++;
            continue;
        }
        if(in_value || isdigit(str[i])){
            value[k] = str[i];
            k += 1;
        }
        i++;
    }
    if(strncmp(key, "sensorIndex", strlen("sensorIndex")) == 0){
        sc->index = string_to_int(value);
    }

    if(strncmp(key, "heaterProfile", strlen("heaterProfile")) == 0){
        strncpy(sc->h_p, value, strlen(value));
        sc->h_p[strlen(sc->h_p)] = '\0';
    }

    if(strncmp(key, "dutyCycleProfile", strlen("dutyCycleProfile")) == 0){
        strncpy(sc->d_c_prof , value, strlen(value));
        sc->d_c_prof[strlen(sc->d_c_prof)] = '\0';
    }

}

void read_json_file(char* filename, struct mainConfig* config){
    char buf[100];
    frjson = f_open(&fil, filename, FA_READ);
    bool header = false;
    bool body = false;
    bool heaterProfiles = false;
    bool dutyCycleProfiles = false;
    bool sensorConfiguration = false;
    bool temperatureTimeVectors = false;
    int prof_index;
    int list_pos;
    int arr_idx;
    if (frjson != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", frjson);
        while (true);
    }

    printf("...Reading from file '%s':\r\n", filename);
    while (f_gets(buf, sizeof(buf), &fil)) {
        char trimmed[100];
        char key[100] = "";
        trim_whitespace(buf, trimmed, 100);  //line read without spaces 
        int i = read_key(trimmed, key, 100);
        
        //set the state variable that we are in header
        if(strncmp(key, CONFIGHEADER, strlen(CONFIGHEADER)) == 0){
            header = true;
            body = false;
            continue;
        }

        //set the state variable that we are no longer in the header but in the body
        if(strncmp(key, CONFIGBODY, strlen(CONFIGBODY)) == 0){
            header = false;
            body = true;
            continue;
        }
        //parse the header
        if(header){
            parse_header(trimmed, key, &config->config_header, i);
        }
        //parse the body
        if(body && strncmp(key, HEATERPROFILES, strlen(HEATERPROFILES)) == 0){
            heaterProfiles = true;
            dutyCycleProfiles = false;
            sensorConfiguration = false;
            prof_index = 0;
            continue;
        }

        if(body && strncmp(key, DUTYCYCLEPROFILES, strlen(DUTYCYCLEPROFILES)) == 0){
            heaterProfiles = false;
            dutyCycleProfiles = true;
            struct dutyCycleProfile newProfile;
            config->duty_cycle_profile = newProfile;
            sensorConfiguration = false;
            continue;
        }

        if(body && strncmp(key, SENSORCONFIGURATIONS, strlen(SENSORCONFIGURATIONS)) == 0){
            heaterProfiles = false;
            dutyCycleProfiles = false;
            sensorConfiguration = true;
            prof_index = 0;
            continue;
        }

        if(heaterProfiles){
            if(key[0] == '{'){
                struct heaterProfile newProfile;
                config->heater_profile[prof_index] = newProfile;
                continue;
            }

            if(key[0] == '}'){
                prof_index += 1;
                temperatureTimeVectors = false;
                continue;
            }

            if(strncmp(key, TEMPTIMEVECTORS, strlen(TEMPTIMEVECTORS)) == 0){
                temperatureTimeVectors = true;
                list_pos = 0;
                arr_idx = 0;
                continue;
            }
            if(!temperatureTimeVectors){
                parse_heater_profiles(trimmed, key, &config->heater_profile[prof_index], i);
            }else{
                parse_array(trimmed, &config->heater_profile[prof_index], &list_pos, &arr_idx);
            }
        }

        if(dutyCycleProfiles){
            parse_dutyc_profiles(trimmed, key, &config->duty_cycle_profile, i);
        }
        
        if(sensorConfiguration){
            if(key[0] == '{'){
                struct sensorConfigurations newProfile;
                config->sensor_configurations[prof_index] = newProfile;
                continue;
            }

            if(key[0] == '}'){
                prof_index += 1;
                continue;
            }

            parse_config_profiles(trimmed, key, &config->sensor_configurations[prof_index], i);

        }
    }



    frjson = f_close(&fil);
    if (frjson != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", frjson);
        while (true);
    }
}