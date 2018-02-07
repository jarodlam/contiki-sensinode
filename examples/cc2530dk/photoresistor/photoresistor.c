#include "contiki.h"
#include <stdio.h>
#include "cc253x.h"
#include "contiki-conf.h"

// #define DEBUG 1

#define BIT0 0x00  /* 00000000 */
#define BIT1 0x01  /* 00000001 */
#define BIT2 0x02  /* 00000010 */
#define BIT3 0x04  /* 00000100 */
#define BIT4 0x08  /* 00001000 */
#define BIT5 0x10  /* 00010000 */
#define BIT6 0x20  /* 00100000 */
#define BIT7 0x40  /* 01000000 */
#define BIT8 0x80  /* 10000000 */

/* Select analogue input 6 */
#define PINNUM 0x01 /*     0001 */
#define PINBIT 0x01 /* 00000001 */

/*---------------------------------------------------------------------------*/
PROCESS(sensor_read_process, "Sensor read process");
AUTOSTART_PROCESSES(&sensor_read_process);
/*---------------------------------------------------------------------------*/
static struct etimer et;
static uint16_t ReadLightSensor()
{
	uint8_t command;
	uint16_t reading;

	/* Configure pins */
#ifdef DEBUG
	printf("Set P0DIR.\n");
#endif
    P0DIR = ~(~P0DIR | PINBIT);  /* Set pin as input */
#ifdef DEBUG
	printf("Set P0INP.\n");
#endif
    P0INP |= PINBIT;  /* Set as tri-state */
#ifdef DEBUG
	printf("Set APCFG.\n");
#endif
    APCFG |= PINBIT;  /* configure ADC on pin */

#ifdef DEBUG
	printf("Construct ADCCON3 command.\n");
#endif
    /* Construct a command to send to the ADCCON3 register. See p138 of user guide. */
    command = BIT5 | BIT6;  /* Configure to use internal 1.25 V reference and max decimation rate. */
    command |= PINNUM;  /* Choose the channel for the ADC to use. */
	/* Write to the ADCCON3 register. */
#ifdef DEBUG
	printf("Command: 0x%x\n", command);
#endif
	ADCCON3 = command;

	/* Wait for the ADC interrupt flag to be set. Timeout after 1 s.*/
#ifdef DEBUG
	printf("Waiting for ADC interrupt flag.\n");
#endif
	etimer_set(&et, CLOCK_SECOND);
    while (!ADCIF) {
    	if(etimer_expired(&et)) {
    		continue;
    	}
    }
	etimer_reset(&et);
    ADCIF = 0;

    /* Read the high and low bytes. */
	reading = ADCL;
    reading |= (((uint8_t) ADCH) << 8);
	/* We only need 8 bits of resolution. */
	reading = reading >> 7;
	
	return reading;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_read_process, ev, data)
{
	uint8_t reading;

	PROCESS_BEGIN();

	printf("Program begin.\n");

	while(1) {
		reading = ReadLightSensor();
		printf("%d\n", reading);
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
