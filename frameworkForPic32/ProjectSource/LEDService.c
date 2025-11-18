/****************************************************************************
 Module
   LEDService.c

 Revision
   1.0.1

 Description
   This module controls all functionality related to the MAX7219 dot matrix LED module and is implementing a simple service under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 11/17/25      karthi24  began conversion from TemplateService.c
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"

#include "LEDService.h"
#include "GameSM.h"
#include "DM_Display.h"
#include "FontStuff.h"
#include "PIC32_SPI_HAL.h"
/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
static void LED_SPI_Init(void);
static void LED_RenderDifficulty(uint8_t pct);
static void LED_RenderCountdown(uint8_t seconds_remaining);
static void LED_RenderScore(uint16_t score);
static void LED_RenderMessage(LED_MessageID_t msgID);
/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static bool    g_LedPushPending = false;
static bool g_DisplayInitDone = false;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTemplateService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
****************************************************************************/
bool InitLEDService(uint8_t Priority)
{
    MyPriority = Priority;
    /********************************************
     in here you write your initialization code
     *******************************************/
    // SPI + MAX7219 interface init
    LED_SPI_Init();
    // post the initial transition event
    ES_Event_t initEvent = { .EventType = ES_INIT };
    return ES_PostToService(MyPriority, initEvent);
}

/****************************************************************************
 Function
     PostLEDService

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostLEDService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunLEDService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunLEDService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent = { .EventType = ES_NO_EVENT };
  /********************************************
   in here you write your service code
   *******************************************/
  switch(ThisEvent.EventType){
        case ES_INIT:{
            // --- Code : Display init sequence ---
            if (!g_DisplayInitDone) {
                bool done = DM_TakeInitDisplayStep();   // performs 1 small init step

                if (!done) {
                    // Not finished yet: re-post ES_INIT to ourselves so that
                    // the next step happens on the next framework dispatch.
                    ES_Event_t again = { .EventType = ES_INIT };
                    PostLEDService(again);

                    // Stay in GS_InitPState and DO NOT fall through to the rest yet.
                    return ReturnEvent;
                }

                    // At this point the display is fully initialized.
                    g_DisplayInitDone = true;
            }
        }break;
        
        case ES_LED_SHOW_DIFFICULTY:
            LED_RenderDifficulty((uint8_t)ThisEvent.EventParam);
            break;
            
        case ES_LED_SHOW_COUNTDOWN:
            LED_RenderCountdown((uint8_t)ThisEvent.EventParam);
            break;
        
        case ES_LED_SHOW_SCORE:
            LED_RenderScore((uint16_t)ThisEvent.EventParam);
            break;
            
        case ES_LED_SHOW_MESSAGE:
            LED_RenderMessage((LED_MessageID_t)ThisEvent.EventParam);
            break;
        
        case ES_LED_PUSH_STEP:{
            // Row-by-row non-blocking push to the physical display
            if (g_LedPushPending) {
                bool done = DM_TakeDisplayUpdateStep();   // sends 1 row this call
                if (!done) {
                    ES_Event_t again = { .EventType = ES_LED_PUSH_STEP };
                    PostLEDService(again);
                } else {
                    g_LedPushPending = false;             // all 8 rows sent
                }
            }      
        }break;
            
            
            
  }
  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/
static void LED_SPI_Init(void)
{
  // Basic SPI1 setup for MAX7219 dot-matrix
  SPISetup_BasicConfig(SPI_SPI1);                        // default base config
  SPISetup_SetLeader(SPI_SPI1, true);                    // PIC is the master

  // Bit time: choose something reasonably fast
  // Assuming PBCLK = 10MHz, a 100ns bit-time ? 10MHz SPI.
  SPISetup_SetBitTime(SPI_SPI1, 100);                    // 100 ns / bit

  SPISetup_MapSSOutput(SPI_SPI1, SPI_RPA0);              // RPA0: follower select
  SPISetup_MapSDOutput(SPI_SPI1, SPI_RPA1);              // RPA1: SDO1

  // Idle clock low, data on rising edge (CPOL=0, CPHA=0)
  SPISetup_SetClockIdleState(SPI_SPI1, SPI_CLK_HI);          // SS active low, SCK idle low
  SPISetup_SetActiveEdge(SPI_SPI1, SPI_SECOND_EDGE);                // data valid on rising edge

  // 16-bit mode, no enhanced buffer
  SPISetup_SetXferWidth(SPI_SPI1, SPI_16BIT);
  SPISetEnhancedBuffer(SPI_SPI1, true);

  SPISetup_EnableSPI(SPI_SPI1);
}

static void LED_RenderDifficulty(uint8_t pct)
{
    if (pct < 1)   pct = 1;
    if (pct > 100) pct = 100;

    char buf[4];
    sprintf(buf, "%u", (unsigned)pct);

    DM_ClearDisplayBuffer();

    for (char *p = buf; *p; p++) {
        DM_AddChar2DisplayBuffer((unsigned char)*p);
        DM_ScrollDisplayBuffer(4);
    }

    g_LedPushPending = true;
    ES_Event_t e = { .EventType = ES_LED_PUSH_STEP };
    PostLEDService(e);
}

static void LED_RenderCountdown(uint8_t seconds_remaining)
{
    char buf[4];
    sprintf(buf, "%u", (unsigned)seconds_remaining);

    DM_ClearDisplayBuffer();
    for (char *p = buf; *p != '\0'; p++) {
        DM_AddChar2DisplayBuffer((unsigned char)*p);
        DM_ScrollDisplayBuffer(4);   // 3 cols glyph + 1 space
    }

    g_LedPushPending = true;
    ES_Event_t e = { .EventType = ES_LED_PUSH_STEP };
    PostLEDService(e);
}

static void LED_RenderScore(uint16_t score)
{
    char numBuf[6];         // up to 65535
    sprintf(numBuf, "%u", (unsigned)score);

    DM_ClearDisplayBuffer();

    // Prefix, same as in your GameSM: "SC:"
    const char *prefix = "SC:";
    for (const char *p = prefix; *p != '\0'; p++) {
        DM_AddChar2DisplayBuffer((unsigned char)*p);
        DM_ScrollDisplayBuffer(4);
    }

    // Numeric part
    for (char *p = numBuf; *p != '\0'; p++) {
        DM_AddChar2DisplayBuffer((unsigned char)*p);
        DM_ScrollDisplayBuffer(4);
    }

    g_LedPushPending = true;
    ES_Event_t e = { .EventType = ES_LED_PUSH_STEP };
    PostLEDService(e);
}

static void LED_RenderMessage(LED_MessageID_t msgID)
{
    const char *msg = "WELCOME";

    switch (msgID) {
        case LED_MSG_WELCOME: msg = "WELCOME";  break;
          // for additional messages, add more cases here
        default:  msg = "WELCOME";  break;
    }

    DM_ClearDisplayBuffer();
    for (const char *p = msg; *p != '\0'; p++) {
        DM_AddChar2DisplayBuffer((unsigned char)(*p));
        DM_ScrollDisplayBuffer(4);
    }

    g_LedPushPending = true;
    ES_Event_t e = { .EventType = ES_LED_PUSH_STEP };
    PostLEDService(e);
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

