#include "arch/v4.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)

void CMSConnection4::optimize()

{
  CBEType *mwType = msFactory->getMachineWordType();
  int mwAlignment = mwType->getAlignment();

  /* Kernel messages: Check and modify layout */
    
  if (iid == IID_KERNEL)
    {
      CMSChunk4 *item;
      bool isOk;
      switch (fid)
        {
          case 0 : 
            do {
                 isOk = false;
                 if (mem_fixed[CHANNEL_IN]->isEmpty())
                   break;
                 item = (CMSChunk4*)mem_fixed[CHANNEL_IN]->getFirstElement();
                 if ((item->size != globals.word_size) || (item->channels != 1))
                   break;
                 if (item->next->isEndOfList())
                   break;
                 item = (CMSChunk4*)item->next;
                 if ((item->size != globals.word_size) || (item->channels != 1))
                   break;
                 if (!item->next->isEndOfList())
                   break;
                 if (!mem_fixed[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!fpages[CHANNEL_IN]->isEmpty() || !fpages[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!strings[CHANNEL_IN]->isEmpty() || !strings[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!mem_variable[CHANNEL_IN]->isEmpty() || !mem_variable[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!(options & OPTION_ONEWAY))
                   break;
                   
                 isOk = true;
               } while (0);
               
            if (!isOk)
              panic("malformed startup message; use oneway 'in word, in word' as a signature");
            break;

          case 1 : 
            do {
                 isOk = false;
                 if (!mem_fixed[CHANNEL_IN]->isEmpty() || !mem_fixed[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!fpages[CHANNEL_IN]->isEmpty() || !fpages[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!strings[CHANNEL_IN]->isEmpty() || !strings[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!mem_variable[CHANNEL_IN]->isEmpty() || !mem_variable[CHANNEL_OUT]->isEmpty())
                   break;
                 isOk = true;
               } while (0);
               
            if (!isOk)
              panic("malformed interrupt message; use empty signature");
            break;
        
          case 2 : 
            do {
                 isOk = false;
                 if (mem_fixed[CHANNEL_IN]->isEmpty())
                   break;
                 item = (CMSChunk4*)mem_fixed[CHANNEL_IN]->getFirstElement();
                 if ((item->size != (globals.word_size)) || (item->channels != 1))
                   break;
                 if (item->next->isEndOfList())
                   break;
                 item = (CMSChunk4*)item->next;
                 if ((item->size != (globals.word_size)) || (item->channels != 1))
                   break;
                 if (item->next->isEndOfList())
                   break;
                 item = (CMSChunk4*)item->next;
                 if ((item->size > (globals.word_size)) || (item->channels != 1))
                   break;
                 if (!item->next->isEndOfList())
                   break;
                 mem_fixed[CHANNEL_IN]->removeElement(item);
                 item->specialPos = POS_PRIVILEGES;
                 special[CHANNEL_IN]->add(item);
                 if (!mem_fixed[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!fpages[CHANNEL_IN]->isEmpty())
                   break;
                 if (fpages[CHANNEL_OUT]->isEmpty())
                   break;
                 item = (CMSChunk4*)fpages[CHANNEL_OUT]->getFirstElement();
                 if (item->channels != 2)
                   break;
                 if (!item->next->isEndOfList())
                   break;
                 if (!strings[CHANNEL_IN]->isEmpty() || !strings[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!mem_variable[CHANNEL_IN]->isEmpty() || !mem_variable[CHANNEL_OUT]->isEmpty())
                   break;
                 isOk = true;
               } while (0);
               
            if (!isOk)
              panic("malformed pagefault message; use 'in word, in word, in {word,int,char,short,long}, out fpage' as a signature");
            break;

          case 3 : 
            do {
                 isOk = false;
                 if (mem_fixed[CHANNEL_IN]->isEmpty())
                   break;
                 item = (CMSChunk4*)mem_fixed[CHANNEL_IN]->getFirstElement();
                 if ((item->size != 8) || (item->channels != 1))
                   break;
                 if (!item->next->isEndOfList())
                   break;
                 if (!mem_fixed[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!fpages[CHANNEL_IN]->isEmpty())
                   break;
                 if (!fpages[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!strings[CHANNEL_IN]->isEmpty() || !strings[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!mem_variable[CHANNEL_IN]->isEmpty() || !mem_variable[CHANNEL_OUT]->isEmpty())
                   break;
                 isOk = true;
               } while (0);
               
            if (!isOk)
              panic("malformed preemption message; use 'in long long' as a signature");
            break;

          case 4 : 
          case 5 :
            do {
                 isOk = false;
                 if (mem_fixed[CHANNEL_IN]->isEmpty())
                   break;
                 item = (CMSChunk4*)mem_fixed[CHANNEL_IN]->getFirstElement();
                 if ((item->size < globals.word_size) || (item->channels != 3))
                   break;
                 if (!item->next->isEndOfList())
                   break;
                 if (mem_fixed[CHANNEL_OUT]->isEmpty())
                   break;
                 item = (CMSChunk4*)mem_fixed[CHANNEL_OUT]->getFirstElement();
                 if ((item->size < globals.word_size) || (item->channels != 3))
                   break;
                 if (!item->next->isEndOfList())
                   break;
                 if (!fpages[CHANNEL_IN]->isEmpty())
                   break;
                 if (!fpages[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!strings[CHANNEL_IN]->isEmpty() || !strings[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!mem_variable[CHANNEL_IN]->isEmpty() || !mem_variable[CHANNEL_OUT]->isEmpty())
                   break;
                 isOk = true;
               } while (0);
               
            if (!isOk)
              panic("malformed preemption message; use 'inout <struct>' as a signature");
            break;
          
          case 6 : 
            do {
                 isOk = false;
                 if (mem_fixed[CHANNEL_IN]->isEmpty())
                   break;
                 item = (CMSChunk4*)mem_fixed[CHANNEL_IN]->getFirstElement();
                 if ((item->size != globals.word_size) || (item->channels != 1))
                   break;
                 if (item->next->isEndOfList())
                   break;
                 item = (CMSChunk4*)item->next;
                 if ((item->size != globals.word_size) || (item->channels != 1))
                   break;
                 if (!mem_fixed[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!fpages[CHANNEL_IN]->isEmpty())
                   break;
                 if (fpages[CHANNEL_OUT]->isEmpty())
                   break;
                 item = (CMSChunk4*)fpages[CHANNEL_OUT]->getFirstElement();
                 if (item->channels != 2)
                   break;
                 if (!item->next->isEndOfList())
                   break;
                 if (!strings[CHANNEL_IN]->isEmpty() || !strings[CHANNEL_OUT]->isEmpty())
                   break;
                 if (!mem_variable[CHANNEL_IN]->isEmpty() || !mem_variable[CHANNEL_OUT]->isEmpty())
                   break;
                 isOk = true;
               } while (0);
               
            if (!isOk)
              panic("malformed sigma0 rpc message; use 'in word, in word, out fpage' as a signature");
            break;
	    
          case 8 : 
            do {
	      
		isOk = false;
		if (mem_fixed[CHANNEL_IN]->isEmpty())
		    break;
		item = (CMSChunk4*)mem_fixed[CHANNEL_IN]->getFirstElement();
		if ((item->size != (globals.word_size)) || (item->channels != 1))
		  break;
		if (item->next->isEndOfList())
		  break;
		item = (CMSChunk4*)item->next;
		if ((item->size != (globals.word_size)) || (item->channels != 1))
		    break;
		if (item->next->isEndOfList())
		    break;
		item = (CMSChunk4*)item->next;
		if ((item->size > (globals.word_size)) || (item->channels != 1))
		    break;
		if (!item->next->isEndOfList())
		    break;
		mem_fixed[CHANNEL_IN]->removeElement(item);
		item->specialPos = POS_PRIVILEGES;
		special[CHANNEL_IN]->add(item);
		if (!mem_fixed[CHANNEL_OUT]->isEmpty())
		    break;
		if (!fpages[CHANNEL_IN]->isEmpty())
		    break;
		if (fpages[CHANNEL_OUT]->isEmpty())
		    break;
		item = (CMSChunk4*)fpages[CHANNEL_OUT]->getFirstElement();
		if (item->channels != 2)
		    break;
		if (!item->next->isEndOfList())
		    break;
		if (!strings[CHANNEL_IN]->isEmpty() || !strings[CHANNEL_OUT]->isEmpty())
		    break;
		if (!mem_variable[CHANNEL_IN]->isEmpty() || !mem_variable[CHANNEL_OUT]->isEmpty())
		    break;
		isOk = true;
	    } while (0);
            if (!isOk)
              panic("malformed iopagefault message; use 'in long, in long, in {char,short,long}, out fpage' as a signature");
            break;
	    
          
          default :
            warning("unknown kernel message id: %d", fid);
        }
    }
  
  /* Add message tags to every channel */

  for (int i=0;i<numChannels;i++)
    if ((i!=CHANNEL_OUT) || (!(options&OPTION_ONEWAY)))
      {
        CMSChunk4 *tagChunk = new CMSChunk4(++chunkID, bitsPerWord/8, "_msgtag", mwType, mwType, CHANNEL_MASK(i), bitsPerWord/8, CHUNK_PINNED);
        reg_fixed[i]->add(tagChunk);
      } 

  dprintln("Before optimization");
  channelDump();  

  for (int i=0;i<numChannels;i++)
    {
      /* Find out how many message registers are occupied by typed elements.
         MR0 always holds the MsgTag. */
  
      int availableMRs = 63;
      bool needMemoryMessage = false;
      dprintln("# Optimizing channel %d", i);

      /* At first, we assume that a memory message is required */
  
      if (!mem_fixed[i]->isEmpty() || !mem_variable[i]->isEmpty())
        {
          needMemoryMessage = true;
          availableMRs -= 2;
        }
    
      /* Typed elements occupy 2 MRs each and cannot be moved */
    
      forAllMS(strings[i], availableMRs -= 2);
      forAllMS(fpages[i], availableMRs -= 2);
      
      if (availableMRs<0)
        panic("More than 64 MRs used!");
        
      dprintln("%d MRs available", availableMRs);
        
      /* Try to move fixed-size elements from the memory message to MRs */
      
      bool madeProgress;
      int maxBytes = availableMRs * (bitsPerWord/8);
      int remainingBytes = maxBytes;
      CMSList *processed = new CMSList();
      
      do {
           madeProgress = false;
           dprintln("Have %d bytes left", remainingBytes);
           while (!mem_fixed[i]->isEmpty())
             {
               CMSChunk4 *chunk = (CMSChunk4*)mem_fixed[i]->removeFirstElement();
               dprintln("Trying chunk %s (%d bytes, align %d)", chunk->name, chunk->size, chunk->alignment);
               if (chunk->size <= remainingBytes)
                 {
                   reg_fixed[i]->addWithPrio(chunk, alignmentPrio(chunk->size));
                   int bytesCurrentlyUsed = layoutMessageRegisters(i);
                   if (bytesCurrentlyUsed <= maxBytes)
                     {
                       madeProgress = true;
                       channelDump();
                      
                       /* The memory message may not be necessary any more;
                          we release it, freeing up two MRs */
                      
                       if (needMemoryMessage && mem_fixed[i]->isEmpty() && mem_variable[i]->isEmpty())
                         {
                           needMemoryMessage = false;
                           availableMRs += 2;
                           maxBytes += 2 * (bitsPerWord/8);
                           dprintln("Memory message no longer needed");
                         }

                       remainingBytes = maxBytes - bytesCurrentlyUsed;
                       dprintln("Now have %d bytes left", remainingBytes);
                     } else {
                              reg_fixed[i]->removeElement(chunk);
                              processed->add(chunk);
                            }  
                 } else processed->add(chunk);
             }    
           mem_fixed[i]->merge(processed);
         } while (madeProgress);

      /* Try to move variable-size elements to the message registers, if there
         is enough space left */

      while (remainingBytes%mwAlignment) remainingBytes--;
      int oldRemainingBytes = remainingBytes;
         
      do {
           madeProgress = false;
            
           forAllMS(mem_variable[i], 
             {
               CMSChunk4 *chunk = (CMSChunk4*)item;
               if ((chunk->size!=UNKNOWN) && (chunk->size<=remainingBytes))
                 {
                   dprintln("Moving variable chunk %s (%d bytes, align %d) from mem to reg", chunk->name, chunk->size, chunk->alignment);
                   mem_variable[i]->removeElement(chunk);
                   reg_variable[i]->add(chunk);
                   remainingBytes -= chunk->size;
                   item = mem_variable[i]->getFirstElement();
                   while (remainingBytes%mwAlignment) remainingBytes--;
                   channelDump();
                   madeProgress = true;
                 }
             }
           );
         } while (madeProgress);
         
      if (remainingBytes<oldRemainingBytes)
        {
          reg_fixed[i]->addWithPrio(
            new CMSChunk4(++chunkID, (oldRemainingBytes-remainingBytes), "_varbuf", NULL, NULL, CHANNEL_MASK(i), mwAlignment, CHUNK_XFER_COPY|CHUNK_FORCE_ARRAY), 99
          );
      
          int bytesCurrentlyUsed = layoutMessageRegisters(i);
          if (bytesCurrentlyUsed > maxBytes)
            panic("MR overflow (caused by varbuf)");
        }
          
      /* Layout the rest of the buffer */

      int memVarbufSize = 0;
      forAllMS(mem_variable[i], 
        {
          CMSChunk4 *chunk = (CMSChunk4*)item;
          int thisChunkSize = chunk->size;
          
          chunk->offset = UNKNOWN;
          while (thisChunkSize%globals.word_size) thisChunkSize++;
          memVarbufSize += thisChunkSize;
        }
      );

      if (memVarbufSize>0)
        mem_fixed[i]->addWithPrio(
          new CMSChunk4(++chunkID, memVarbufSize, "_varbuf", NULL, NULL, CHANNEL_MASK(i), mwAlignment, 0), 99
        );

      int offset = 0;
      forAllMS(mem_fixed[i], 
        {
          CMSChunk4 *chunk = (CMSChunk4*)item;
          assert(chunk->alignment>0);
          while (offset % (chunk->alignment)) offset++;
          chunk->offset = offset;
          offset += chunk->size;
        }
      );
        
      offset = 0;
      forAllMS(fpages[i], ((CMSChunk4*)item)->offset = offset++);
    }
    
  /* Assign buffer numbers. We first build a list of all string buffers,
     sorted IN..INOUT..OUT..TEMP */
  
  CMSList *buffers = new CMSList();
  forAllMS(strings[CHANNEL_IN],
    {
      CMSChunk4 *chunk = (CMSChunk4*)item;
      int prio;
      if (chunk->flags & CHUNK_XFER_COPY)
        {
          if (HAS_OUT(chunk->channels))
            prio = 2;
            else prio = 1;
        } else prio = 4;
      buffers->addWithPrio(new CMSChunk4(chunk->chunkID, 1, NULL, NULL, NULL, chunk->channels, 1, 0), prio);
    }
  );
  for (int i=CHANNEL_OUT;i<numChannels;i++)
    forAllMS(strings[i],
      {
        CMSChunk4 *chunk = (CMSChunk4*)item;
        if (!HAS_IN(chunk->channels))
          {
            int prio = (chunk->flags & CHUNK_XFER_COPY) ? 3 : 4;
            buffers->addWithPrio(new CMSChunk4(chunk->chunkID, 1, NULL, NULL, NULL, chunk->channels, 1, 0), prio);
          }
      }
    );
    
  /* Assign the numbers sequentially */
    
  int index = 0;
  forAllMS(buffers,
    ((CMSChunk4*)item)->bufferIndex = index++
  );
  
  /* Transfer the numbers back to the original chunks */
  
  for (int i=0;i<numChannels;i++)
    {
      CMSList *newList = new CMSList();
      while (!strings[i]->isEmpty())
        { 
          CMSChunk4 *chunk = (CMSChunk4*)strings[i]->removeFirstElement();
          CMSChunk4 *id = findChunk(buffers, chunk->chunkID);
          assert(id);
          chunk->bufferIndex = id->bufferIndex;
          newList->addWithPrio(chunk, id->bufferIndex);
        }

      int offset = 0;
      forAllMS(newList, ((CMSChunk4*)item)->offset = offset++);
      strings[i] = newList;
    }
}

