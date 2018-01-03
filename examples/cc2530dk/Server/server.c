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

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"
#include "dev/watchdog.h"
#include "dev/leds.h"
#include "net/rpl/rpl.h"
#include "dev/button-sensor.h"
#include "debug.h"

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len])

#define MAX_PAYLOAD_LEN 120

static struct uip_udp_conn *server_conn;
static char buf[MAX_PAYLOAD_LEN];
static uint16_t len;

#define SERVER_REPLY          1

/* Should we act as RPL root? */
#define SERVER_RPL_ROOT       1

#if SERVER_RPL_ROOT
static uip_ipaddr_t ipaddr;
#endif
/*---------------------------------------------------------------------------*/
PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
/* This function is run every time the device receives something. */
/* Its purpose is to output information and respond to the message. */
static void
tcpip_handler(void)
{
  /* Clear out a space in memory for a new message */
  memset(buf, 0, MAX_PAYLOAD_LEN);
  
  /* Check if data has been received */
  if(uip_newdata()) {
	  
    /* Turn on red LED (yellow on CC2531) */
    leds_on(LEDS_RED);
	
	/* Set buffer to whatever was received (echo the message back) */
    len = uip_datalen();
    memcpy(buf, uip_appdata, len);
	
	/* Print information about the packet received */
    PRINTF("%u bytes from [", len);
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    PRINTF("]:%u\n", UIP_HTONS(UIP_UDP_BUF->srcport));
	
#if SERVER_REPLY
	/* Connect to the client */
    uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
    server_conn->rport = UIP_UDP_BUF->srcport;

    /* Add "Server replied!" to the end of the message */
    strcat(buf,"-Server replied!\n");
    uip_udp_packet_send(server_conn, buf, len+sizeof("-Server replied!\n"));
	
    /* Restore server connection to allow data from any node */
    uip_create_unspecified(&server_conn->ripaddr);
    server_conn->rport = 0;
#endif
  }
  
  /* Turn off the red LED (yellow on CC2531) */
  leds_off(LEDS_RED);
  
  return;
}
/*---------------------------------------------------------------------------*/
/* If the button is held down during startup, print debugging stats */
#if (BUTTON_SENSOR_ON && (DEBUG==DEBUG_PRINT))
static void
print_stats()
{
  PRINTF("tl=%lu, ts=%lu, bs=%lu, bc=%lu\n",
         rimestats.toolong, rimestats.tooshort, rimestats.badsynch,
         rimestats.badcrc);
  PRINTF("llrx=%lu, lltx=%lu, rx=%lu, tx=%lu\n", rimestats.llrx,
         rimestats.lltx, rimestats.rx, rimestats.tx);
}
#endif
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
/* Create a DAG (Directed Acyclic Graph), basically create the global network */
#if SERVER_RPL_ROOT
void
create_dag()
{
  rpl_dag_t *dag;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  /* List all the sever addresses */
  print_local_addresses();

  /* Create the DAG */
  dag = rpl_set_root(RPL_DEFAULT_INSTANCE,
                     &uip_ds6_get_global(ADDR_PREFERRED)->ipaddr);
  /* Do this if the DAG was created successfully */
  if(dag != NULL) {
    /* Set the IPv6 address prefix */
    uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &ipaddr, 64);
	/* Print stuff */
    PRINTF("Created a new RPL dag with ID: ");
    PRINT6ADDR(&dag->dag_id);
    PRINTF("\n");
  }
}
#endif /* SERVER_RPL_ROOT */
/*---------------------------------------------------------------------------*/
/* The main process */
PROCESS_THREAD(udp_server_process, ev, data)
{

  PROCESS_BEGIN();
  putstring("Starting UDP server\n");

#if BUTTON_SENSOR_ON
  putstring("Button 1: Print RIME stats\n");
#endif

#if SERVER_RPL_ROOT
  create_dag();
#endif

  /* Start the server */
  server_conn = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(server_conn, UIP_HTONS(3000));

  PRINTF("Listen port: 3000, TTL=%u\n", server_conn->ttl);

  while(1) {
    PROCESS_YIELD();
	/* Run tcpip_hander() if a packet is received */
    if(ev == tcpip_event) {
      tcpip_handler();
#if (BUTTON_SENSOR_ON && (DEBUG==DEBUG_PRINT))
    } else if(ev == sensors_event && data == &button_sensor) {
      print_stats();
#endif /* BUTTON_SENSOR_ON */
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
