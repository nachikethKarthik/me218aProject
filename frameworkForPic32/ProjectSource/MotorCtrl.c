/****************************************************************************
 Module
   MotorCtrl.c

 Revision
   1.0.1

 Description
   This module implements the MotorCtrl service.It implements the simple service under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 11/17/25   karthi24    started minor functionality changes, scoring system, LED service, longer messages
 11/14/25   karthi24    completed integration testing
 11/12/25   karthi24    started conversion into final code
 11/11/25   karthi24    continued adding pseudocode for functionality
 11/10/25   karthi24    started conversion into BalloonCtrl.c 
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
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
// PBCLK = 20 MHz, /8 = 2.5 MHz -> 0.4 µs per tick
#define SERVO_US_TO_TICKS(us)   ((uint16_t)(((us) * 5u) / 2u))

// 500-2500 us for the SKU: 2000-0025-0504 super speed servo
// 437-2637 us for the SKU: 31318 HS-318 servo

// B1 servo position (tunable parameter)
#define B1_MIN_US      1000u
#define B1_MAX_US      2000u

#define B1_MIN_TICKS   SERVO_US_TO_TICKS(B1_MIN_US)
#define B1_MAX_TICKS   SERVO_US_TO_TICKS(B1_MAX_US)

// B2 servo position (tunable parameter)
#define B2_MIN_US      1000u
#define B2_MAX_US      2000u

#define B2_MIN_TICKS   SERVO_US_TO_TICKS(B2_MIN_US)
#define B2_MAX_TICKS   SERVO_US_TO_TICKS(B2_MAX_US)

// B3 servo position (tunable parameter)
#define B3_MIN_US      1000u
#define B3_MAX_US      2000u

#define B3_MIN_TICKS   SERVO_US_TO_TICKS(B3_MIN_US)
#define B3_MAX_TICKS   SERVO_US_TO_TICKS(B3_MAX_US)



// Gear servo positions (tunable parameter)
#define GEAR_SERVO_DISPENSE_US      600u
#define GEAR_SERVO_REST_US  2000u

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

static void MotorHW_InitServos(void);

/* prototypes for public functions for this service.They should be functions
   relevant to the behavior of this service
*/
void MC_SetDifficultyPercent(uint8_t pct);
void MC_CommandRise(uint8_t idx);
void MC_RaiseAllToTop(void);
void MC_CommandFall(uint8_t idx);
uint8_t MC_CountBalloonsAboveMidline(void);
/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;

static Axis_t Ax[3]; // each axis has one balloon
static bool g_crashed = false;

const uint8_t chan[3] = {   B1_SERVO_CHANNEL,
                            B2_SERVO_CHANNEL,
                            B3_SERVO_CHANNEL };
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMotorCtrl

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
bool InitMotorCtrl(uint8_t Priority)
{
    ES_Event_t e = { .EventType = ES_INIT };

    MyPriority = Priority;
   
    ES_Timer_InitTimer(TID_BALLOON_UPDATE, 100); // Tunable parameter for the update rate of the servos
    
    MotorHW_InitServos();
    
    // Per-balloon calibration in ticks
    const uint16_t minTicks[3] = { B1_MIN_TICKS, B2_MIN_TICKS, B3_MIN_TICKS };
    const uint16_t maxTicks[3] = { B1_MAX_TICKS, B2_MAX_TICKS, B3_MAX_TICKS };
    
    // Init balloon axes (you?ll tune these values)
    for (uint8_t i = 0; i < 3; i++){
        Ax[i].floor_ticks   = minTicks[i];   // bottom for this balloon (ticks)
        Ax[i].ceiling_ticks = maxTicks[i];   // top for this balloon (ticks)

        Ax[i].pos_ticks     = Ax[i].ceiling_ticks; // start all at top
        Ax[i].tgt_ticks     = Ax[i].ceiling_ticks;
        Ax[i].max_step      = 50;                  // will be overridden by MC_SetDifficultyPercent
    }
    /********************************************
    in here you write your initialization code
    *******************************************/
    // post the initial transition event
    return ES_PostToService(MyPriority, e); 
}

/****************************************************************************
 Function
     PostMotorCtrl

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
bool PostMotorCtrl(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMotorCtrl

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
ES_Event_t RunMotorCtrl(ES_Event_t ThisEvent)
{
    ES_Event_t ret = { .EventType=ES_NO_EVENT };
    /********************************************
     in here you write your service code
     *******************************************/
    if((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == TID_BALLOON_UPDATE)){
        
        // Slew each axis toward target
        if(QueryGameSM() == GS_Gameplay){
            for (uint8_t i = 0; i < 3; i++) {
                int32_t delta = Ax[i].tgt_ticks - Ax[i].pos_ticks;

                if (delta >  Ax[i].max_step)  delta =  Ax[i].max_step;
                if (delta < -Ax[i].max_step)  delta = -Ax[i].max_step;

                Ax[i].pos_ticks += delta;

                // Clamp into this balloon's calibrated tick range
                if (Ax[i].pos_ticks < Ax[i].floor_ticks){
                    Ax[i].pos_ticks = Ax[i].floor_ticks;
                }
                if (Ax[i].pos_ticks > Ax[i].ceiling_ticks){
                    Ax[i].pos_ticks = Ax[i].ceiling_ticks;
                }

                // Directly drive PWM in ticks (world == ticks)
                PWMOperate_SetPulseWidthOnChannel((uint16_t)Ax[i].pos_ticks, chan[i]);

                // Crash detect at floor - similar to event checker // only for B1 for now, change for all three balloons later 
                if ((Ax[i].pos_ticks <= Ax[i].floor_ticks) && (!g_crashed)){
                    g_crashed = true; // only check for crash if none have crashed
                    ES_Event_t crash = { .EventType = ES_OBJECT_CRASHED };
                    PostGameSM(crash);
                }
            }
        }
        
        ES_Timer_InitTimer(TID_BALLOON_UPDATE, 100); // new update frame
        return ret;
    }
