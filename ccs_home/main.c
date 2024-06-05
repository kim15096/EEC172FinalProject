//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     -   SSL Demo
// Application Overview -   This is a sample application demonstrating the
//                          use of secure sockets on a CC3200 device.The
//                          application connects to an AP and
//                          tries to establish a secure connection to the
//                          Google server.
// Application Details  -
// docs\examples\CC32xx_SSL_Demo_Application.pdf
// or
// http://processors.wiki.ti.com/index.php/CC32xx_SSL_Demo_Application
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup ssl
//! @{
//
//*****************************************************************************

// ANDREW KIM & ROCCO SCINTO

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "string.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"
#include "hw_nvic.h"
#include "systick.h"
#include "uart.h"
#include "uart_if.h"
#include "pin_mux_config.h"
#include "hw_uart.h"
#include "simplelink.h"
#include "common.h"
#include "utils/network_utils.h"
#include "gpio_if.h"
#include "time.h"
#include "cJSON.h"


//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                4    /* Current Date */
#define MONTH               6     /* Month 1-12 */
#define YEAR                2024  /* Current year */
#define HOUR                12    /* Time - hours */
#define MINUTE              00    /* Time - minutes */
#define SECOND              0     /* Time - seconds */



#define APPLICATION_NAME "SSL"
#define APPLICATION_VERSION "SQ24"
#define SERVER_NAME "a2hn94z4q1ycvj-ats.iot.us-east-1.amazonaws.com" // CHANGE ME
#define GOOGLE_DST_PORT 8443

#define POSTHEADER "POST /things/andrew_cc3200/shadow HTTP/1.1\r\n" // CHANGE ME
#define GETHEADER "GET /things/andrew_cc3200/shadow HTTP/1.1\r\n"
#define HOSTHEADER "Host: a2hn94z4q1ycvj-ats.iot.us-east-1.amazonaws.com\r\n" // CHANGE ME
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

#define DATA1 "{"                                \
              "\"state\": {\r\n"                 \
              "\"desired\" : {\r\n"              \
              "\"var\" :\""                      \
              "Hello phone, "                    \
              "message from CC3200 via AWS IoT!" \
              "\"\r\n"                           \
              "}"                                \
              "}"                                \
              "}\r\n\r\n"

// Setup for SPI
#define FOREVER 1
#define CONSOLE UARTA0_BASE
#define FAILURE -1
#define SUCCESS 0
#define RETERR_IF_TRUE(condition) \
    {                             \
        if (condition)            \
            return FAILURE;       \
    }
#define RET_IF_ERR(Func)        \
    {                           \
        int iRetVal = (Func);   \
        if (SUCCESS != iRetVal) \
            return iRetVal;     \
    }
#define MASTER_MODE 1
#define SPI_IF_BIT_RATE 100000
#define TR_BUFF_SIZE 100

// Setup for SYSTICK
#define SYSCLKFREQ 80000000ULL
#define SYSTICK_RELOAD_VAL 3200000ULL

#define TICKS_TO_US(ticks)                  \
    ((((ticks) / SYSCLKFREQ) * 1000000UL) + \
     ((((ticks) % SYSCLKFREQ) * 1000000UL) / SYSCLKFREQ)) // macro to convert ticks to microseconds
#define TICKS_TO_MS(ticks)               \
    ((((ticks) / SYSCLKFREQ) * 1000UL) + \
     ((((ticks) % SYSCLKFREQ) * 1000UL) / SYSCLKFREQ)) // macro to convert microseconds to ticks

#define US_TO_TICKS(us) ((SYSCLKFREQ / 1000000ULL) * (us))

#define ONE_SECOND_TICKS (SYSCLKFREQ / 10)

// For periodic get requests
volatile unsigned long lastGetTick = 0;

volatile int systick_cnt = 0;

volatile int last_move = 0; //0,1,2,3 // U,D,L,R

// Global Variables
extern void (*const g_pfnVectors[])(void);

volatile int print_request_flag = 0;
volatile int getStateFlag = 0;

