/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
 * All rights reserved.
 *
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
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include <stm32l1xx_nucleo.h>
#include "sys/node-id.h"

#include "simple-udp.h"
#include "servreg-hack.h"

#include <stdio.h>
#include <string.h>

/** @addtogroup Udp_sender
  * @{
  */

#define UDP_PORT 1234
#define SERVICE_ID 190

#define SEND_INTERVAL		(5 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

static struct simple_udp_connection unicast_connection;
process_event_t sender_event;

extern TIM_HandleTypeDef htim3; 

//extern variable --------------------------------------
//extern  volatile uint8_t ADC_Flag;
/*---------------------------------------------------------------------------*/
PROCESS(unicast_sender_process, "Unicast sender example process");
AUTOSTART_PROCESSES(&unicast_sender_process);
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
 
  //uip_ipaddr_t *addr;
  static char buf[2];
  
 // addr = servreg_hack_lookup(SERVICE_ID); 
  
  printf("Data received %d,%d \n",data[0],data[1]);  
  if(data[0]==51 && data[1]==0)
  {
        buf[0]=51;
        buf[1]=1; //ack to host 
           
       // if(addr!=NULL)
       // {    
             simple_udp_sendto(&unicast_connection, buf,strlen(buf),sender_addr);
             BSP_LED_Off(LED3_ALARM);
             HAL_TIM_PWM_Stop(&htim3,TIM_CHANNEL_4); 
             printf("alarm disabled \n\r");
       //} 
  }
  
  
}
/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;
  int i;
  uint8_t state;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  printf("IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_sender_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t *addr;
  int reg_err;
  char buf[5];
  char msg[20];
 
  
  PROCESS_BEGIN();

  sender_event = process_alloc_event();
  servreg_hack_init();

  set_global_address();
  
  do{
      reg_err=simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);
      HAL_Delay(2000);
      printf("reg err:%d\r\n",reg_err);
  }while(reg_err==0);    
 // etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    
    PROCESS_WAIT_EVENT();
    //PROCESS_WAIT_EVENT_UNTIL(ADC_Flag);
   // PROCESS_PAUSE();
    //HAL_Delay(1000);
    addr = servreg_hack_lookup(SERVICE_ID);
    if(addr==NULL)
    {
         printf("ser look up addr is NULL!!\r\n");
    }      
    if(addr != NULL) {
            
      BSP_LED_Toggle(LED_GREEN);
      buf[0]=0x01; // report type: 0,periodic 1,alarm
      buf[1]=0x04; // device ID:0x04 door detector
      buf[2]=0x03; // device status
      buf[3]=0x40; // battery
      buf[4]=(char)data; 
      //simple_udp_sendto(&unicast_connection, buf, strlen(buf) + 1, addr);
      if(data==1)
      {
        simple_udp_sendto(&unicast_connection, buf, strlen(buf),addr);
        BSP_LED_On(LED3_ALARM);
        HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_4); 
      }
      
      printf("Door state: %d\n", buf[4]);
      
    }
   
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/**
  * @}
  */
