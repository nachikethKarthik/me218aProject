/****************************************************************************
 Module
   GameSM.c

 Revision
   1.0.1

 Description
   This module implements the top level state machine for the game. It uses the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 11/17/25   karthi24    started minor functionality changes, scoring system, LED service, longer messages
 11/14/25   karthi24    completed integration testing
 11/13/25   karthi24    began integration testing
 11/12/25   karthi24    started conversion into final code
 11/11/25   karthi24    started adding more states into the pseudocode
 11/10/25   karthi24    started conversion into GameSM.c
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "EventCheckers.h"
#include "ES_Framework.h"
#include "DM_Display.h"
#include "GameSM.h"
#include "MotorCtrl.h"
#include "PIC32_AD_Lib.h"
#include "PIC32_SPI_HAL.h"
#include "PWM_PIC32.h"

/*----------------------------- Module Defines ----------------------------*/

//#define START_IN_TEST_MODE // used for debugging and testing modules in the state machine, comment out for final functionality

#define SERVO_US_TO_TICKS(us)   ((uint16_t)(((us) * 5u) / 2u)) // 0.4 µs per tick

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static void GameHW_InitPins(void);
static void CaptureALS_Baselines_Init(void);

static void LED_BeginRenderDifficulty(uint8_t pct);
static void LED_SPI_Init(void); 
static void LED_ShowCountdown(uint8_t seconds_remaining);
static void LED_ShowMessage(const char *msg);
static void LED_ShowScore(uint16_t);

/* prototypes for public functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
void SetAllBalloonsToTop(void);
/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
static GameState_t CurrentState;
static uint8_t     SecondsLeft;

// g stands for global, GS stands for Game State, BC stands for Balloon control
static bool g_LedPushPending = false;
static bool g_DisplayInitDone = false;
static bool g_SPI_InitDone = false;

static uint16_t    g_Score; // running variable for score

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitGameSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
****************************************************************************/
bool InitGameSM(uint8_t Priority)
{
    
    ES_Event_t e = { .EventType = ES_INIT };
    
    LED_SPI_Init();
    MyPriority = Priority;
    
    GameHW_InitPins();
    
    #ifdef START_IN_TEST_MODE
    CurrentState = GS_TestMode;
    #else
    CurrentState = GS_InitPState;
    #endif

    printf("Init function complete\n\r");
    
    return ES_PostToService(MyPriority, e);   
}