char buffer_string[33]; // Buffer String
int rx_msg_flag = 0;
int buff_idx = 0;

// UARTA1
char uartBuffer[128];
int bufferIndex = 0;

typedef struct PinSetting
{
    unsigned long port;
    unsigned int pin;
} PinSetting;

static int set_time() {
    long retVal;

    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_sec = HOUR;
    g_time.tm_hour = MINUTE;
    g_time.tm_min = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                          SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                          sizeof(SlDateTime),(unsigned char *)(&g_time));

    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

#if defined(ccs) || defined(gcc)
extern void (*const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

//*****************************************************************************
//                 GLOBAL VARIABLES -- End: df
//*****************************************************************************

//****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES
//****************************************************************************
static int set_time();
static void BoardInit(void);
void UART1_Handler(void);
static int http_post(int, char *);
static int http_get(int);


//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static inline void SysTickReset(void)
{
    HWREG(NVIC_ST_CURRENT) = 1;
}

// SysTick Interrupt Handler
static void SysTickHandler(void)
{
    systick_cnt++;
    if (systick_cnt >= 50)
    {
        getStateFlag = 1; // Set flag to trigger GET request
        systick_cnt = 0;  // Update last tick count
    }
}

static void BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

volatile int motion_flag = 0;

#define SERVO_DELAY 20
unsigned char BOTTOM_SERVO = 0x04;
unsigned char TOP_SERVO = 0x8;

unsigned char LASER = 0x1;
// GPIOA2_BASE, 0x1

static const PinSetting switch2 = {.port = GPIOA1_BASE, .pin = 0x80}; // This receives from pin 5

// Define the Servo struct
typedef struct
{
    int currentPosition;
} Servo;

// Define the setter function
void setServoPosition(Servo *servo, int position)
{
    servo->currentPosition = position;
}

// Define the getter function
int getServoPosition(Servo *servo)
{
    return servo->currentPosition;
}

// Define the setter function
void move_servo_right(Servo *servo)
{

    // int x_set[] = {8,6,5,3,1}; //excluding 10% DC because that's just too much movement
    // int y_set[] = {6,5,3,1,1};
    //  5 elements in  these sets

    if (servo->currentPosition == 0)
    {
        servo->currentPosition = 0;
    }
    else
    {
        servo->currentPosition = servo->currentPosition - 1;
    }
}

// Define the getter function
void move_servo_left(Servo *servo)
{
    // int x_set[] = {8,6,5,3,1}; //excluding 10% DC because that's just too much movement
    // int y_set[] = {6,5,3,1,1};
    if (servo->currentPosition == 4)
    {
        servo->currentPosition = 4;
    }
    else
    {
        servo->currentPosition = servo->currentPosition + 1;
    }
}

// IR Interrupt Handler
static void GPIOA2IntHandler(void)
{
    // Setup interrupt
    unsigned long ulStatus = MAP_GPIOIntStatus(switch2.port, true);
    MAP_GPIOIntClear(switch2.port, ulStatus); // clear interrupts on GPIOA2

    //int delay_tuning = 10000000;
    //MAP_UtilsDelay(delay_tuning);
    motion_flag = 1;
}

void delay_mS(int mS)
{
    // Assuming 1 ms delay requires 80000 ticks for a 80 MHz clock
    int ticks_per_mS = 80000;
    int total_ticks = mS * ticks_per_mS;
    MAP_UtilsDelay(total_ticks);
}

void delay_uS(int uS)
{
    // Assuming 1 �s delay requires 80 ticks for a 80 MHz clock
    int ticks_per_uS = 80;
    int total_ticks = uS * ticks_per_uS;
    MAP_UtilsDelay(total_ticks);
}

void delay_mS_us(int mS, int us)
{
    // Assuming 1 ms delay requires 80000 ticks for a 80 MHz clock
    int ticks_per_mS = 80000;
    int total_ticks = mS * ticks_per_mS;
    MAP_UtilsDelay(total_ticks);
    delay_uS(us);
}

void move_servo_top_1_DC()
{
    int i = 0;
    for (i = 0; i < SERVO_DELAY; i++) // 1%
    {
        // 0x8 for other servo
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, TOP_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(35);
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, 0x00); // go low
        delay_mS(3);
        delay_uS(250);
    }
}

