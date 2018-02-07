#include "contiki.h"
#include "cc253x.h"
#include "contiki-conf.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>
#include <stdio.h>

// #define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"
#include "dev/watchdog.h"
#include "dev/leds.h"
#include "net/rpl/rpl.h"
#include "dev/button-sensor.h"
#include "debug.h"

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len])

#define MAX_PAYLOAD_LEN 1024

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

static struct uip_udp_conn *server_conn;
static char buf[MAX_PAYLOAD_LEN];
static char received_data[MAX_PAYLOAD_LEN];
static uint16_t reading;

/*---------------------------------------------------------------------------*/
PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
/* This function is run every time the device receives something. */
/* Its purpose is to output information and respond to the message. */
static void
tcpip_handler(void)
{
  if(uip_newdata()) {
    leds_on(LEDS_RED);  /* Yellow on CC2531 */
	
	ReadLightSensor();
	
	memcpy(received_data, uip_appdata, uip_datalen());
	printf("Received data: %s\n", received_data);
	
	/* Connect to the client */
    uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
    server_conn->rport = UIP_UDP_BUF->srcport;
	/* Send the value */
    uip_udp_packet_send(server_conn, &reading, sizeof(reading));
	printf("Sent message: %d (%x)\n", reading, reading);
    /* Restore server connection to allow data from any node */
    uip_create_unspecified(&server_conn->ripaddr);
    server_conn->rport = 0;
	
	memset(received_data, 0, MAX_PAYLOAD_LEN);
    leds_off(LEDS_RED);
  }
  return;
}
/*---------------------------------------------------------------------------*/
/* Function to read photoresistor */
static struct etimer et;
static char ReadLightSensor()
{
	uint8_t command;

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
	
	return reading;
}
/*---------------------------------------------------------------------------*/
/* This function is run once when the network is created. */
/* Its purpose is to list all the IP addresses that the server owns. */
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused && (state == ADDR_TENTATIVE || state
        == ADDR_PREFERRED)) {
      PRINTF("  ");
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
      if(state == ADDR_TENTATIVE) {
        uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
/* The main process */
PROCESS_THREAD(udp_server_process, ev, data)
{

  PROCESS_BEGIN();
  putstring("Starting UDP server\n");

  /* Start the server */
  server_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(server_conn, UIP_HTONS(3000));

  PRINTF("Listen port: 3000, TTL=%u\n", server_conn->ttl);

  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
