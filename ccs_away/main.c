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

// EEC 172 Final Project; Andrew Kim & Rocco Scinto

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
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "spi.h"
#include "oled_test.h"
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
#include "cJSON.h"

#define APPLICATION_NAME      "SSL"
#define APPLICATION_VERSION   "SQ24"
#define SERVER_NAME           "a2hn94z4q1ycvj-ats.iot.us-east-1.amazonaws.com" // CHANGE ME
#define GOOGLE_DST_PORT       8443


#define POSTHEADER "POST /things/andrew_cc3200/shadow HTTP/1.1\r\n"             // CHANGE ME
#define GETHEADER "GET /things/andrew_cc3200/shadow HTTP/1.1\r\n"
#define HOSTHEADER "Host: a2hn94z4q1ycvj-ats.iot.us-east-1.amazonaws.com\r\n"  // CHANGE ME
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json; charset=utf-8\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

#define DATA1 "{" \
            "\"state\": {\r\n"                                              \
                "\"desired\" : {\r\n"                                       \
                    "\"var\" :\""                                           \
                        "Hello phone, "                                     \
                        "message from CC3200 via AWS IoT!"                  \
                        "\"\r\n"                                            \
                "}"                                                         \
            "}"                                                             \
        "}\r\n\r\n"

#define UPARROW "^"
#define DOWNARROW "v"
#define LEFTARROW "<"
#define RIGHTARROW ">"

//NEED TO UPDATE THIS FOR IT TO WORK!
#define DATE                4    /* Current Date */
#define MONTH               6     /* Month 1-12 */
#define YEAR                2024  /* Current year */
#define HOUR                10    /* Time - hours */
#define MINUTE              7    /* Time - minutes */
#define SECOND              49     /* Time - seconds */

// Setup for SPI
#define FOREVER                 1
#define CONSOLE                 UARTA0_BASE
#define FAILURE                 -1
#define SUCCESS                 0
#define RETERR_IF_TRUE(condition) {if(condition) return FAILURE;}
#define RET_IF_ERR(Func)          {int iRetVal = (Func); \
                                   if (SUCCESS != iRetVal) \
                                     return  iRetVal;}
#define MASTER_MODE 1
#define SPI_IF_BIT_RATE 100000
#define TR_BUFF_SIZE 100

// Setup for SYSTICK
#define SYSCLKFREQ 80000000ULL
#define SYSTICK_RELOAD_VAL 3200000ULL

#define TICKS_TO_US(ticks) \
    ((((ticks) / SYSCLKFREQ) * 1000000UL) + \
    ((((ticks) % SYSCLKFREQ) * 1000000UL) / SYSCLKFREQ))\
// macro to convert ticks to microseconds
#define TICKS_TO_MS(ticks) \
    ((((ticks) / SYSCLKFREQ) * 1000UL) + \
    ((((ticks) % SYSCLKFREQ) * 1000UL) / SYSCLKFREQ))\
// macro to convert microseconds to ticks

#define US_TO_TICKS(us) ((SYSCLKFREQ / 1000000ULL) * (us))

#define ONE_SECOND_TICKS (SYSCLKFREQ / 10)

// For periodic get requests
volatile unsigned long lastGetTick = 0;

volatile int systick_cnt = 0;

// Global Variables
extern void (* const g_pfnVectors[])(void);

volatile int print_request_flag = 0;
volatile int getStateFlag = 0;

char buffer_string[33]; // Buffer String
int rx_msg_flag = 0;
int buff_idx = 0;

// UARTA1
char uartBuffer[128];
int bufferIndex = 0;

typedef struct PinSetting {
    unsigned long port;
    unsigned int pin;
} PinSetting;


// Set IR_PIN as IR out
static const PinSetting IR_PIN = { .port = GPIOA0_BASE, .pin = 0x40};

typedef struct {
    unsigned char prevPressed;  // Previously pressed button
    const char* prevPressedName;
    unsigned int type;          // Type of button pressed (0 - Arrow, 1 - Presets, 2 - Reset Menu)
} ButtonFunc;

ButtonFunc buttonFunc = {'\0', "\0", 2};
//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
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
static int http_post(int, char*, char*);
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
static inline void SysTickReset(void) {
    HWREG(NVIC_ST_CURRENT) = 1;
}