void move_servo_top_3_DC()
{
    int i = 0;
    for (i = 0; i < SERVO_DELAY; i++) // 3%
    {
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, TOP_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(100);
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, 0x00); // go low
        delay_mS(3);
        delay_uS(15);
    }
}

void move_servo_top_4_DC()
{
    int i = 0;
    for (i = 0; i < SERVO_DELAY; i++) // 4%
    {
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, TOP_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(130);
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, 0x00); // go low
        delay_mS(3);
        delay_uS(100);
    }
}

void move_servo_top_5_DC()
{
    int i = 0;
    for (i = 0; i < SERVO_DELAY; i++) // 5%
    {
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, TOP_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(160);
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, 0x00); // go low
        delay_mS_us(3, 90);
    }
}

void move_servo_top_6_DC()
{
    int i;
    for (i = 0; i < SERVO_DELAY; i++) // 6%
    {
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, TOP_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(185);
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, 0x00); // go low
        delay_mS_us(3, 65);
    }
}

void move_servo_top_7_DC()
{
    int i;
    for (i = 0; i < SERVO_DELAY; i++) // 7%
    {
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, TOP_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(215);
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, 0x00); // go low
        delay_mS_us(3, 35);
    }
}

void move_servo_top_8_DC()
{
    int i;
    for (i = 0; i < SERVO_DELAY; i++) // 8%
    {
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, TOP_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(245);
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, 0x00); // go low
        delay_mS_us(3, 5);
    }
}

void move_servo_top_9_DC()
{
    int i;
    for (i = 0; i < SERVO_DELAY; i++) // 9%
    {
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, TOP_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(270);
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, 0x00); // go low
        delay_mS_us(2, 980);
    }
}

void move_servo_top_10_DC()
{
    int i = 0;
    for (i = 0; i < SERVO_DELAY; i++) // 10%
    {
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, TOP_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(400);
        GPIOPinWrite(GPIOA1_BASE, TOP_SERVO, 0x00); // go low
        delay_mS(2);
        delay_uS(285);
    }
}

//////////
void move_servo_bottom_1_DC()
{
    int i = 0;
    for (i = 0; i < SERVO_DELAY; i++) // 1%
    {
        // 0x8 for other servo
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, BOTTOM_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(35);
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, 0x00); // go low
        delay_mS(3);
        delay_uS(250);
    }
}

void move_servo_bottom_3_DC()
{
    int i = 0;
    for (i = 0; i < SERVO_DELAY; i++) // 3%
    {
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, BOTTOM_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(100);
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, 0x00); // go low
        delay_mS(3);
        delay_uS(15);
    }
}

void move_servo_bottom_4_DC()
{
    int i = 0;
    for (i = 0; i < SERVO_DELAY; i++) // 4%
    {
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, BOTTOM_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(130);
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, 0x00); // go low
        delay_mS(3);
        delay_uS(100);
    }
}

void move_servo_bottom_5_DC()
{
    int i = 0;
    for (i = 0; i < SERVO_DELAY; i++) // 5%
    {
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, BOTTOM_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(160);
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, 0x00); // go low
        delay_mS_us(3, 90);
    }
}

void move_servo_bottom_6_DC()
{
    int i;
    for (i = 0; i < SERVO_DELAY; i++) // 6%
    {
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, BOTTOM_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(185);
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, 0x00); // go low
        delay_mS_us(3, 65);
    }
}

void move_servo_bottom_7_DC()
{
    int i;
    for (i = 0; i < SERVO_DELAY; i++) // 7%
    {
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, BOTTOM_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(215);
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, 0x00); // go low
        delay_mS_us(3, 35);
    }
}

void move_servo_bottom_8_DC()
{
    int i;
    for (i = 0; i < SERVO_DELAY; i++) // 8%
    {
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, BOTTOM_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(245);
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, 0x00); // go low
        delay_mS_us(3, 5);
    }
}