// Handling gear dispensing
    if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == TID_GEAR_SERVO){
//        if the timer expires, it means the servo is in the dispensing position and so needs to now come back to the resting position
        uint16_t restTicks     = SERVO_US_TO_TICKS(GEAR_SERVO_REST_US);
        PWMOperate_SetPulseWidthOnChannel(restTicks, GEAR_SERVO_CHANNEL);
        return ret;
    }

    return ret;
}

/***************************************************************************
 public functions
 ***************************************************************************/
void MC_SetDifficultyPercent(uint8_t pct){
    
    const int32_t MIN_STEP_TICKS = 10;    // very easy / slow motion tunable parameter
    const int32_t MAX_STEP_TICKS = 50;   // very hard / fast motion tunable parameter

    // Map 1->100% linearly into [MIN_STEP_TICKS to MAX_STEP_TICKS]
    // Using 99 in the denominator so that pct=1 -> MIN, pct=100 -> MAX exactly.
    int32_t step = MIN_STEP_TICKS +
                   ((int32_t)(pct - 1) * (MAX_STEP_TICKS - MIN_STEP_TICKS)) / 99;

    // Apply to all 3 balloon axes
    for (uint8_t i = 0; i < 3; i++){
        Ax[i].max_step = step;
    }
}

void MC_DispenseTwoGearsOnce(void){
//    Command the servo to the position for dispensing gears and start a timer
    uint16_t dispenseTicks = SERVO_US_TO_TICKS(GEAR_SERVO_DISPENSE_US);
    PWMOperate_SetPulseWidthOnChannel(dispenseTicks, GEAR_SERVO_CHANNEL);
    ES_Timer_InitTimer(TID_GEAR_SERVO, 500);   // 500ms before first move
}

void MC_CommandRise(uint8_t idx){ Ax[idx-1].tgt_ticks = Ax[idx-1].ceiling_ticks; }

void MC_CommandFall(uint8_t idx){ Ax[idx-1].tgt_ticks = Ax[idx-1].floor_ticks;}

void MC_RaiseAllToTop(void){
    for (uint8_t i = 0; i < 3; i++) {
        // force internal state to "at top"
        Ax[i].pos_ticks = Ax[i].ceiling_ticks;
        Ax[i].tgt_ticks = Ax[i].ceiling_ticks;

        PWMOperate_SetPulseWidthOnChannel(
            (uint16_t)Ax[i].ceiling_ticks,
            chan[i]
        );
    }
    
    // we are no longer in a crashed condition
    g_crashed = false;
}

void MC_DebugPrintAxes(void){
    printf("B1 pos=%ld tgt=%ld floor=%ld ceil=%ld\r\n",
           Ax[0].pos_ticks, Ax[0].tgt_ticks,
           Ax[0].floor_ticks, Ax[0].ceiling_ticks);
    printf("B2 pos=%ld tgt=%ld floor=%ld ceil=%ld\r\n",
           Ax[1].pos_ticks, Ax[1].tgt_ticks,
           Ax[1].floor_ticks, Ax[1].ceiling_ticks);
    printf("B3 pos=%ld tgt=%ld floor=%ld ceil=%ld\r\n",
           Ax[2].pos_ticks, Ax[2].tgt_ticks,
           Ax[2].floor_ticks, Ax[2].ceiling_ticks);
}

uint8_t MC_CountBalloonsAboveDangerline(void)
{
    uint8_t count = 0;

    for (uint8_t i = 0; i < 3; i++) {
        int32_t dangerLine = Ax[i].floor_ticks + ((Ax[i].ceiling_ticks - Ax[i].floor_ticks ) / 4); // tunable parameter for danger line in score calculation

        // "Above midline" ? you can change to >= if you prefer
        if (Ax[i].pos_ticks >= dangerLine) {
            count++;
        }
    }

    return count;
}

/***************************************************************************
 private functions
 ***************************************************************************/
static void MotorHW_InitServos(void){
    // We will use channels 1, 3, 4, 5 (OC1, OC3, OC4, OC5)
    PWMSetup_BasicConfig(5);   // init all 5 channels in the library

    // All servos share the same 50 Hz timebase on Timer3
    PWMSetup_AssignChannelToTimer(1, _Timer3_);  // gear:   OC1
    // Channel 2 / OC2 unused
    PWMSetup_AssignChannelToTimer(3, _Timer3_);  // balloon 1: OC3
    PWMSetup_AssignChannelToTimer(4, _Timer3_);  // balloon 2: OC4
    PWMSetup_AssignChannelToTimer(5, _Timer3_);  // balloon 3: OC5

    // Map channels to your chosen physical pins:
    //   OC1 -> RPB15  (gear)
    //   OC3 -> RPA3   (balloon 1)
    //   OC4 -> RPA4   (balloon 2)
    //   OC5 -> RPA2   (balloon 3)
    PWMSetup_MapChannelToOutputPin(1, PWM_RPB15);  // gear
    PWMSetup_MapChannelToOutputPin(3, PWM_RPA3);   // B1
    PWMSetup_MapChannelToOutputPin(4, PWM_RPA4);   // B2
    PWMSetup_MapChannelToOutputPin(5, PWM_RPA2);   // B3
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

