#include "read_json.h"
#include "sd_card.h"
#include "ff.h"
#include <string.h>
#include <ctype.h>

FRESULT frjson;
FIL fil;



/*static char* trim_white_space(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
 
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}*/
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

static void parse_heater_profiles(const char* str, char* key, struct heaterProfile* hp, int t_t_idx, int i){
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
        if(in_value || str[i] == '0' || str[i] == '1' || str[i] == '2' || str[i] == '3' || str[i] == '4' || str[i] == '5' || str[i] == '6' || str[i] == '7' || str[i] == '8' || str[i] == '9'){
            value[k] = str[i];
            k += 1;
        }
        i++;
    }
    if(strncmp(key, "id", 2) == 0){
        strncpy(hp->id, value, strlen(value));
        hp->id[strlen(value)] = '\0';
    }

    if(strncmp(key, "timeBase", strlen("timeBase")) == 0){
        sscanf(value, "%d",  &hp->timeBase);
    }

    if(strncmp(key, "temperatureTimeVectors", strlen("temperatureTimeVectors")) == 0){
        
    }
}


void read_json_file(char* filename, struct mainConfig* config){
    char buf[100];
    frjson = f_open(&fil, filename, FA_READ);
    bool header = false;
    bool body = false;
    bool heaterProfiles = false;
    bool dutyCycleProfiles = false;
    bool sensorConfigurations = false;
    int heat_prof_index = 0;
    int temp_time_index = 0;
    if (frjson != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", frjson);
        while (true);
    }

    printf("...Reading from file '%s':\r\n", filename);
    while (f_gets(buf, sizeof(buf), &fil)) {
        char trimmed[100];
        char key[100];
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
            sensorConfigurations = false;
            continue;
        }

        if(body && strncmp(key, DUTYCYCLEPROFILES, strlen(DUTYCYCLEPROFILES)) == 0){
            heaterProfiles = false;
            dutyCycleProfiles = true;
            sensorConfigurations = false;
            continue;
        }

        if(body && strncmp(key, SENSORCONFIGURATIONS, strlen(SENSORCONFIGURATIONS)) == 0){
            heaterProfiles = false;
            dutyCycleProfiles = false;
            sensorConfigurations = true;
            continue;
        }

        if(heaterProfiles){
            if(key[0] == '{'){
                struct heaterProfile newProfile;
                config->heater_profile[heat_prof_index] = newProfile;
                continue;
            }

            if(key[0] == '}'){
                heat_prof_index += 1;
                continue;
            }

            parse_heater_profiles(trimmed, key, &config->heater_profile[heat_prof_index], temp_time_index, i);
        }

        /*if(dutyCycleProfiles){
            parse_heater_profiles();
        }

        if(sensorConfigurations){
            parse_heater_profiles();
        }*/
    }



    frjson = f_close(&fil);
    if (frjson != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", frjson);
        while (true);
    }
}