void move_servo_bottom_9_DC()
{
    int i;
    for (i = 0; i < SERVO_DELAY; i++) // 9%
    {
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, BOTTOM_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(270);
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, 0x00); // go low
        delay_mS_us(2, 980);
    }
}

void move_servo_bottom_10_DC()
{
    int i = 0;
    for (i = 0; i < SERVO_DELAY; i++) // 10%
    {
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, BOTTOM_SERVO); // go high
        // delay_mS(1);  //pulses are from 1-2 mS
        delay_uS(400);
        GPIOPinWrite(GPIOA1_BASE, BOTTOM_SERVO, 0x00); // go low
        delay_mS(2);
        delay_uS(285);
    }
}

void laser_on()
{
    GPIOPinWrite(GPIOA2_BASE, LASER, LASER); // go high
}

void laser_off()
{
    GPIOPinWrite(GPIOA2_BASE, LASER, 0x00); // go low
}
///////////

void laser_point(int x, int y)
{
    switch (x)
    {
    case 1:
        move_servo_bottom_1_DC();
        break;
    case 3:
        move_servo_bottom_3_DC();
        break;
    case 4:
        move_servo_bottom_4_DC();
        break;
    case 5:
        move_servo_bottom_5_DC();
        break;
    case 6:
        move_servo_bottom_6_DC();
        break;
    case 7:
        move_servo_bottom_7_DC();
        break;
    case 8:
        move_servo_bottom_8_DC();
        break;
    case 9:
        move_servo_bottom_9_DC();
        break;
    case 10:
        move_servo_bottom_10_DC();
        break;
    default:
        break;
    }

    switch (y)
    {
    case 1:
        move_servo_top_1_DC();
        break;
    case 3:
        move_servo_top_3_DC();
        break;
    case 4:
        move_servo_top_4_DC();
        break;
    case 5:
        move_servo_top_5_DC();
        break;
    case 6:
        move_servo_top_6_DC();
        break;
    case 7:
        move_servo_top_7_DC();
        break;
    case 8:
        move_servo_top_8_DC();
        break;
    case 9:
        move_servo_top_9_DC();
        break;
    case 10:
        move_servo_top_10_DC();
        break;
    default:
        break;
    }
}

void laser_mode_3()
{
    int delay_2 = 20;
    //laser_on();
    laser_point(1, 1);
    delay_mS(delay_2);
    laser_point(3, 2);
    delay_mS(delay_2);
    laser_point(4, 3);
    delay_mS(delay_2);
    laser_point(3, 2);
    delay_mS(delay_2);
    laser_point(2, 1);
    laser_point(1, 1);
    delay_mS(delay_2);
    laser_point(3, 2);
    delay_mS(delay_2);
    laser_point(4, 3);
    delay_mS(delay_2);
    laser_point(3, 2);
    delay_mS(delay_2);
    laser_point(2, 1);

    laser_point(3, 1);
    delay_mS(delay_2);
    laser_point(3, 1);
    delay_mS(delay_2);
    laser_point(4, 2);
    delay_mS(delay_2);
    laser_point(3, 2);
    delay_mS(delay_2);
    laser_point(2, 1);
    laser_point(3, 1);
    delay_mS(delay_2);
    laser_point(3, 4);
    delay_mS(delay_2);
    laser_point(4, 3);
    delay_mS(delay_2);
    laser_point(3, 2);
    delay_mS(delay_2);
    laser_point(2, 1);
    //laser_off();
}


void laser_mode_4()
{
    int delay_2 = 20;

    laser_point(3, 4);
    delay_mS(delay_2);
    laser_point(4, 3);
    delay_mS(delay_2);
    laser_point(3, 4);
    delay_mS(delay_2);
    laser_point(4, 3);
    delay_mS(delay_2);
    laser_point(3, 4);
    delay_mS(delay_2);
    laser_point(4, 3);
    delay_mS(delay_2);
    laser_point(3, 4);
    delay_mS(delay_2);
    laser_point(3, 4);
    delay_mS(delay_2);
    laser_point(4, 3);
    delay_mS(delay_2);
    laser_point(3, 4);
    delay_mS(delay_2);
    laser_point(4, 3);
    delay_mS(delay_2);
    laser_point(3, 4);
    //laser_off();
}



