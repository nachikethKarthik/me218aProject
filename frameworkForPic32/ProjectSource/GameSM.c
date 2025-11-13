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
#define GEAR_SERVO_CHANNEL      1u
#define SERVO_US_TO_TICKS(us)   ((uint16_t)(((us) * 5u) / 2u)) // 0.4 µs per tick
// dwell times for gear dispensing(how long to wait at each position)
#define GEAR_DWELL_TO_DISPENSE_MS  500u   // rest -> dispense
#define GEAR_DWELL_TO_REST_MS      500u   // dispense -> rest

#define GEAR_SERVO_REST_US      1500u   // neutral/home
#define GEAR_SERVO_DISPENSE_US  2100u   // push 2 gears (tune as needed)

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static void GameHW_InitPins(void);
static void CaptureALS_Baselines_Init(void);

static void LED_BeginRenderDifficulty(uint8_t pct);
static void LED_SPI_Init(void); 
static void LED_ShowWelcome(void);
static void LED_ShowCountdown(uint8_t seconds_remaining);

static void DispenseTwoGearsOnce(void);
/* prototypes for public functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
void SetAllBalloonsToTop(void);
/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match that of enum in header file
static GameState_t CurrentState;
static uint8_t     SecondsLeft;

// g stands for guard, GS stands for Game State, BC stands for Balloon control
static bool g_LedPushPending = false;
static bool        g_DisplayInitDone = false;

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

    MyPriority = Priority;

    CurrentState = GS_InitPState;

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
    
    // Global handler: row-by-row LED update: handled regardless of CurrentState
    if (ThisEvent.EventType == ES_LED_PUSH_STEP){
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
    
    // Global handler: --- Gear servo mini-state machine: handled regardless of CurrentState ---
    if (ThisEvent.EventType == ES_TIMEOUT &&
        ThisEvent.EventParam == TID_GEAR_SERVO) {

        uint16_t restTicks     = SERVO_US_TO_TICKS(GEAR_SERVO_REST_US);
        uint16_t dispenseTicks = SERVO_US_TO_TICKS(GEAR_SERVO_DISPENSE_US);

        switch (GearState) {
          case GEAR_GOING_TO_DISPENSE:
            // Now command DISPENSE position and schedule return to rest
            PWMOperate_SetPulseWidthOnChannel(dispenseTicks, GEAR_SERVO_CHANNEL);
            GearState = GEAR_GOING_TO_REST;
            ES_Timer_InitTimer(TID_GEAR_SERVO, GEAR_DWELL_TO_REST_MS);
            break;

          case GEAR_GOING_TO_REST:
            // Return to REST and finish the sequence
            PWMOperate_SetPulseWidthOnChannel(restTicks, GEAR_SERVO_CHANNEL);
            GearState = GEAR_IDLE;
            break;

          default:
            // If timer fires unexpectedly, just force idle
            GearState = GEAR_IDLE;
            break;
        }

        // We handled the event; no need to feed it into the main state machine.
        return ReturnEvent;
    }
    
    switch (CurrentState)
    {
        case GS_InitPState:        // If current state is initial Psedudo State
        {
            if (ThisEvent.EventType == ES_INIT)    // only respond to ES_Init
            {
                // Initialize SPI and LED display
                LED_SPI_Init();
                
                // --- Non-blocking display init sequence ---
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
                
                GameHW_InitPins();
                
                // Capture baselines for ALS-PT19 sensors once at boot
                CaptureALS_Baselines_Init();
                
                LED_ShowWelcome();
                MC_RaiseAllToTop();
                CurrentState = GS_Welcome;
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
                  SecondsLeft = 60;
                  LED_ShowCountdown(SecondsLeft);
                  // Start timers: 60s gameplay, 20s inactivity, 1s tick
                  ES_Timer_InitTimer(TID_GAME_60S,     60000);
                  ES_Timer_InitTimer(TID_INACTIVITY_20S, 20000);
                  ES_Timer_InitTimer(TID_TICK_1S,       1000);
                  // Begin falling all balloons
                  MC_CommandFall(1); MC_CommandFall(2); MC_CommandFall(3);
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
                        ES_Timer_InitTimer(TID_TICK_1S,1000);
                        
                    } else if (ThisEvent.EventParam == TID_GAME_60S){ // Victory - Game over
                        
                      CurrentState = GS_CompletingMode;
                      ES_Timer_InitTimer(TID_MODE_3S,3000);

                    } else if (ThisEvent.EventParam == TID_INACTIVITY_20S){ // User inactive - Game over

                      CurrentState = GS_NoUserInput;
                      ES_Timer_InitTimer(TID_MODE_3S,3000);

                    }break;
                    
                    case ES_OBJECT_CRASHED:
                        CurrentState = GS_LosingMode;
                        ES_Timer_InitTimer(TID_MODE_3S,3000);
                        break;

            }break;

        }break;
        
        case GS_NoUserInput: {
            if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == TID_MODE_3S){
                MC_RaiseAllToTop();
                LED_ShowWelcome();
                CurrentState = GS_WaitingForHandWave;
            }
        }break;
        
        case GS_LosingMode: {
            if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == TID_MODE_3S){
                MC_RaiseAllToTop();
                LED_ShowWelcome();
                CurrentState = GS_WaitingForHandWave;
            } 
        }break;
        
        case GS_CompletingMode: {
            if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == TID_MODE_3S){
                MC_RaiseAllToTop();
                MC_DispenseTwoGearsOnce();                      // sweep min to max one time
                LED_ShowWelcome();
                CurrentState = GS_WaitingForHandWave;
            }
        }break;
        
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
    BEAM_BREAK_CNPU = 1;
    

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
    if (pct < 1) pct = 1; if (pct > 100) pct = 100;
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

static void LED_ShowWelcome(){
    // Renders the string "WELCOME" into the off-screen buffer,
    // then kicks off a non-blocking push using ES_LED_PUSH_STEP.

    const char *msg = "WELCOME";   // all caps; font covers ASCII chars

    // Clear the off-screen frame buffer
    DM_ClearDisplayBuffer();

    // Build the text into the buffer, similar to LED_BeginRenderDifficulty()
    // Each char: add glyph, then scroll a few columns to make room. 
    for (const char *p = msg; *p != '\0'; p++){
            DM_AddChar2DisplayBuffer((unsigned char)(*p));
            DM_ScrollDisplayBuffer(4);
    }

    // Mark that a push to the physical display is required
    g_LedPushPending = true;

    // Kick the first row transfer using the existing ES_LED_PUSH_STEP mechanism
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