// IR Interrupt Handler
static void IR_InterruptHandler(void) {
    // Setup interrupt
    unsigned long ulStatus = MAP_GPIOIntStatus(IR_PIN.port, true);
    MAP_GPIOIntClear(IR_PIN.port, ulStatus);        // clear interrupts on GPIOA2

    // Get delta time interval in us
    unsigned long int currentTime_ticks = SYSTICK_RELOAD_VAL - SysTickValueGet();
    unsigned long int currentTime = TICKS_TO_US(currentTime_ticks);

    // Identify Start Bit
    if (currentTime > 12000 && currentTime < 14000){
        rx_msg_flag = 1;
        buff_idx = 0; // Reset buffer index when a new message starts
        memset(buffer_string, 0, sizeof(buffer_string));

    }
    // Receive bits after start bit
    if (rx_msg_flag) {
            if (currentTime > 1500 && currentTime < 3000) {
                buffer_string[buff_idx++] = '1';
            } else if (currentTime > 1000 && currentTime <= 1500) {
                buffer_string[buff_idx++] = '0';
            }

            if (buff_idx == 32) { // Only set print request if we have exactly 32 bits
                print_request_flag = 1;
            }
        }
    // Reset Systick
    SysTickReset();
}

// SysTick Interrupt Handler
static void SysTickHandler(void) {
    systick_cnt++;
    if (systick_cnt >= 300) {
        getStateFlag = 1; // Set flag to trigger GET request
        systick_cnt = 0; // Update last tick count
    }
}

