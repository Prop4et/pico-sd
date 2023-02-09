#include "read_json.h"
#include "sd_card.h"
#include "ff.h"
#include <string.h>
#include <ctype.h>
FRESULT frjson;



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


void read_json_file(char* filename, struct mainConfig* config){
    FIL fil;
    char buf[100];
    frjson = f_open(&fil, filename, FA_READ);
    
    if (frjson != FR_OK) {
        printf("ERROR: Could not open file (%d)\r\n", frjson);
        while (true);
    }

    printf("...Reading from file '%s':\r\n", filename);
    while (f_gets(buf, sizeof(buf), &fil)) {
        char trimmed[100];
        trim_whitespace(buf, trimmed, 100);  //line read without spaces 
        char c;
        char key[100] = " ";
        char value[100] = " ";
        int i = 0, j = 0, k = 0;
        int len = strlen(trimmed);
        bool in_string = false;
        bool in_key = true;
        bool in_value = false;
        while(i<len && trimmed[i] != ':'){
            if(trimmed[i] == '"'){
                in_string = !in_string;
                i++;
                continue;
            }

            if(in_string && in_key){
                key[j] = trimmed[i];
                j += 1;
            }

            i++;
            /*if(!in_string && c == ':' ){
                in_value = true;
                in_key = false;
                continue;
            }

            if(in_string && in_value){
                value[k] = c;
                k += 1;
            }*/
        }
        key[j] = 0;
        if(strncmp(key, DATECREATED, strlen(DATECREATED)) == 0){
            while(i<len){
                if(trimmed[i] == '"'){
                    in_value = !in_value;
                    i++;
                    continue;
                }
                if(in_value){
                    config->config_header.dateCreated[k] = trimmed[i];
                    k += 1;
                }
                i++;
            }
            config->config_header.dateCreated[k] = 0;
            printf("Key: %s\n", key);
            printf("Value: %s\n", config->config_header.dateCreated);
        }
        
    }

    frjson = f_close(&fil);
    if (frjson != FR_OK) {
        printf("ERROR: Could not close file (%d)\r\n", frjson);
        while (true);
    }
}