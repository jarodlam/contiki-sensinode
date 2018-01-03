/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include "debug.h"

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#define SEND_INTERVAL		2 * CLOCK_SECOND
#define MAX_PAYLOAD_LEN		40

static char buf[MAX_PAYLOAD_LEN];

/* Our destinations and udp conns. One link-local and one global */
#define LOCAL_CONN_PORT 3001
static struct uip_udp_conn *l_conn;
#if UIP_CONF_ROUTER
#define GLOBAL_CONN_PORT 3002
static struct uip_udp_conn *g_conn;
#endif

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
#if BUTTON_SENSOR_ON
PROCESS_NAME(ping6_process);
AUTOSTART_PROCESSES(&udp_client_process, &ping6_process);
#else
AUTOSTART_PROCESSES(&udp_client_process);
#endif
/*---------------------------------------------------------------------------*/
/* This function is run every time the device receives something. */
/* Its purpose is to output information about the bytes it received. */
static void
tcpip_handler(void)
{
  /* Turn on the green LED */
  leds_on(LEDS_GREEN);
  if(uip_newdata()) {
    /* Output information about the data that was received */
    /* Example output: "0x02 bytes response=0x004c" */
    putstring("0x");
    puthex(uip_datalen());
    putstring(" bytes response=0x");
    puthex((*(uint16_t *) uip_appdata) >> 8);
    puthex((*(uint16_t *) uip_appdata) & 0xFF);
    putchar('\n');
  }
  leds_off(LEDS_GREEN);
  return;
}
/*---------------------------------------------------------------------------*/
/* This function is run at set intervals (default every 2 secionds). */
/* Its purpose is to send a message to the server. */
static void
timeout_handler(void)
{
  /* Create some variables */
  static int seq_id;
  struct uip_udp_conn *this_conn;

  /* Turn on the red LED */
  leds_on(LEDS_RED);
  /* Clear out a space in memory for the message */
  memset(buf, 0, MAX_PAYLOAD_LEN);
  /* Increment the sequence ID, a counter for how many messages have been sent */
  seq_id++;

  /* If the sequence ID is odd, make a local connection */
  if(seq_id & 0x01) {
    this_conn = l_conn;
  /* If the sequence ID is even, make a global connection */
  } else {
    this_conn = g_conn;
    if(uip_ds6_get_global(ADDR_PREFERRED) == NULL) {
      return;
    }
  }

  /* Print the address the message is being sent to */
  PRINTF("Client to: ");
  PRINT6ADDR(&this_conn->ripaddr);

  /* Set the variable "buf" to the message that will be sent */
  memcpy(buf, &seq_id, sizeof(seq_id));

  /* Print information about the message being sent */
  PRINTF(" Remote Port %u,", UIP_HTONS(this_conn->rport));
  PRINTF(" (msg=0x%04x), %u bytes\n", *(uint16_t *) buf, sizeof(seq_id));

  /* Send the message!! */
  uip_udp_packet_send(this_conn, buf, sizeof(seq_id));
  
  /* Turn off the red LED after finished sending the packet */
  leds_off(LEDS_RED);
}
/*---------------------------------------------------------------------------*/
/* This function is run at startup. */
/* It is the main process that makes everything else run. */
PROCESS_THREAD(udp_client_process, ev, data)
{
  /* Make variables for the timer and IP address */
  static struct etimer et;
  uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();
  PRINTF("UDP client process started\n");

  /* This is the LOCAL IP address of the SERVER. */
  uip_ip6addr(&ipaddr, 0xfe80, 0, 0, 0, 0x0212, 0x4b00, 0x0154, 0x5b98);
  /* Make a local connection with the server. */
  l_conn = udp_new(&ipaddr, UIP_HTONS(3000), NULL);
  if(!l_conn) {
    PRINTF("udp_new l_conn error.\n");
  }
  udp_bind(l_conn, UIP_HTONS(LOCAL_CONN_PORT));

  /* Print information about the local connection. */
  PRINTF("Link-Local connection with ");
  PRINT6ADDR(&l_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
         UIP_HTONS(l_conn->lport), UIP_HTONS(l_conn->rport));

  /* This is the GLOBAL IP address of the SERVER. */
  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0x0212, 0x4b00, 0x0154, 0x5b98);
  /* Make a global connection with the server. */
  g_conn = udp_new(&ipaddr, UIP_HTONS(3000), NULL);
  if(!g_conn) {
    PRINTF("udp_new g_conn error.\n");
  }
  udp_bind(g_conn, UIP_HTONS(GLOBAL_CONN_PORT));

  /* Print information about the global connection. */
  PRINTF("Global connection with ");
  PRINT6ADDR(&g_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
         UIP_HTONS(g_conn->lport), UIP_HTONS(g_conn->rport));

  /* Set the timer to the SEND_INTERVAL, default 2 seconds. */
  etimer_set(&et, SEND_INTERVAL);

  /* Loop forever */
  while(1) {
    PROCESS_YIELD();
	/* Run timeout_handler() if the timer has expired */
    if(etimer_expired(&et)) {
      timeout_handler();
      etimer_restart(&et);
    /* Run tcpip_handler() if a packet has been received */
    } else if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
