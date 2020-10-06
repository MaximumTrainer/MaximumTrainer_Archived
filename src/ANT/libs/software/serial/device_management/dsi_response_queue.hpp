/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_RESPONSE_QUEUE_H)
#define DSI_RESPONSE_QUEUE_H

/////////////////////////////////////////////////////////////
// Internal Response Queue
/////////////////////////////////////////////////////////////

//TODO //TTA Why not just use std:Queue?
// !! IMPORTANT:  Any Response types that make use of this queue must always implement NONE = 0
template<typename ResponseType>
class DSIResponseQueue
{
 public:
   DSIResponseQueue()
   {
      pstFront = (Item*)NULL;
      pstLast = (Item*)NULL;
   }

   ~DSIResponseQueue()
   {
      this->Clear();
   }

   void AddResponse(ResponseType tResponse_)
   {
      if(!this->isEmpty())
      {
         pstLast->next = new Item(tResponse_);
         pstLast = pstLast->next;
      }
      else
      {
         pstLast = new Item(tResponse_);
         pstFront = pstLast;
      }
   }

   ResponseType GetResponse()
   {
      ResponseType tResponse;
      if(!this->isEmpty())
      {
         tResponse = pstFront->tResponse;
         Item* pstNewFront = pstFront->next;
         delete pstFront;
         pstFront = pstNewFront;
         if(pstFront == NULL)
            pstLast = (Item*)NULL;
      }
      else
         tResponse = (ResponseType) 0; // !! Response types must always implement NONE = 0

      return tResponse;
   }

   BOOL isEmpty()
   {
      return (pstFront == NULL);
   }

   void Clear()
   {
      Item* pstCur = pstFront;
      Item* pstLastCur = pstCur;
      while(pstCur != NULL)
      {
         pstCur = pstCur->next;
         delete pstLastCur;
         pstLastCur = pstCur;
      }
      pstFront = (Item*)NULL;
      pstLast = (Item*)NULL;
   }

 private:
   struct Item
   {
      Item(ResponseType tResponse_) { tResponse = tResponse_; next = (Item*)NULL; }
      ResponseType tResponse;
      Item* next;
   };

   Item* pstFront;
   Item* pstLast;
};


#endif // DSI_RESPONSE_QUEUE_H