/****************************************************************************
 Function
     PostGameSM

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostGameSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunGameSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunGameSM(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent = { .EventType = ES_NO_EVENT };
//     printf("Entering RunGameSM because of event: %lu\r\n", ThisEvent.EventType);
    // Global handler: row-by-row LED update: handled regardless of CurrentState
    if (ThisEvent.EventType == ES_LED_PUSH_STEP){
//        printf("Pushing to LED\r\n");
        if (g_LedPushPending){
            bool done = DM_TakeDisplayUpdateStep();   // sends 1 row this call 
            if (!done){
                // not finished: post yourself again to send the next row later
                ES_Event_t again = { .EventType = ES_LED_PUSH_STEP };
                PostGameSM(again);
            } else {
              g_LedPushPending = false;               // all 8 rows sent
            }
        }    
        return ReturnEvent;
    }
    
    switch (CurrentState)
    {
        case GS_InitPState:        // If current state is initial Psedudo State
        {
            //
            if (ThisEvent.EventType == ES_INIT)    // only respond to ES_Init
            {

                // Capture baselines for ALS-PT19 sensors once at boot
                CaptureALS_Baselines_Init();
                
                // --- Blocking Code : Display init sequence ---
                if (!g_DisplayInitDone) {
                    bool done = DM_TakeInitDisplayStep();   // performs 1 small init step

                    if (!done) {
                        // Not finished yet: re-post ES_INIT to ourselves so that
                        // the next step happens on the next framework dispatch.
                        ES_Event_t again = { .EventType = ES_INIT };
                        PostGameSM(again);

                        // Stay in GS_InitPState and DO NOT fall through to the rest yet.
                        return ReturnEvent;
                    }

                        // At this point the display is fully initialized.
                        g_DisplayInitDone = true;
                }
//                printf("running show welcome\n\r");
                LED_ShowMessage("WELCOME");
                MC_RaiseAllToTop();
                CurrentState = GS_WaitingForHandWave;
            }
        }break;

        case GS_WaitingForHandWave: {
            switch(ThisEvent.EventType){

              case ES_DIFFICULTY_CHANGED:{
                  uint8_t pct = ThisEvent.EventParam;
                  LED_BeginRenderDifficulty(pct);    // render number on the matrix
                  MC_SetDifficultyPercent(pct);     // update motion speeds
              }break;

              case ES_HAND_WAVE_DETECTED: // from event checker
                  g_Score     = 0;
                  
                  SecondsLeft = 60;
                  
                  
                  LED_ShowCountdown(SecondsLeft);
                  // Start timers: 60s gameplay, 20s inactivity, 1s tick
                  ES_Timer_InitTimer(TID_GAME_60S,     60000);
                  ES_Timer_InitTimer(TID_INACTIVITY_20S, 20000);
                  ES_Timer_InitTimer(TID_TICK_1S,       1000);
                  // Begin falling all balloons
                  MC_CommandFall(1); 
                  MC_CommandFall(2); 
                  MC_CommandFall(3);
                  CurrentState = GS_Gameplay;
                 
                  break;
                  
            }break;
            
        }break;

        case GS_Gameplay: {
            switch(ThisEvent.EventType){

                // Laser hit logic (laser hit implies RISE; no hit implies FALL)
                case DIRECT_HIT_B1: {MC_CommandRise(1); ES_Timer_InitTimer(TID_INACTIVITY_20S,20000);} break;

                case DIRECT_HIT_B2: {MC_CommandRise(2); ES_Timer_InitTimer(TID_INACTIVITY_20S,20000);} break;

                case DIRECT_HIT_B3: {MC_CommandRise(3); ES_Timer_InitTimer(TID_INACTIVITY_20S,20000);} break;

                case NO_HIT_B1:{    MC_CommandFall(1);} break;

                case NO_HIT_B2:{    MC_CommandFall(2);} break;

                case NO_HIT_B3:{    MC_CommandFall(3);} break;

                case ES_TIMEOUT:
                    
                    if (ThisEvent.EventParam == TID_TICK_1S){            // 1 Hz display update
                        
                        if (SecondsLeft) SecondsLeft--;
                        LED_ShowCountdown(SecondsLeft);

                        uint8_t afloat = MC_CountBalloonsAboveDangerline();
                        g_Score += afloat;

                        ES_Timer_InitTimer(TID_TICK_1S,1000);
                        
                    } else if (ThisEvent.EventParam == TID_GAME_60S){ // Victory - Game over
                        
                      CurrentState = GS_CompletingMode;
                      
                      LED_ShowScore(g_Score);
                      
                      ES_Timer_InitTimer(TID_MODE_3S,3000);

                    } else if (ThisEvent.EventParam == TID_INACTIVITY_20S){ // User inactive - Game over

                      CurrentState = GS_NoUserInput;
                      ES_Timer_InitTimer(TID_MODE_3S,3000);

                    }break;
                    
                    case ES_OBJECT_CRASHED:
                        CurrentState = GS_LosingMode;
                        
                        LED_ShowScore(g_Score);
                        
                        ES_Timer_InitTimer(TID_MODE_3S,3000);
                        break;

            }break;

        }break;
        
        case GS_NoUserInput: {
            if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == TID_MODE_3S){
                MC_RaiseAllToTop();
                LED_ShowMessage("WELCOME");
                CurrentState = GS_WaitingForHandWave;
            }
        }break;
        
        case GS_LosingMode: {
            if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == TID_MODE_3S){
                MC_RaiseAllToTop();
                MC_DispenseTwoGearsOnce();
                LED_ShowMessage("WELCOME");
                CurrentState = GS_WaitingForHandWave;
            } 
        }break;
        
        case GS_CompletingMode: {
            if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == TID_MODE_3S){
                MC_RaiseAllToTop();
                MC_DispenseTwoGearsOnce();                      // sweep min to max one time
                LED_ShowMessage("WELCOME");
                CurrentState = GS_WaitingForHandWave;
            }
        }break;
        
        case GS_TestMode: {
            // entering calibration mode this stops motor ctrl:
            ES_Timer_StopTimer(TID_BALLOON_UPDATE);  // MotorCtrl won?t run updates 
            printf("Entering test mode because of event: %lu\r\n", ThisEvent.EventType);
            switch (ThisEvent.EventType) {
                case ES_NEW_KEY: {
                        char k = (char)ThisEvent.EventParam;

                        switch (k) {
                            
                            case '1':
                                printf("testing the GS_WaitingForHandWave state\r\n");
                                
                                break;
                            
                            case '2':
                                printf("testing the beam break sensor\r\n");
                                printf("value at digital input is : %lu\r\n",
                                       BEAM_BREAK_PORT);
                                break;
                            
                            case '3':{   // Check4LaserHits event checker test
                                // read ADCs once and print
                                
                                
                            }break;
                            
                            
                            case 'm':
                                printf("testing servo motors\r\n");
                                // 500-2500 us for the SKU: 2000-0025-0504 super speed servo
                                // 437-2637 us for the SKU: 31318 HS-318 servo
                                static uint16_t TestPulseUs    = 1500; // 500-2500 us for the SKU: 2000-0025-0504 super speed
                                uint16_t ticks = SERVO_US_TO_TICKS(TestPulseUs);
                                
                                printf("Commanding Channel 3 OC3 pin 10. Motor for B1. Commanding it to pwm microsecond value %lu\r\n", TestPulseUs);
                                PWMOperate_SetPulseWidthOnChannel(ticks, B3_SERVO_CHANNEL);
                                // similar print state
                                break;
                            
                            case 'a':{   // analog test
                                // read ADCs once and print
                                GameHW_InitPins();
                                uint32_t adc[8];
                                ADC_MultiRead(adc);
                                printf("AN11(slider)=%lu AN12(B1)=%lu AN5(B2)=%lu AN4(B3)=%lu\r\n",
                                       adc[2], adc[3], adc[1], adc[0]);
                            }break;
                            

                            case '8':   // move B1 up : Requires re-enabling of the motor control timer
                                MC_CommandRise(1);
                                printf("B1 rise\r\n");
                                break;

                            case 'q':   // move B1 down : Requires re-enabling of the motor control timer
                                MC_CommandFall(1);
                                printf("B1 fall\r\n");
                                break;

                            case '9':   // B2 up : Requires re-enabling of the motor control timer
                                MC_CommandRise(2);
                                printf("B2 rise\r\n");
                                break;

                            case 'w':   // B2 down : Requires re-enabling of the motor control timer
                                MC_CommandFall(2);
                                printf("B2 fall\r\n");
                                break;

                            case 'f':   // B3 up : Requires re-enabling of the motor control timer
                                MC_CommandRise(3);
                                printf("B3 rise\r\n");
                                break;

                            case 'e':   // B3 down : Requires re-enabling of the motor control timer
                                MC_CommandFall(3);
                                printf("B3 fall\r\n");
                                break;

                            case 'g':   // test gear dispenser
                                MC_DispenseTwoGearsOnce();
                                printf("Dispense test\r\n");
                                break;
                            
                            

                            case 'd':   // dump positions
                                MC_DebugPrintAxes();
                                break;

                            case 'x':   // leave test mode, run actual game
                                LED_ShowMessage("WELCOME");
                                CurrentState = GS_WaitingForHandWave;
                                printf("Exiting TestMode and restarting motor Ctrl timer ? WaitingForHandWave\r\n");
                                ES_Timer_StartTimer(TID_BALLOON_UPDATE);  // Restart MotorCtrl timer
                                break;
                        }//switch (k)
                        
                }break; //case ES_NEW_KEY:

                default:
                // ignore other events in TestMode
                break;
                
            }//switch (ThisEvent.EventType)
            
        } break; // case GS_TestMode:
        
        default: break;
        
    }
  return ReturnEvent;
  
}


/****************************************************************************
 Function
     QueryGameSM

 Parameters
     None

 Returns
     GameState_t The current state of the game state machine

 Description
     returns the current state of the game state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:21
****************************************************************************/
GameState_t QueryGameSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/


