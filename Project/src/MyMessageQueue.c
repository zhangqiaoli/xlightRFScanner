#include "MyMessageQueue.h"
#include <stdlib.h>
#include <string.h> 

// Init MQ
bool MMQ_InitQueue(PMQueue pQueue, const u8 _maxlen)
{
  if( !pQueue ) return FALSE;
  if( pQueue->length > 0 || pQueue->maxLen > 0 ) return FALSE;
  
  MMQ_Node_t *pNewNode, *pOldNode = 0;
  for( u8 i = 0; i < _maxlen; i++ ) {
    pNewNode = (MMQ_Node_t *)malloc(sizeof(MMQ_Node_t));
    if( !pNewNode ) return FALSE;
    pNewNode->pNext = 0;
    pNewNode->pPrev = 0;
    pNewNode->ttl = 0;
    if( pOldNode ) {
      // First node
      pQueue->pHead = pNewNode;
      pQueue->pTail = pNewNode;
    } else {
      pOldNode->pNext = pNewNode;
      pNewNode->pPrev = pOldNode;      
    }
    pOldNode = pNewNode;
  }

  // Make a loop
  if( pQueue->pHead )
  {
    pQueue->pHead->pPrev = pOldNode;
    pOldNode->pNext = pQueue->pHead;
  }
  pQueue->maxLen = _maxlen;
  pQueue->locked = FALSE;
  return TRUE;
}

// Add message at the end of queue
int MMQ_AddMessage(PMQueue pQueue, const u8 *pData)
{
  if( !pQueue || !pData ) return -1;
  
  u16 _timeout = 0xAFF;
  while(_timeout-- && pQueue->locked);
  if( pQueue->locked ) return -2;
  
  int rc = 0;
  pQueue->locked = TRUE;
  if( pQueue->length < pQueue->maxLen )
  {
    // Set Data
    pQueue->pTail->ttl = 0;
    memcpy(pQueue->pTail->data, pData, MAX_MESSAGE_LENGTH);
    pQueue->pTail = pQueue->pTail->pNext;
    pQueue->length++;
    rc = 1;
  }
  pQueue->locked = FALSE;
  return rc;
}

// Remove member
int MMQ_RemoveMessage(PMQueue pQueue, PMQNode pNode)
{
  u16 _timeout = 0xAFF;
  while(_timeout-- && pQueue->locked);
  if( pQueue->locked ) return -2;
  
  int rc = 0;
  pQueue->locked = TRUE;
  if( pNode == 0 ) pNode = pQueue->pHead;
  if( pNode != pQueue->pTail && pQueue->length > 0 ) {
    // Rearrange node chain
    pNode->pPrev->pNext = pNode->pNext;
    pNode->pNext->pPrev = pNode->pPrev;
    pNode->pNext = pQueue->pTail->pNext;
    pNode->pPrev = pQueue->pTail;
    pQueue->pTail->pNext->pPrev = pNode;
    pQueue->pTail->pNext = pNode;
    pQueue->length--;
  }
  pQueue->locked = FALSE;
  return rc;
}

// Remove all messages from queue
void MMQ_RemoveAllMessage(PMQueue pQueue)
{
  // Lock Queue
  pQueue->locked = TRUE;
  pQueue->length = 0;
  pQueue->pHead = pQueue->pTail;
  // Unlock
  pQueue->locked = FALSE;
}