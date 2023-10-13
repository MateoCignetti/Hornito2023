/*-------------- Includes --------------*/
#include <stdio.h>
#include <stdbool.h>
/*-------------- Defines ---------------*/
#define BUFFER_SIZE 10
#define MAV_SIZE BUFFER_SIZE/2

static uint16_t adcBuffer[BUFFER_SIZE]={1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static uint16_t resultado[BUFFER_SIZE]={0};
static uint16_t mavBuffer[MAV_SIZE]={0};

/*------------- Function prototypes -----*/
void mav_filter(void);
void shift_mav_filter(void);

void app_main(void) {
mav_filter();
}

/*---------------- Functions -----------------*/
void mav_filter(void){
    uint16_t sumMAV;
    uint16_t suma=0;
    for (int k = 0; k < BUFFER_SIZE; k++) {
        mavBuffer[0] = adcBuffer[k];
        sumMAV = 0;
        for(int j=0; j < MAV_SIZE; j++){
            sumMAV += mavBuffer[j];
        }
        //printf("%d\n", sumMAV); // hasta acá anda        
        shift_mav_filter();
        suma=sumMAV/((uint16_t) MAV_SIZE);
        printf("%d\n", suma);
    }
}

void shift_mav_filter(void){
    for (int i = (MAV_SIZE-1); i > 0; i--) {
        mavBuffer[i] = mavBuffer[i-1];
        //printf("%d\n", mavBuffer[i]);
    }
    /*for (int j = 0; j < MAV_SIZE; j++) {
        printf("%d", mavBuffer[j]);
    }*/
    
}