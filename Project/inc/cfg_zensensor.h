#ifndef _CFG_ZENSENSOR_H
#define _CFG_ZENSENSOR_H

typedef struct
{
  UC action;                                // Type of action
  UC keyMap;                                // Button Key Map: 8 bits for each button, one bit corresponds to one relay key
} Button_Action_t;

#endif // _CFG_ZENSENSOR_H