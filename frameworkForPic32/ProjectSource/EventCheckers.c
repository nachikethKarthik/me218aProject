/****************************************************************************
 Module
   EventCheckers.c

 Revision
   1.0.1

 Description
   This is the sample for writing event checkers along with the event
   checkers used in the basic framework test harness.

 Notes
   Note the use of static variables in sample event checker to detect
   ONLY transitions.

 History
 When           Who     What/Why
 -------------- ---     --------
 11/12/25       karthi24     adding code/pseudocode for the final event checker
 11/11/25       karthi24     started adding event checker pseudocode
 08/06/13 13:36 jec     initial version
****************************************************************************/

// this will pull in the symbolic definitions for events, which we will want
// to post in response to detecting events
#include "ES_Configure.h"
// This gets us the prototype for ES_PostAll
#include "ES_Framework.h"
// this will get us the structure definition for events, which we will need
// in order to post events in response to detecting events
#include "ES_Events.h"
// if you want to use distribution lists then you need those function
// definitions too.
#include "ES_PostList.h"
// This include will pull in all of the headers from the service modules
// providing the prototypes for all of the post functions
#include "ES_ServiceHeaders.h"
// this test harness for the framework references the serial routines that
// are defined in ES_Port.c
#include "ES_Port.h"
// include our own prototypes to insure consistency between header &
// actual functions definition
#include "EventCheckers.h"

#include <inttypes.h>
static uint16_t Baselines[3] = {0,0,0}; // { AN12, AN5, AN4 }
static bool g_HW_InitDone = false;

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/* prototypes for public functions for this service.They should be functions
   relevant to the behavior of this service
*/
void Targets_SetBaselines(uint16_t b12, uint16_t b5, uint16_t b4);
/***************************************************************************
 EventChecker functions
 ***************************************************************************/

  

/****************************************************************************
 Function
   Check4Keystroke
 Parameters
   None
 Returns
   bool: true if a new key was detected & posted
 Description
   checks to see if a new key from the keyboard is detected and, if so,
   retrieves the key and posts an ES_NewKey event to TestHarnessService0
 Notes
   The functions that actually check the serial hardware for characters
   and retrieve them are assumed to be in ES_Port.c
   Since we always retrieve the keystroke when we detect it, thus clearing the
   hardware flag that indicates that a new key is ready this event checker
   will only generate events on the arrival of new characters, even though we
   do not internally keep track of the last keystroke that we retrieved.
 Author
   J. Edward Carryer, 08/06/13, 13:48
****************************************************************************/
bool Check4Keystroke(void)
{
//    printf("keystroke event checker");
    if (IsNewKeyReady())   // new key waiting?
    {
      ES_Event_t ThisEvent;
      ThisEvent.EventType   = ES_NEW_KEY;
      ThisEvent.EventParam  = GetNewKey();
      ES_PostAll(ThisEvent);
      return true;
    }
    return false;
}


bool Check4HandWave(void){
    static uint8_t last = 1;                  // idle high (beam unbroken)
    uint8_t cur = BEAM_BREAK_PORT;            // direct register read
//    printf("beam break event checker\n");

    if ((cur != last) && (cur == 0)){         // falling edge = beam broken
        ES_Event_t e = { .EventType = ES_HAND_WAVE_DETECTED };
        PostGameSM(e);
        last = cur;
        
        return true;
    }
    last = cur;
    return false;
}
 
bool Check4Difficulty(void){
    
//    if(!g_HW_InitDone) return false; // dont check for difficulty unless hardware is initialized
////    With the values configured in ADC_ConfigAutoScan, the ADC_MultiRead() array indices are :
////            idx_AN4 = 0
////            idx_AN5 = 1
////            idx_AN11 = 2
////            idx_AN12 = 3
//    
//    static uint8_t lastRaw = 0xFFFF; // set a random value initially  
//    uint32_t adc[8];                         
//    ADC_MultiRead(adc);                      // reads 8 channels and stores the values
//
//    const uint16_t raw = (uint16_t)adc[2];   // AN11
//    
//    
//    
//    // 5% deadband
////    
//    if (lastRaw == 0xFFFF || raw > (lastRaw+100) || (raw < (lastRaw-100))){
//        // map 10-bit raw (0..1023) to 0..100%
//        uint8_t pct = (uint16_t)((raw * 100u) / 1023u); // with rounding math
//        printf("%1u %\n",pct);
////  Post a change in difficulty only if the difficulty changes by more than 2%
//        ES_Event_t e = { .EventType = ES_DIFFICULTY_CHANGED, .EventParam = pct };
//        
//        PostGameSM(e);
//        
//        lastRaw = raw;
//        
//        return true;
//    }
    
    return false;
}



bool Check4LaserHits(void){
    return false;
//    //Balloon mapping:
//    //
//    //B1 ? AN12
//    //B2 ? AN5
//    //B3 ? AN4
//    
//    static uint8_t isHit[3] = {0,0,0};   // 0 = currently ?no hit? plateau, 1 = ?hit? plateau
//
//    uint32_t adc[8];
//    ADC_MultiRead(adc);
//
//    // channel order from scan set: [0]=AN4, [1]=AN5, [2]=AN11, [3]=AN12
//    const uint16_t an12 = (uint16_t)adc[3];   // B1
//    const uint16_t an5  = (uint16_t)adc[1];   // B2
//    const uint16_t an4  = (uint16_t)adc[0];   // B3
//
//    const uint16_t v[3] = { an12, an5, an4 };
//
//    // margins relative to baseline
//    // when signal climbs above (baseline + HIT_DELTA) from below ? rising edge
//    // when signal falls below (baseline + RELEASE_DELTA) from above ? falling edge
//    const uint16_t HIT_DELTA     = 1000;   // ?clearly above baseline? (tuneable parameters)
//    const uint16_t RELEASE_DELTA = 20;   // ?back near baseline? (tuneable parameters)
//
//    bool any = false;
//
//    for (int i = 0; i < 3; i++) {
//
//        // --- Rising edge: NO HIT plateau ? HIT plateau ---
//        if (isHit[i] == 0) {
//            // we are currently in "no hit" state, look for jump above baseline
//            if (v[i] > (uint16_t)(Baselines[i] + HIT_DELTA)) {
//                isHit[i] = 1;  // now in HIT plateau
//
//                ES_Event_t e;
//                e.EventType = (DIRECT_HIT_B1 + i);   // B1/B2/B3
//                e.EventParam = 0;
//                PostGameSM(e);
//
//                any = true;
//            }
//        }
//
//        // --- Falling edge: HIT plateau ? NO HIT plateau ---
//        else { // isHit[i] == 1
//            // we are currently in "hit" state, look for drop back near baseline
//            if (v[i] < (uint16_t)(Baselines[i] + RELEASE_DELTA)) {
//                isHit[i] = 0;  // now in NO HIT plateau
//
//                ES_Event_t e;
//                e.EventType = (NO_HIT_B1 + i);       // B1/B2/B3
//                e.EventParam = 0;
//                PostGameSM(e);
//
//                any = true;
//            }
//        }
//    }
//
//    return any;
}

/***************************************************************************
 public functions
 ***************************************************************************/
void Targets_SetBaselines(uint16_t b12, uint16_t b5, uint16_t b4){
  
//printf("Setting Baseline values of AN12(B1)=%lu AN5(B2)=%lu AN4(B3)=%lu\r\n",
//                                   b12, b5, b4);
    Baselines[0] = b12;  // B1 ? AN12
    Baselines[1] = b5;   // B2 ? AN5
    Baselines[2] = b4;   // B3 ? AN4
}




