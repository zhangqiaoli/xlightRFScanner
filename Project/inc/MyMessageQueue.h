#ifndef _MY_MESSAGE_Q_H_
#define _MY_MESSAGE_Q_H_

#include "MyMessage.h"
#include "stm8s.h"

typedef struct _node_t
{
  u8 ttl;       // Time to live
  u8 data[MAX_MESSAGE_LENGTH];
  struct _node_t *pNext;
  struct _node_t *pPrev;
} MMQ_Node_t, *PMQNode;

typedef struct
{
  bool locked;
  u8 length;
  u8 maxLen;
  MMQ_Node_t *pHead;
  MMQ_Node_t *pTail;
} MMQ_Queue_t, *PMQueue;

bool MMQ_InitQueue(PMQueue pQueue, const u8 _maxlen);
int MMQ_AddMessage(PMQueue pQueue, const u8 *pData);
int MMQ_RemoveMessage(PMQueue pQueue, PMQNode pNode);
void MMQ_RemoveAllMessage(PMQueue pQueue);

#endif // _MY_MESSAGE_Q_H_