//*****************************************************************************
//
//! This function updates the date and time of CC3200.
//!
//! \param None
//!
//! \return
//!     0 for success, negative otherwise
//!
//*****************************************************************************



/**
 * Initializes SysTick Module
 */
static void SysTickInit(void)
{
    MAP_SysTickPeriodSet(SYSTICK_RELOAD_VAL);
    MAP_SysTickIntRegister(SysTickHandler);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();
}

//*****************************************************************************
//
//! Main
//!
//! \param  none
//!
//! \return None
//!
//*****************************************************************************

Servo top_servo;
Servo bottom_servo;


void main()
{
    long lRetVal = -1;

    unsigned long ulStatus;

    //
    // Initialize board configuration
    //
    BoardInit();

    PinMuxConfig();

    // Enable SysTick
    SysTickInit();
    SysTickPeriodSet(SYSTICK_RELOAD_VAL);
    SysTickEnable();
    SysTickIntEnable();
    SysTickReset();

    InitTerm();
    ClearTerm();
    IntMasterEnable();

    MAP_GPIOIntRegister(switch2.port, GPIOA2IntHandler);              // interrupt handler
    MAP_GPIOIntTypeSet(switch2.port, switch2.pin, GPIO_FALLING_EDGE); // falling edge
    ulStatus = MAP_GPIOIntStatus(switch2.port, false);
    MAP_GPIOIntClear(switch2.port, ulStatus);     // clear interrupts on GPIOA2
    MAP_GPIOIntEnable(switch2.port, switch2.pin); // enable interrupts



    top_servo.currentPosition = 2;
    bottom_servo.currentPosition = 2;
    laser_point(bottom_servo.currentPosition, top_servo.currentPosition);
    laser_off();


    // initialize global default app configuration
    g_app_config.host = SERVER_NAME;
    g_app_config.port = GOOGLE_DST_PORT;

    // Connect the CC3200 to the local access point
    lRetVal = connectToAccessPoint();

    // Set time so that encryption can be used
    lRetVal = set_time();

    if (lRetVal < 0)
    {
        UART_PRINT("Unable to set time in the device");
        LOOP_FOREVER();
    }
    // Connect to the website with TLS encryption
    lRetVal = tls_connect();

    if (lRetVal < 0)
    {
        ERR_PRINT(lRetVal);
    }

    http_post(lRetVal, "TRUE");
    // Main while loop

    while (1)
    {

        // laser_mode_3();

        if (getStateFlag)
        {
            // we read in the data
            http_get(lRetVal);

            getStateFlag = 0;
        }

        if (motion_flag)
        {
            // we read in the data
            http_post(lRetVal, "TRUE");
            delay_mS(1);
            motion_flag = 0;
        }
    }
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

static int http_post(int iTLSSockID, char *state)
{
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char *pcBufHeaders;
    int lRetVal = 0;

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    char formatted_msg[512];

    sprintf(formatted_msg,
            "{\r\n"
            "    \"state\": {\r\n"
            "        \"desired\": {\r\n"
            "            \"motion_detected\": \"%s\",\r\n"
            "            \"motion_detected_msg\": \"Your cat wants to play! Go to eec172-lazercat.vercel.app to check it out!\"\r\n"
            "        }\r\n"
            "    }\r\n"
            "}\r\n\r\n",
            state);

    int dataLength = strlen(formatted_msg);

    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);

    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);

    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);

    strcpy(pcBufHeaders, formatted_msg);
    pcBufHeaders += strlen(formatted_msg);

    int testDataLength = strlen(pcBufHeaders);

    UART_PRINT(acSendBuff);

    //
    // Send the packet to the server */
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if (lRetVal < 0)
    {
        UART_PRINT("POST failed. Error Number: %i\n\r", lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if (lRetVal < 0)
    {
        UART_PRINT("Received failed. Error Number: %i\n\r", lRetVal);
        // sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    else
    {
        acRecvbuff[lRetVal + 1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}

static int http_get(int iTLSSockID)
{
    char acSendBuff[512];
    char acRecvbuff[1460];
    char *pcBufHeaders;
    int lRetVal = 0;

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, GETHEADER);
    pcBufHeaders += strlen(GETHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, "\r\n\r\n");

    UART_PRINT(acSendBuff);

    //
    // Send the packet to the server
    //
    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if (lRetVal < 0)
    {
        UART_PRINT("GET failed. Error Number: %i\n\r", lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if (lRetVal < 0)
    {
        UART_PRINT("Received failed. Error Number: %i\n\r", lRetVal);
        // sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    else
    {
        acRecvbuff[lRetVal] = '\0';

        // Find start of json format
        const char *json_start = strchr(acRecvbuff, '{');
        if (!json_start)
        {
            printf("JSON part not found.\n");
            return 1;
        }

        // Parse the JSON part
        cJSON *json = cJSON_Parse(json_start);
        if (!json)
        {
            printf("Error parsing JSON.\n");
            return 1;
        }

        // Extract the desired variables
        cJSON *state = cJSON_GetObjectItem(json, "state");
        if (state)
        {
            cJSON *desired = cJSON_GetObjectItem(state, "desired");
            if (desired)
            {
                cJSON *home_cc_state = cJSON_GetObjectItem(desired, "home_cc_state");
                cJSON *home_cc_input = cJSON_GetObjectItem(desired, "home_cc_input");
                // here
                if (home_cc_state && cJSON_IsString(home_cc_state))
                {
                    printf("home_cc_state: %s\n", home_cc_state->valuestring);
                    if (strcmp(home_cc_state->valuestring, "IDLE") == 0)
                    {
                        laser_off();
                    }
                }
                if (home_cc_input && cJSON_IsString(home_cc_input))
                {
                    printf("home_cc_input: %s\n", home_cc_input->valuestring);
                    // IF RUNNING PRESET WAS BOX
                    if (strcmp(home_cc_input->valuestring, "BOX") == 0)
                    {
                        Report("RUNNING BOX MODE");
                        laser_on();
                        laser_mode_3();
                    }
                    // IF RUNNING PRESET WAS ZIGZAG
                    else if (strcmp(home_cc_input->valuestring, "ZIGZAG") == 0)
                    {
                        laser_on();
                        laser_mode_4();
                    }

                    else if (strcmp(home_cc_input->valuestring, "UP") == 0)
                    {
                        if(last_move != 0)
                        {
                            move_servo_left(&top_servo);
                            laser_point(bottom_servo.currentPosition,top_servo.currentPosition);
                            last_move = 0; //0,1,2,3 // U,D,L,R
                        }
                    }
                    else if (strcmp(home_cc_input->valuestring, "DOWN") == 0)
                    {
                        if(last_move != 1)
                        {
                            move_servo_right(&top_servo);
                            laser_point(bottom_servo.currentPosition,top_servo.currentPosition);
                            last_move = 1; //0,1,2,3 // U,D,L,R
                        }
                    }
                    else if (strcmp(home_cc_input->valuestring, "LEFT") == 0)
                    {
                        if(last_move != 2)
                        {
                            move_servo_left(&bottom_servo);
                            laser_point(bottom_servo.currentPosition,top_servo.currentPosition);
                            last_move = 2; //0,1,2,3 // U,D,L,R
                        }
                    }
                    else if (strcmp(home_cc_input->valuestring, "RIGHT") == 0)
                    {
                        if(last_move != 3)
                        {
                            move_servo_right(&bottom_servo);
                            laser_point(bottom_servo.currentPosition,top_servo.currentPosition);
                            last_move = 3; //0,1,2,3 // U,D,L,R
                        }
                    }

                }
            }
        }

        // make if statements

        // Clean up
        cJSON_Delete(json);
    }

    return 0;
}