static void GameHW_InitPins(void){
    // --- Beam-break: digital input, pull-up, digital mode ---
    BEAM_BREAK_TRIS = 1;
    BEAM_BREAK_CNPU = 0;
    

    // --- Slider (AN11): analog input ---
    SLIDER_TRIS = 1;
    SLIDER_ANSEL = 1;

    // --- ALS sensors (AN12, AN5, AN4): analog inputs ---
    ALS1_TRIS = 1; ALS1_ANSEL = 1;   // AN12 / RB12
    ALS2_TRIS = 1; ALS2_ANSEL = 1;   // AN5  / RB3
    ALS3_TRIS = 1; ALS3_ANSEL = 1;   // AN4  / RB2
    
    ADC_ConfigAutoScan(ADC_CHANSET);
}

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
static void LED_BeginRenderDifficulty(uint8_t pct){
    if (pct < 0) pct = 1; if (pct > 100) pct = 100;
    char buf[4]; sprintf(buf, "%u", (unsigned)pct); //Convert difficulty into string

    DM_ClearDisplayBuffer();                 // clear off-screen buffer

    for (char *p = buf; *p; p++){ 
      DM_AddChar2DisplayBuffer(*p);          // Add glyph into buffer 
      DM_ScrollDisplayBuffer(4);             // space before each glyph 
    }

    g_LedPushPending = true;                 // mark that rows must be sent
    // kick the first row push by posting a local event to ourselves
    ES_Event_t e = { .EventType = ES_LED_PUSH_STEP};
    PostGameSM(e);
}

