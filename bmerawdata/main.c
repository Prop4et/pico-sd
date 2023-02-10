#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "../lib/bme68x/bme68x.h"
#include "../lib/bme_api/bme68x_API.h"
#include "../lib/bsec/bsec_datatypes.h"
#include "../lib/bsec/bsec_interface.h"
#include "sd_card.h"
#include "ff.h"
#include "../lib/json/read_json.h"

void print_results(int id, float signal, int accuracy){
    switch(id){
        case BSEC_OUTPUT_IAQ:
            printf("IAQ   Accuracy\n");
            printf("%.2f  %d \n", signal, accuracy);
            break;
        case BSEC_OUTPUT_STATIC_IAQ:
            printf("STATIC IAQ\n");
            break;
        case BSEC_OUTPUT_CO2_EQUIVALENT:
            printf("CO2[ppm]  Accuracy\n");
            printf("%.2f   %d \n", signal, accuracy);
            break;
        case BSEC_OUTPUT_RAW_TEMPERATURE:
                printf("Temperature[°C] \n");
                printf("%.2f \n", signal);
            break;
        case BSEC_OUTPUT_RAW_HUMIDITY:
                printf("Humidity[%%rH] \n");
                printf("%.2f\n", signal);
            break;
        case BSEC_OUTPUT_RAW_PRESSURE:
                printf("Pressure[hPa] \n");
                printf("%.2f \n", signal);
            break;
        case BSEC_OUTPUT_RAW_GAS:
            printf("[Ohm]    | Accuracy\n");
            printf("%.2f     | %d \n", signal, accuracy);
            break;
        case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
            printf("VOC\n");
            break;
    }
}

