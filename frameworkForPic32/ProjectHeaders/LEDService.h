/****************************************************************************

  Header file for LED service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef LEDService_H
#define LEDService_H

#include "ES_Types.h"
//typedefs


// Small ID for canned messages
typedef enum {
  LED_MSG_WELCOME = 0,   // "WELCOME"
  // add more if you want other fixed messages later
} LED_MessageID_t;

// Public Function Prototypes

bool InitLEDService(uint8_t Priority);
bool PostLEDService(ES_Event_t ThisEvent);
ES_Event_t RunLEDService(ES_Event_t ThisEvent);

#endif /* LEDService_H */