static void BoardInit(void) {
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

/**
 * Initializes SysTick Module
 */
static void SysTickInit(void) {
    MAP_SysTickPeriodSet(SYSTICK_RELOAD_VAL);
    MAP_SysTickIntRegister(SysTickHandler);
    MAP_SysTickIntEnable();
    MAP_SysTickEnable();
}

// Return's the button pressed given binary buffer
char GetButtonPressed(char* buffer_string){
    char returnChar;

    if (strcmp(buffer_string, "01100000110111111100100000110111") == 0) {
        return '1';
    } else if (strcmp(buffer_string, "01100000110111110000100011110111") == 0) {
        returnChar = '2';
    } else if (strcmp(buffer_string, "01100000110111111000100001110111") == 0) {
        returnChar = '3';
    } else if (strcmp(buffer_string, "01100000110111111110000000011111") == 0) {
        returnChar = 'X';
    } else if (strcmp(buffer_string, "01100000110111110011101011000101") == 0) {
        returnChar = 'O';
    } else if (strcmp(buffer_string, "01100000110111110010001011011101") == 0) {
        returnChar = 'U';
    } else if (strcmp(buffer_string, "01100000110111110001001011101101") == 0) {
        returnChar = 'R';
    } else if (strcmp(buffer_string, "01100000110111111011100001000111") == 0) {
        returnChar = 'D';
    } else if (strcmp(buffer_string, "01100000110111110011100011000111") == 0) {
        returnChar = 'L';
    }
    // Return * for any other buttons
    else return '*';

    return returnChar;
}

void InitialBottomDisplay(){
    fillRect(0, 80, 128, 53, WHITE);
    drawString(45, 80, "Start", GREEN, 1, 7);
    drawString(15, 95, "Arrow - Manual", BLACK, 1, 7);
    drawString(15, 110, "Number - Auto", BLACK, 1, 7);
    buttonFunc.type = 2;
}

void RunningPresetDisplay(char* preset){
    fillRect(0, 80, 128, 53, WHITE);
    drawString(40, 80, "Running", GREEN, 1, 7);
    drawString(35, 95, preset, BLACK, 2, 11);
    drawString(50, 118, "STOP (Last)", RED, 1, 7);
    buttonFunc.type = 1;
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
void main() {
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

    // Setup SPI
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralReset(PRCM_GSPI);
    MAP_SPIReset(GSPI_BASE);
    MAP_SPIConfigSetExpClk(GSPI_BASE, MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                             SPI_IF_BIT_RATE, SPI_MODE_MASTER, SPI_SUB_MODE_0,
                             (SPI_SW_CTRL_CS |
                              SPI_4PIN_MODE |
                              SPI_TURBO_OFF |
                              SPI_CS_ACTIVEHIGH |
                              SPI_WL_8));
    MAP_SPIEnable(GSPI_BASE);

    // Setup interrupts
    MAP_GPIOIntRegister(IR_PIN.port, IR_InterruptHandler); // interrupt handler
    MAP_GPIOIntTypeSet(IR_PIN.port, IR_PIN.pin, GPIO_FALLING_EDGE); // falling edge
    ulStatus = MAP_GPIOIntStatus(IR_PIN.port, false);
    MAP_GPIOIntClear(IR_PIN.port, ulStatus);           // clear interrupts on GPIOA2
    MAP_GPIOIntEnable(IR_PIN.port, IR_PIN.pin);       // enable interrupts

    UART_PRINT("My terminal works!\n\r");

    // initialize global default app configuration
    g_app_config.host = SERVER_NAME;
    g_app_config.port = GOOGLE_DST_PORT;

    //Connect the CC3200 to the local access point
    lRetVal = connectToAccessPoint();
    //Set time so that encryption can be used
    lRetVal = set_time();
    if(lRetVal < 0) {
        UART_PRINT("Unable to set time in the device");
        LOOP_FOREVER();
    }
    //Connect to the website with TLS encryption
    lRetVal = tls_connect();
    if(lRetVal < 0) {
        ERR_PRINT(lRetVal);
    }

    // Initialize Adafruit
    Adafruit_Init();

    // Setup OLED screen graphics
    fillScreen(WHITE);

    char* display_name = "LazerCat";
    char* cat_ascii_1 =" /\\_/\\";
    char* cat_ascii_2 ="( o.o )";
    char* cat_ascii_3 ="   ^";


    // Display init setup for OLED
    drawString(38, 60, display_name, MAGENTA, 1, 7);
    drawString(28, 5, cat_ascii_1, BLACK, 2, 10);
    drawString(28, 25, cat_ascii_2, BLACK, 2, 10);
    drawString(28, 45, cat_ascii_3, BLACK, 2, 10);
    drawFastHLine(0, 75, 128, BLACK);

    InitialBottomDisplay();

    // Main while loop
    while (1) {

        // Fired every button pressed
        if (print_request_flag) {

            // Get pressed button
            Report("%s\t Pressed button:", buffer_string);
            char buttonPressed = GetButtonPressed(buffer_string);
            Report("%c\n", buttonPressed);

            // Clear bottom canvas for preset display
            if (buttonPressed == '1' || buttonPressed == '2' || buttonPressed == '3'){
                fillRect(0, 80, 128, 53, WHITE);
            }

            // Clear for activating lazer mode
            if ((buttonFunc.type != 0) && (buttonPressed == 'L' || buttonPressed == 'R' || buttonPressed == 'U' || buttonPressed == 'D')){
                fillRect(0, 80, 128, 53, WHITE);
            }

            // Check for button presses
            if (buttonPressed != '*'){
                if (buttonPressed == 'L'){
                    buttonFunc.prevPressed = 'L';
                    buttonFunc.type = 0;
                    drawString(56, 92, LEFTARROW, BLACK, 3, 10);
                    http_post(lRetVal, "MANUAL", "LEFT");
                }
                else if (buttonPressed == 'R'){
                    buttonFunc.prevPressed = 'R';
                    buttonFunc.type = 0;
                    drawString(56, 92, RIGHTARROW, BLACK, 3, 10);
                    http_post(lRetVal, "MANUAL", "RIGHT");
                }
                else if (buttonPressed == 'U'){
                    buttonFunc.prevPressed = 'U';
                    buttonFunc.type = 0;
                    drawString(56, 92, UPARROW, BLACK, 3, 10);
                    http_post(lRetVal, "MANUAL", "UP");
                }
                else if (buttonPressed == 'D'){
                    buttonFunc.prevPressed = 'D';
                    buttonFunc.type = 0;
                    drawString(56, 92, DOWNARROW, BLACK, 3, 10);
                    http_post(lRetVal, "MANUAL", "DOWN");
                }
                else if (buttonPressed == '1'){
                    buttonFunc.prevPressed = '1';
                    buttonFunc.prevPressedName = "BOX";
                    buttonFunc.type = 1;
                    drawString(15, 85, "1. Box", BLACK, 2, 11);
                    drawString(5, 115, "X(Last)", RED, 1, 7);
                    drawString(108, 115, "OK", GREEN, 1, 7);
                    systick_cnt = 0;

                }
                else if (buttonPressed == '2'){
                    buttonFunc.prevPressed = '2';
                    buttonFunc.type = 1;
                    buttonFunc.prevPressedName = "ZIGZAG";
                    drawString(15, 85, "2. Zigzag", BLACK, 2, 11);
                    drawString(5, 115, "X(Last)", RED, 1, 7);
                    drawString(108, 115, "OK", GREEN, 1, 7);
                    systick_cnt = 0;
                }
                else if (buttonPressed == '3'){
                    buttonFunc.prevPressed = '3';
                    buttonFunc.type = 1;
                    buttonFunc.prevPressedName = "LINE";
                    drawString(15, 85, "3. Line", BLACK, 2, 11);
                    drawString(5, 115, "X(Last)", RED, 1, 7);
                    drawString(108, 115, "OK", GREEN, 1, 7);
                    systick_cnt = 0;
                }
                // OK BUTTON
                else if (buttonPressed == 'O'){
                    if (buttonFunc.prevPressed == 'L' || buttonFunc.prevPressed == 'R' || buttonFunc.prevPressed == 'U' || buttonFunc.prevPressed == 'D'){
                        // Ignore
                        ;
                    }
                    else {
                        char* preset_input = malloc(strlen(buttonFunc.prevPressedName) + 1);
                        strcpy(preset_input, buttonFunc.prevPressedName);
                        http_post(lRetVal, "AUTO", preset_input);
                        RunningPresetDisplay(preset_input);
                        buttonFunc.type = 1;
                        free(preset_input);
                    }
                }
                // Cancel Button Pressed
                else if (buttonPressed == 'X'){
                    http_post(lRetVal, "IDLE", "NONE");
                    InitialBottomDisplay();
                    buttonFunc.type = 2;
                }
            }
            // Reset for next set of interrupts
            print_request_flag = 0;
            buff_idx = 0;
            rx_msg_flag = 0;
            memset(buffer_string, 0, sizeof(buffer_string));
        }

        if (getStateFlag){
            http_get(lRetVal);
            getStateFlag = 0;
        }

    }
}
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

static int http_post(int iTLSSockID, char* state, char* input){
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
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
            "            \"home_cc_state\": \"%s\",\r\n"
            "            \"home_cc_input\": \"%s\"\r\n"
            "        }\r\n"
            "    }\r\n"
            "}\r\n\r\n",
            state, input);

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
    if(lRetVal < 0) {
        UART_PRINT("POST failed. Error Number: %i\n\r",lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r",lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
           return lRetVal;
    }
    else {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}

static int http_get(int iTLSSockID) {
    char acSendBuff[512];
    char acRecvbuff[1460];
    char* pcBufHeaders;
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
    if (lRetVal < 0) {
        UART_PRINT("GET failed. Error Number: %i\n\r", lRetVal);
        sl_Close(iTLSSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if (lRetVal < 0) {
        UART_PRINT("Received failed. Error Number: %i\n\r", lRetVal);
        //sl_Close(iSSLSockID);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        return lRetVal;
    }
    else {
        acRecvbuff[lRetVal] = '\0';

        // Find start of json format
        const char *json_start = strchr(acRecvbuff, '{');
        if (!json_start) {
            printf("JSON part not found.\n");
            return 1;
        }

        // Parse the JSON part
        cJSON *json = cJSON_Parse(json_start);
        if (!json) {
            printf("Error parsing JSON.\n");
            return 1;
        }

        // Extract the desired variables
        cJSON *state = cJSON_GetObjectItem(json, "state");
        if (state) {
            cJSON *desired = cJSON_GetObjectItem(state, "desired");
            if (desired) {
                cJSON *home_cc_state = cJSON_GetObjectItem(desired, "home_cc_state");
                cJSON *home_cc_input = cJSON_GetObjectItem(desired, "home_cc_input");

                if (home_cc_state && cJSON_IsString(home_cc_state)) {
                    printf("home_cc_state: %s\n", home_cc_state->valuestring);
                    if (strcmp(home_cc_state->valuestring, "IDLE") == 0 && buttonFunc.type != 2 && buttonFunc.type != 0){
                        InitialBottomDisplay();
                    }
                }
                if (home_cc_input && cJSON_IsString(home_cc_input)) {
                    printf("home_cc_input: %s\n", home_cc_input->valuestring);
                    // IF RUNNING PRESET WAS BOX
                    if (strcmp(home_cc_input->valuestring, "BOX") == 0){
                        RunningPresetDisplay("Box");
                    }
                    // IF RUNNING PRESET WAS ZIGZAG
                    else if (strcmp(home_cc_input->valuestring, "ZIGZAG") == 0){
                        RunningPresetDisplay("Zigzag");
                    }
                    // IF RUNNING PRESET WAS LINE
                    else if (strcmp(home_cc_input->valuestring, "LINE") == 0){
                        RunningPresetDisplay("Line");
                    }

                }
           }
       }

       // Clean up
       cJSON_Delete(json);


    }

    return 0;
}
