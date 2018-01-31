#include "contiki.h"
#include <stdio.h>
#include "cc253x.h"
#include "contiki-conf.h"

#define AIN0 0x00  /* 0000 */
#define AIN1 0x01  /* 0001 */
#define AIN2 0x02  /* 0010 */
#define AIN3 0x03  /* 0011 */
#define AIN4 0x04  /* 0100 */
#define AIN5 0x05  /* 0101 */
#define AIN6 0x06  /* 0110 */
#define AIN7 0x07  /* 0111 */

#define BIT0 0x00  /* 00000000 */
#define BIT1 0x01  /* 00000001 */
#define BIT2 0x02  /* 00000010 */
#define BIT3 0x04  /* 00000100 */
#define BIT4 0x08  /* 00001000 */
#define BIT5 0x10  /* 00010000 */
#define BIT6 0x20  /* 00100000 */
#define BIT7 0x40  /* 01000000 */
#define BIT8 0x80  /* 10000000 */

/*---------------------------------------------------------------------------*/
PROCESS(sensor_read_process, "Sensor read process");
AUTOSTART_PROCESSES(&sensor_read_process);
/*---------------------------------------------------------------------------*/
// static uint8_t ReadLightSensor()
// {
// 	uint8_t command;
// 	uint16_t reading;
//
// 	// printf("Set registers.\n");
// 	/* Select pin in hex form */
//     P0SEL |= BIT6;  /* Set pin (sample) as GPIO */
//     P0DIR ~= ~P0DIR | BIT6;  /* Set pin as input */
//     P0INP |= BIT6;  /* Set as tri-state */
//     APCFG |= BIT6;  /* configure ADC on pin */
//
// 	// printf("Construct ADCCON3 command.\n");
//     /* Construct a command to send to the ADCCON3 register. */
// 	/* See p138 of user guide. */
// 	/* Configure to use internal 1.25 V reference and max decimation rate. */
//     command = ADCCON3_EDIV1 | ADCCON3_EDIV0;
// 	/* Choose the channel for the ADC to use. */
//     command |= AIN6;
// 	/* Write to the ADCCON3 register. */
// 	ADCCON3 = command;
// 	// printf("Command: 0x%x\n", command);
//
// 	// printf("Waiting for ADC interrupt flag.\n");
// 	/* Wait for the ADC interrupt flag to be set. */
//     while (!ADCIF);
//     ADCIF = 0;
//
// 	// printf("Combining low and high bytes.\n");
// 	/* Combine low ADCL and high ADCH bytes. */
//     reading = ADCL;
//     reading |= (((uint8_t) ADCH) << 8);
// 	// printf("Reading: 0x%x\n");
//
// 	// printf("Returning reading.\n");
// 	return reading;
// }
static struct etimer et;
static uint8_t ReadLightSensor()
{
	uint8_t command;
	uint16_t reading;
	uint8_t ains[] = {0x00,0x01,0x02,0x03,/*0x04,*/0x05,0x06,0x07};
	uint8_t bits[] = {0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40};
	
	int i;

	/* Configure pins */
	// printf("Set P0DIR.\n");
    P0DIR = 0x00; //~ (~P0DIR | bits[i]);  /* Set pin as input */
	// printf("Set P0INP.\n");
    P0INP = 0xFF; //|= bits[i];  /* Set as tri-state */
	// printf("Set APCFG.\n");
    APCFG = 0xF7; //|= bits[i];  /* configure ADC on pin */
	
	for (i=0; i<=6; i++) {
		// printf("Construct ADCCON3 command.\n");
	    /* Construct a command to send to the ADCCON3 register. */
		/* See p138 of user guide. */
		/* Configure to use internal 1.25 V reference and max decimation rate. */
	    command = BIT5 | BIT6;
		/* Choose the channel for the ADC to use. */
	    command |= ains[i];
		/* Write to the ADCCON3 register. */
		// printf("Command: 0x%x\n", command);
		ADCCON3 = command;
	
		/* Wait for the ADC interrupt flag to be set. */
		// printf("Waiting for ADC interrupt flag.\n");
		etimer_set(&et, CLOCK_SECOND);
	    while (!ADCIF) {
			/* Timeout */
	    	if(etimer_expired(&et)) {
	    		continue;
	    	}
	    }
		etimer_reset(&et);
	    ADCIF = 0;
	
		/* Combine low ADCL and high ADCH bytes. */
		// printf("Combining low and high bytes.\n");
	    reading = ADCL;
	    reading |= (((uint8_t) ADCH) << 8);
	
		printf("AIN%d=%d ", i, reading);
	}	
	printf("\n");
	return reading;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_read_process, ev, data)
{
	uint16_t reading;

	PROCESS_BEGIN();

	printf("Program begin.\n");

	while(1) {
		reading = ReadLightSensor();
		printf("%d\n", reading);
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