static void LED_ShowScore(uint16_t score)
{
    char numBuf[6];  // enough for scores up to 65535
    sprintf(numBuf, "%u", (unsigned)score);

    DM_ClearDisplayBuffer();

    // 1) Add prefix "SCORE: "
    const char *prefix = "SC:";
    for (const char *p = prefix; *p != '\0'; p++) {
        DM_AddChar2DisplayBuffer((unsigned char)(*p));
        DM_ScrollDisplayBuffer(4);   // spacing between chars
    }

    // 2) Add the numeric part
    for (char *p = numBuf; *p != '\0'; p++) {
        DM_AddChar2DisplayBuffer((unsigned char)(*p));
        DM_ScrollDisplayBuffer(4);
    }

    // 3) Kick off non-blocking push to the physical display
    g_LedPushPending = true;
    ES_Event_t e = { .EventType = ES_LED_PUSH_STEP };
    PostGameSM(e);
}

static void LED_ShowCountdown(uint8_t seconds_remaining){
    // Render the remaining seconds as a decimal number on the 4-module display.
    // This is non-blocking: we only update the frame buffer here and then
    // kick off the ES_LED_PUSH_STEP mechanism to stream rows out via SPI.

    char buf[4];  // enough for "60", "100", etc.
    sprintf(buf, "%u", (unsigned)seconds_remaining);

    // Clear off-screen buffer
    DM_ClearDisplayBuffer();

    // Add each digit and scroll a bit to space them
    for (char *p = buf; *p != '\0'; p++) {
        DM_AddChar2DisplayBuffer((unsigned char)*p);
        DM_ScrollDisplayBuffer(4);   // 3 cols glyph + 1 col space
    }

    // Mark that rows need to be pushed to the hardware
    g_LedPushPending = true;

    // Kick the first non-blocking update step
    ES_Event_t e = { .EventType = ES_LED_PUSH_STEP };
    PostGameSM(e);
}

static void LED_ShowMessage(const char *msg)
{
    // Clear the off-screen frame buffer
    DM_ClearDisplayBuffer();

    // Build the text into the buffer
    for (const char *p = msg; *p != '\0'; p++) {
        DM_AddChar2DisplayBuffer((unsigned char)(*p));
        DM_ScrollDisplayBuffer(4);   // spacing between chars
    }

    // Mark that a push to the physical display is required
    g_LedPushPending = true;

    // Kick the first row transfer using the ES_LED_PUSH_STEP mechanism
    ES_Event_t e = { .EventType = ES_LED_PUSH_STEP };
    PostGameSM(e);
}

static void CaptureALS_Baselines_Init(void){
    enum { N = 10 };                 // number of samples to average
    uint32_t adc[8];
    uint32_t sum_an12 = 0, sum_an5 = 0, sum_an4 = 0;

    for(int i=0;i<N;i++){
        ADC_MultiRead(adc);            // indices by ascending AN: [0]=AN4, [1]=AN5, [2]=AN11, [3]=AN12
        uint16_t an4  = (uint16_t)adc[0];
        uint16_t an5  = (uint16_t)adc[1];
        uint16_t an12 = (uint16_t)adc[3];

        sum_an12 += an12;
        sum_an5  += an5;
        sum_an4  += an4;
    }

    uint16_t b12 = (uint16_t)(sum_an12 / N);
    uint16_t b5  = (uint16_t)(sum_an5  / N);
    uint16_t b4  = (uint16_t)(sum_an4  / N);

    Targets_SetBaselines(b12, b5, b4);
}