int main(){
    /*
    BSEC VARIABLES
*/
    bsec_library_return_t rslt_bsec;
    //measurements basically
    bsec_sensor_configuration_t requested_virtual_sensors[4];
    uint8_t n_requested_virtual_sensors = 4;
    // Allocate a struct for the returned physical sensor settings
    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR]; 
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;
    //configuration coming from bsec
    bsec_bme_settings_t conf_bsec;
    bsec_input_t input[4];
    uint8_t n_input = 4;
    bsec_output_t output[4];
    uint8_t n_output=4;
    //state handling
    uint8_t serialized_state[BSEC_MAX_STATE_BLOB_SIZE];
    uint32_t n_serialized_state_max = BSEC_MAX_STATE_BLOB_SIZE;
    uint32_t n_serialized_state = BSEC_MAX_STATE_BLOB_SIZE;
    uint8_t work_buffer_state[BSEC_MAX_WORKBUFFER_SIZE];
    uint32_t n_work_buffer_size = BSEC_MAX_WORKBUFFER_SIZE;
    //configuration on shut down
    uint8_t serialized_settings[BSEC_MAX_PROPERTY_BLOB_SIZE];
    uint32_t n_serialized_settings_max = BSEC_MAX_PROPERTY_BLOB_SIZE;
    uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE];
    uint32_t n_work_buffer = BSEC_MAX_WORKBUFFER_SIZE;
    uint32_t n_serialized_settings = 0;
    //
    uint64_t time_us;
    /*
        BME API VARIABLES
    */
    struct bme68x_dev bme;
    struct bme68x_data data[3];
    struct bme68x_conf conf;
    struct bme68x_heatr_conf heatr_conf;
    uint16_t sample_count = 1;
    int8_t rslt_api;
    /*
        Config struct
    */
    struct mainConfig config;
    /*
        File System variables
    */
    FRESULT fr;
    FATFS fs;
    int ret;
    stdio_init_all();
    sleep_ms(5000);
    
    int8_t rslt;
    /*
        File System op
    */
    if (!sd_init_driver()) {
        printf("ERROR: Could not initialize SD card\r\n");
        while (true);
    }

    // Mount drive
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        printf("ERROR: Could not mount filesystem (%d)\r\n", fr);
        while (true);
    }
    //read file
    
    read_json_file("default1.bmeconfig", &config);
    sleep_ms(2);
    printf("%s\n", config.config_header.dateCreated);
    printf("%s\n", config.config_header.appVersion);
    printf("%s\n", config.config_header.boardType);
    printf("%s\n", config.config_header.boardMode);
    printf("%s\n", config.config_header.boardLayout);
    printf("---------------heater profile-------------------\n");
    for(int idx = 0; idx < 4; idx++){
        printf("%s\n", config.heater_profile[idx].id);
        printf("%d\n", config.heater_profile[idx].timeBase);
        for(int idx2 = 0; idx2 < 10; idx2++){
            printf("%d ", config.heater_profile[idx].temp_prof[idx2]);
        }
        printf("\n");
        for(int idx2 = 0; idx2 < 10; idx2++){
            printf("%d ", config.heater_profile[idx].mul_prof[idx2]);
        }
        printf("\n");

    }
    printf("---------------duty cycle profile-----------------\n");
    printf("%s\n", config.duty_cycle_profile.id);
    printf("%d\n", config.duty_cycle_profile.numberScanningCycles);
    printf("%d\n", config.duty_cycle_profile.numberSleepingCycles);
    printf("-----------------sensorconfig-------------------\n");
    for(int idx = 0; idx<8; idx++){
        printf("%d\n", config.sensor_configurations[idx].index);
        printf("%s\n", config.sensor_configurations[idx].h_p);
        printf("%s\n", config.sensor_configurations[idx].d_c_prof);
    }
    






    // Unmount drive
    f_unmount("0:");

    /* Heater temperature in degree Celsius */
    uint16_t temp_prof[10] = { 100, 100, 200, 200, 200, 200, 320, 320, 320, 320 };

    /* Multiplier to the shared heater duration */
    uint16_t mul_prof[10] = { 2, 41, 2, 14, 14, 14, 2, 14, 14, 14};

    //sleep to open the serial monitor in time
    sleep_ms(10000);

    printf("...initialization BSEC\n");
    rslt_bsec = bsec_init();
    check_rslt_bsec(rslt_bsec, "BSEC_INIT"); 
    /*
        INITIALIZATION BSEC LIBRARY
    */
    requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_RAW_TEMPERATURE;
    requested_virtual_sensors[0].sample_rate = BSEC_SAMPLE_RATE_LP;
    requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_RAW_PRESSURE;
    requested_virtual_sensors[1].sample_rate = BSEC_SAMPLE_RATE_LP;
    requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_RAW_HUMIDITY;
    requested_virtual_sensors[2].sample_rate = BSEC_SAMPLE_RATE_LP;
    requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_RAW_GAS;
    requested_virtual_sensors[3].sample_rate = BSEC_SAMPLE_RATE_LP;

    rslt_bsec = bsec_update_subscription(requested_virtual_sensors, n_requested_virtual_sensors, required_sensor_settings, &n_required_sensor_settings);
    check_rslt_bsec(rslt_bsec, "BSEC_UPDATE_SUBSCRIPTION");

    rslt_bsec = bsec_sensor_control(time_us_64()*1000, &conf_bsec);
    check_rslt_bsec(rslt_bsec, "BSEC_SENSOR_CONTROL");

    printf("Init BME688\n");
    bme_interface_init(&bme, BME68X_I2C_INTF);
    uint8_t data_id[8];

    //read device id after init to see if everything works for now and to check that the device communicates with I2C
    bme.read(BME68X_REG_CHIP_ID, data_id, 1, bme.intf_ptr);
    if(data_id[0] != BME68X_CHIP_ID){
        printf("Cannot communicate with BME688\n");
        printf("CHIP_ID: %x\t ID_READ: %x\n", BME68X_CHIP_ID, data_id[0]);
    }
    else{
        bme.chip_id = data_id[0];
        printf("Connection valid, DEVICE_ID: %x\n", bme.chip_id);
    }

    sleep_ms(500);
    
    //initialize the structure with all the parameters by reading from the registers
    rslt = bme68x_init(&bme);
    check_rslt_api(rslt, "INIT");

    rslt = bme68x_get_conf(&conf, &bme);
    check_rslt_api(rslt, "bme68x_get_conf");

    /*
        Set oversampling for measurements
        1x oversampling for humidity
        16x oversampling for pressure
        2x oversampling for temperature
    */
    conf.os_hum = BME68X_OS_1X;
    conf.os_pres = BME68X_OS_16X;
    conf.os_temp = BME68X_OS_2X;
    conf.filter = BME68X_FILTER_OFF;
    conf.odr = BME68X_ODR_NONE;

    rslt = bme68x_set_conf(&conf, &bme);
    check_rslt_api(rslt, "bme68x_set_conf");

    /*  
        Set the remaining gas sensor settings and link the heating profile 
        enable the heater plate
        set the temperature plate to 300°C
        set the duration to 100 ms
    */
    heatr_conf.enable = BME68X_ENABLE;
    heatr_conf.heatr_temp_prof = temp_prof;
    heatr_conf.heatr_dur_prof = mul_prof;

    /* Shared heating duration in milliseconds */
    heatr_conf.shared_heatr_dur = 140 - (bme68x_get_meas_dur(BME68X_PARALLEL_MODE, &conf, &bme) / 1000);
    
    heatr_conf.profile_len = 10;

    rslt = bme68x_set_heatr_conf(BME68X_PARALLEL_MODE, &heatr_conf, &bme);
    check_rslt_api(rslt, "bme68x_set_heatr_conf");

    
    rslt = bme68x_set_op_mode(BME68X_PARALLEL_MODE, &bme); 
    check_rslt_api(rslt, "bme68x_set_op_mode");

    printf("Print parallel mode data if mask for new data(0x80), gas measurement(0x20) and heater stability(0x10) are set\n\n");
    sleep_ms(2);
    
    uint8_t n_fields;
    uint32_t del_period;

    while(1){

        del_period = bme68x_get_meas_dur(BME68X_PARALLEL_MODE, &conf, &bme) + (heatr_conf.heatr_dur * 1000);

        bme.delay_us(del_period, bme.intf_ptr);

        rslt = bme68x_get_data(BME68X_PARALLEL_MODE, data, &n_fields, &bme);
        check_rslt_api(rslt, "bme68x_get_data");
        
        if (rslt == BME68X_OK){
            printf("ITS OK\n");
            sleep_ms(2);
            for (uint8_t i = 0; i < n_fields; i++){
                time_us = time_us_64()*1000;
                input[0].sensor_id = BSEC_INPUT_GASRESISTOR;
                input[0].signal = data[i].gas_resistance;
                input[0].time_stamp= time_us;
                input[1].sensor_id = BSEC_INPUT_TEMPERATURE;
                input[1].signal = data[i].temperature;
                input[1].time_stamp= time_us;
                input[2].sensor_id = BSEC_INPUT_HUMIDITY;
                input[2].signal = data[i].humidity;
                input[2].time_stamp= time_us;
                input[3].sensor_id = BSEC_INPUT_PRESSURE;
                input[3].signal = data[i].pressure;
                input[3].time_stamp = time_us;

                rslt_bsec = bsec_do_steps(input, n_input, output, &n_output);
                if(rslt_bsec == BSEC_OK){
                    for(int j = 0; j < n_output; j++){
                        print_results(output[j].sensor_id, output[j].signal, output[j].accuracy);
                        sleep_ms(2);
                    }
                    printf("\n");
                }
            }
        }
        sleep_ms(30000);
    }

}