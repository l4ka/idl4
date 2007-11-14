#include "arch/x0.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)
#define dprint(a...) do { if (debug_mode&DEBUG_MARSHAL) print(a); } while (0)

int CMSConnectionX::getVarbufSize(int channel)

{
  int varbufSize = 0;
  forAllMS(vars[channel], 
    { 
      int thisSize = ((CMSChunkX*)item)->size;
      assert((thisSize%4)==0);
      varbufSize += thisSize;
    }
  );  
  return varbufSize;
}

void CMSConnectionX::addServerDopeExtensions()

{
}

void CMSConnectionIX::addServerDopeExtensions()

{
  dopes[CHANNEL_IN]->addWithPrio(
    new CMSChunkX(++chunkID, "_esp", 4, 4, CHUNK_ALIGN4, 
      new CBEOpaqueType("int", 4, 4), 
      NULL, CHANNEL_MASK(CHANNEL_IN), CHUNK_ALLOC_SERVER | CHUNK_PINNED, -2), 
    PRIO_DOPE
  );
  dopes[CHANNEL_IN]->addWithPrio(
    new CMSChunkX(++chunkID, "_ebp", 4, 4, CHUNK_ALIGN4, 
      new CBEOpaqueType("int", 4, 4), 
      NULL, CHANNEL_MASK(CHANNEL_IN), CHUNK_ALLOC_SERVER | CHUNK_PINNED, -1), 
    PRIO_DOPE
  );
  dopes[CHANNEL_IN]->addWithPrio(
    new CMSChunkX(++chunkID, "_caller", 4, 4, CHUNK_ALIGN4, 
      new CBEOpaqueType("l4_threadid_t", 4, 4), 
      NULL, CHANNEL_MASK(CHANNEL_IN), CHUNK_PINNED, 2), 
    PRIO_DOPE
  );
}

void assignTempBuf(CMSChunkX *chunk)

{
  /* Assigns temporary buffers to the components of a shared register */
  
  if (chunk->isShared())
    {
      CMSSharedChunkX *schunk = (CMSSharedChunkX*)chunk;
      assignTempBuf(schunk->chunk1);
      assignTempBuf(schunk->chunk2);
    } else {
             if (HAS_OUT(chunk->channels) || !chunk->type->isScalar())
               chunk->temporaryBuffer = new CASTIdentifier(mprintf("_buf_%s", chunk->name));
           }
}

void CMSConnectionX::optimize()

{
  CBEType *mwType = msFactory->getMachineWordType();
  int regsUsed[MAX_CHANNELS];
  
  dprintln("Before optimization");
  channelDump();  

  for (int i=0;i<numChannels;i++)
    {
      dopes[i]->addWithPrio(new CMSChunkX(++chunkID, "_rcv_window", 4, 4, CHUNK_ALIGN4, new CBEOpaqueType("l4_fpage_t", 4, 4), NULL, CHANNEL_MASK(i), CHUNK_ALLOC_CLIENT | CHUNK_ALLOC_SERVER), PRIO_DOPE);
      dopes[i]->addWithPrio(new CMSChunkX(++chunkID, "_size_dope", 4, 4, CHUNK_ALIGN4, new CBEOpaqueType("idl4_msgdope_t", 4, 4), NULL, CHANNEL_MASK(i), CHUNK_ALLOC_CLIENT | CHUNK_ALLOC_SERVER), PRIO_DOPE);
      dopes[i]->addWithPrio(new CMSChunkX(++chunkID, "_send_dope", 4, 4, CHUNK_ALIGN4, new CBEOpaqueType("idl4_msgdope_t", 4, 4), NULL, CHANNEL_MASK(i), CHUNK_ALLOC_CLIENT | CHUNK_ALLOC_SERVER), PRIO_DOPE);
    }  

  addServerDopeExtensions();

  for (int i=0;i<numChannels;i++)
    {
      if (!vars[i]->isEmpty())
        dwords[i]->addWithPrio(new CMSChunkX(++chunkID, "_varbuf", getVarbufSize(i), getVarbufSize(i), CHUNK_ALIGN4, NULL, NULL, i, CHUNK_ALLOC_CLIENT | CHUNK_ALLOC_SERVER | CHUNK_XFER_COPY), PRIO_VARIABLE);
    }    

  /* Move map chunks into registers, if any */
  
  for (int i=0;i<numChannels;i++)
    {
      regsUsed[i] = 0;
      
      if (i==0)
        {
          CMSChunkX *fidChunk = new CMSChunkX(0, "__fid", 4, 4, 4, mwType, NULL, 0, CHUNK_ALLOC_CLIENT|CHUNK_ALLOC_SERVER|CHUNK_XFER_FIXED|CHUNK_XFER_COPY|CHUNK_PINNED, (numRegs) ? (numRegs - 1) : 0);
          fidChunk->setCachedInput(0, new CASTIntegerConstant(fid + (iid<<FID_SIZE)));
          if (numRegs)
            {
              registers[i]->addWithPrio(fidChunk, PRIO_ID);
              regsUsed[i]++;
            } else dwords[i]->addWithPrio(fidChunk, PRIO_DOPE);  
        }
        
      if (i>0 && !(options&OPTION_ONEWAY) && numRegs)
        regsUsed[i]++;
        
      bool needPad = false;
      int pinIndex = 0;
      while ((dwords[i]->getFirstElementWithPrio(PRIO_MAP)) && ((numRegs-regsUsed[i])>=2))
        {
          CMSChunkX *firstMapElement = (CMSChunkX*)dwords[i]->getFirstElementWithPrio(PRIO_MAP);
      
          dwords[i]->removeElement(firstMapElement);
          firstMapElement->requestedIndex = pinIndex;
          firstMapElement->flags |= CHUNK_PINNED;
          pinIndex += 2;
          registers[i]->addWithPrio(firstMapElement, PRIO_MAP);
          regsUsed[i] += 2;
          needPad = true;

          dprintln("Moved map chunk %d to register", firstMapElement->chunkID);
          channelDump();  
        }
        
      /* Map chunks must be followed by two padding words */
        
      if (needPad)
        {
          int padChunk = ++chunkID;
        
          if (regsUsed[i]<numRegs)
            {
              assert((numRegs-regsUsed[i])>=2); /* odd number of regs doesn't work */
              registers[i]->addWithPrio(new CMSChunkX(padChunk, "__mapPad", 8, 8, 8, new CBEOpaqueType("idl4_mappad_t", 8, 8), NULL, 0, CHUNK_ALLOC_CLIENT|CHUNK_ALLOC_SERVER|CHUNK_XFER_FIXED|CHUNK_XFER_COPY|CHUNK_INIT_ZERO|CHUNK_PINNED, pinIndex), PRIO_PAD);
            } else {
                     if (dwords[i]->hasPrioOrAbove(PRIO_MAP + 1))
                       dwords[i]->addWithPrio(new CMSChunkX(padChunk, "__mapPad", 8, 8, 8, new CBEOpaqueType("idl4_mappad_t", 8, 8), NULL, 0, CHUNK_ALLOC_CLIENT|CHUNK_ALLOC_SERVER|CHUNK_XFER_FIXED|CHUNK_XFER_COPY|CHUNK_INIT_ZERO|CHUNK_PINNED, 0), PRIO_PAD);
                   }  

          dprintln("Added map padding");
          channelDump();  
        }  
    }    

  /* Determine the minimal alignment of the output buffer. This is important
     because IN/OUT overlapping must not cause the output buffer to be
     misaligned */

  int outAlign = 4;
  forAllMS(dwords[CHANNEL_OUT],
    if (((CMSChunkX*)item)->type)
      if (((CMSChunkX*)item)->alignment>outAlign)
        outAlign = ((CMSChunkX*)item)->alignment;
  );

  /* A special optimization for the IN and OUT channels. We try to move
     INOUT chunks into registers first, because we might get to unlock
     the package */

  while ((regsUsed[CHANNEL_IN]<numRegs) && (regsUsed[CHANNEL_OUT]<numRegs))
    {
      CMSChunkX *inChunk = (CMSChunkX*)dwords[CHANNEL_IN]->getFirstElementWithPrio(PRIO_INOUT_FIXED);
      if (!inChunk)
        break;

      while ((!inChunk->isEndOfList()) && (inChunk->size>(bitsPerWord/8)) && 
             (inChunk->prio == PRIO_INOUT_FIXED))
        inChunk = (CMSChunkX*)inChunk->next;     

      if (inChunk->isEndOfList() || (inChunk->prio != PRIO_INOUT_FIXED))
        break;  
        
      CMSChunkX *outChunk = (CMSChunkX*)dwords[CHANNEL_OUT]->getFirstElementWithPrio(PRIO_INOUT_FIXED);
      if (!outChunk)
        break;
        
      while ((!outChunk->isEndOfList()) && (outChunk->chunkID!=inChunk->chunkID))
        outChunk = (CMSChunkX*)outChunk->next;
        
      if (outChunk->isEndOfList())
        break;

      dwords[CHANNEL_IN]->removeElement(inChunk);
      registers[CHANNEL_IN]->addWithPrio(inChunk, PRIO_INOUT_FIXED);
      regsUsed[CHANNEL_IN]+=1;
      dwords[CHANNEL_OUT]->removeElement(outChunk);
      registers[CHANNEL_OUT]->addWithPrio(outChunk, PRIO_INOUT_FIXED);  
      regsUsed[CHANNEL_OUT]+=1;

      dprintln("Moved inout chunks (%d,%d) to registers", inChunk->chunkID, outChunk->chunkID);
      channelDump();  
    }
    
  /* Now we try to move _anything_ to the registers, provided that it fits.
     Note that we do not allow chunks to span registers (simplicity). */
     
  for (int i=0;i<numChannels;i++)
    {
      CMSChunkX *iterator = (CMSChunkX*)dwords[i]->getFirstElement();
      CMSList *candidates = new CMSList();
      int regsLeft = numRegs - regsUsed[i];
      int capacity = bitsPerWord/8;

      while (!iterator->isEndOfList())
        if ((iterator->channels&(1<<i)) && (iterator->size<=(bitsPerWord/8)))
          {
            /* First see if we can share another register */
            
            CMSChunkX *cand = (CMSChunkX*)candidates->getFirstElement();
            bool canShare = false;
            while (!cand->isEndOfList())
              if ((capacity-cand->size)>=iterator->size)
                {
                  canShare = true;
                  break;
                } else cand = (CMSChunkX*)cand->next;

            /* Currently, we only allow sharing in the input channel */
           
            if (i>0)
              canShare = false;                
                
            if (canShare)
              {
                /* A register with enough free space was found */
              
                candidates->removeElement(cand);
                dwords[i]->removeElement(iterator);
                iterator->flags &= (~CHUNK_ALLOC_SERVER); 
                candidates->add(new CMSSharedChunkX(iterator, cand));
                dprintln("Added chunk %d to register in channel %d", iterator->chunkID, i);
                iterator = (CMSChunkX*)dwords[i]->getFirstElement();
              } else {
                       /* Sharing is not possible. If we have a free
                          register left, we use it */
              
                       if (regsLeft)
                         {
                           dwords[i]->removeElement(iterator);
                           iterator->flags &= (~CHUNK_ALLOC_SERVER); 
                           candidates->add(iterator);
                           regsLeft--;
                           dprintln("Moved chunk %d to register in channel %d", iterator->chunkID, i);
                           channelDump();  
                           iterator = (CMSChunkX*)dwords[i]->getFirstElement();
                         } else iterator = (CMSChunkX*)iterator->next;
                     }   
         } else iterator = (CMSChunkX*)iterator->next;

      if (!candidates->isEmpty())
        {
          while (!candidates->isEmpty())
            registers[i]->addWithPrio(candidates->removeFirstElement(), PRIO_OUT_FIXED);
          dprintln("Merged registers in channel %d", i);
          channelDump();
        }  
    }      
    
  /* We try to find a suitable padding offset for the server output buffer.
     There should be as much overlap as possible */
     
  CMSChunkX *inIterator = (CMSChunkX*)dwords[CHANNEL_IN]->getFirstElement();
  int plainInSize = 0;
  while ((!inIterator->isEndOfList()) && (!(inIterator->channels&CHANNEL_MASK(CHANNEL_OUT))))
    {
      plainInSize += inIterator->size;
      inIterator = (CMSChunkX*)inIterator->next;
    }  

  int svrOutputOffset = 0;  
  if (!inIterator->isEndOfList())
    {
      /* We found an INOUT element in CHANNEL_IN! To apply the optimization,
         the following elements must all be present in CHANNEL_OUT, in exactly
         the same sequence */
      
      CMSChunkX *outIterator = (CMSChunkX*)dwords[CHANNEL_OUT]->getFirstElement();
      int commonSize = 0;
      
      while ((!inIterator->isEndOfList()) && (!outIterator->isEndOfList()) &&
             (inIterator->chunkID==outIterator->chunkID))
        {
          commonSize += inIterator->size;
          inIterator = (CMSChunkX*)inIterator->next;
          outIterator = (CMSChunkX*)outIterator->next;
        }
        
      /* Additionally, if there are any output strings, we cannot use the
         primary dope because we would need to overwrite the size dope */
        
      if ((inIterator->isEndOfList()) && 
          ((plainInSize%outAlign)==0) &&
          ((plainInSize>0) || !hasStringsInChannel(CHANNEL_OUT)))
        {
          /* Apply the optimization */
        
          svrOutputOffset = plainInSize;
          dprintln("Overlap: yes; svrOutputOffset = %d", svrOutputOffset);
        } else {
                 /* Cannot apply the optimization, because the INOUT elements
                    are not in sequence (or variable length elements are
                    present). The output buffer must be moved to the end
                    of the input buffer. */
        
                 int remainingInbufSize = 0;
                 while (!inIterator->isEndOfList())
                   {
                     remainingInbufSize += inIterator->size;
                     inIterator = (CMSChunkX*)inIterator->next;
                   }

                 dprintln("Overlap: no; plainInSize %d commonSize %d remainingInbufSize %d", plainInSize, commonSize, remainingInbufSize);
                 svrOutputOffset = plainInSize + commonSize + remainingInbufSize;
                 svrOutputOffset += getVarbufSize(CHANNEL_IN);

                 while ((svrOutputOffset%outAlign)!=0) 
                   svrOutputOffset++;
               }  
    } else {
             /* No INOUT elements were found in CHANNEL_IN. We move the output
                buffer to the end of the input buffer. */
             
             svrOutputOffset = plainInSize;
             svrOutputOffset += (3+numRegs)*4;
             svrOutputOffset += getVarbufSize(CHANNEL_IN);  

             while ((svrOutputOffset%outAlign)!=0) 
               svrOutputOffset++;

             dprintln("Overlap: no - missing inouts; svrOutputOffset = %d", svrOutputOffset);
           }

  /* On the server side, we must make sure that:
       a) INOUT strings have the same position in all channels
       b) IN and OUT strings do not overlap
       c) Temporary string buffers are properly aligned */

  int pureInStrings = 0, inoutStrings = 0;
  forAllMS(strings[CHANNEL_IN],
    {
      CMSChunkX *chunk = (CMSChunkX*)item;
      if (chunk->flags&CHUNK_XFER_COPY)
        {
          if (HAS_OUT(chunk->channels))
            inoutStrings++;
            else pureInStrings++;
        }
    }
  );

  int outStringsInChannel[MAX_CHANNELS];
  int maxOutStrings = 0;
  for (int i=CHANNEL_OUT;i<numChannels;i++)
    {
      outStringsInChannel[i] = 0;
      forAllMS(strings[i],
        {
          CMSChunkX *chunk = (CMSChunkX*)item;
          if (chunk->flags&CHUNK_XFER_COPY)
            if (!HAS_IN(chunk->channels))
              outStringsInChannel[i]++;
        }
      );
      if (outStringsInChannel[i] > maxOutStrings)
        maxOutStrings = outStringsInChannel[i];
    }   
  
  int padNo = 0;
  for (int i=CHANNEL_OUT;i<numChannels;i++)
    {
      for (int j=0;j<pureInStrings;j++)
        strings[i]->addWithPrio(new CMSChunkX(++chunkID, mprintf("_strPad%d", ++padNo), SIZEOF_STRINGDOPE, 0, CHUNK_ALIGN4, NULL, NULL, 1<<i, CHUNK_ALLOC_SERVER), PRIO_IN_PAD_STRING);
      if (i != CHANNEL_OUT)
        for (int j=0;j<inoutStrings;j++)
          strings[i]->addWithPrio(new CMSChunkX(++chunkID, mprintf("_strPad%d", ++padNo), SIZEOF_STRINGDOPE, 0, CHUNK_ALIGN4, NULL, NULL, 1<<i, CHUNK_ALLOC_SERVER), PRIO_INOUT_PAD_STRING);
      for (int j=outStringsInChannel[i];j<maxOutStrings;j++)
        strings[i]->addWithPrio(new CMSChunkX(++chunkID, mprintf("_strPad%d", ++padNo), SIZEOF_STRINGDOPE, 0, CHUNK_ALIGN4, NULL, NULL, 1<<i, CHUNK_ALLOC_SERVER), PRIO_OUT_PAD_STRING);
    }

  /* Now we freeze the positions of the elements */
  
  for (int i=0;i<numChannels;i++)
    {
      int currentClientPos = (3+numRegs)*4;
      int currentServerPos = (3+numRegs)*4;
      int currentClientIndex = 0;
      int currentServerIndex = 0;
      
      if ((i==CHANNEL_OUT) && (svrOutputOffset))
        currentServerPos += svrOutputOffset;

      forAllMS(dwords[i],
        {
          CMSChunkX *chunk = (CMSChunkX*)item;
          int size = chunk->size;
          int alignment = chunk->alignment;

          if (chunk->flags&CHUNK_ALLOC_CLIENT)
            while (currentClientPos%alignment)
              {
                currentClientPos++;
                currentServerPos++;
              }  

          if (chunk->flags&CHUNK_ALLOC_SERVER)
            while (currentServerPos%alignment)
              {
                currentClientPos++;
                currentServerPos++;
              }  

          assert( ((!(chunk->flags&CHUNK_ALLOC_CLIENT)) || ((currentClientPos%alignment)==0)) &&
                  ((!(chunk->flags&CHUNK_ALLOC_SERVER)) || ((currentServerPos%alignment)==0)) );
          
          if (chunk->flags&CHUNK_ALLOC_CLIENT)
            {
              chunk->setClientPosition(currentClientPos, currentClientIndex++);
              currentClientPos += size;
            } else chunk->setClientPosition(UNKNOWN, UNKNOWN);
            
          if (chunk->flags&CHUNK_ALLOC_SERVER)
            {
              chunk->setServerPosition(currentServerPos, currentServerIndex++);
              currentServerPos += size;    
            } else chunk->setServerPosition(UNKNOWN, UNKNOWN);
        }      
      );

      /* Note: The dopes do _not_ start at the beginning of the message,
         because the server allocates some local variables */

      currentClientPos = 0;
      currentServerPos = -4 * service->getServerLocalVars();
      currentClientIndex = 0;
      currentServerIndex = -service->getServerLocalVars();
      
      if ((i==CHANNEL_OUT) && (svrOutputOffset))
        currentServerPos += svrOutputOffset;
        
      forAllMS(dopes[i],
        {
          CMSChunkX *chunk = (CMSChunkX*)item;
          
          if (chunk->flags&CHUNK_ALLOC_CLIENT)
            {
              while (currentClientPos%chunk->alignment)
                currentClientPos++;
              chunk->setClientPosition(currentClientPos, currentClientIndex++);
              currentClientPos+=chunk->size;
            }
          
          if (chunk->flags&CHUNK_ALLOC_SERVER)
            {
              if (chunk->flags&CHUNK_PINNED)
                {
                  chunk->setServerPosition(4*chunk->requestedIndex, chunk->requestedIndex);
                } else {
                         while (currentServerPos%chunk->alignment)
                           currentServerPos++;
                         chunk->setServerPosition(currentServerPos, currentServerIndex++);
                         currentServerPos+=chunk->size;
                       }  
            }
        }  
      );  
      
      currentClientPos = 3*4;
      currentServerPos = 3*4;
      currentClientIndex = 0;
      currentServerIndex = 0;
      
      if ((i==CHANNEL_OUT) && (svrOutputOffset))
        currentServerPos += svrOutputOffset;
        
      forAllMS(registers[i],
        {
          CMSChunkX *chunk = (CMSChunkX*)item;

          if (chunk->flags & CHUNK_PINNED)
            {
              assert(chunk->requestedIndex>=currentClientIndex);
              currentClientPos = 3*4 + chunk->requestedIndex * 4;
              currentServerPos = 3*4 + chunk->requestedIndex * 4;
              if ((i==CHANNEL_OUT) && (svrOutputOffset))
                currentServerPos += svrOutputOffset;
              currentClientIndex = currentServerIndex = chunk->requestedIndex;
            }

          chunk->setClientPosition(currentClientPos, currentClientIndex++);
          chunk->setServerPosition(currentServerPos, currentServerIndex++);
          currentClientPos+=4;
          currentServerPos+=4;
          
          /* We need a register buffer if
               1) The type is not scalar, or
               2) The register is shared and has an output channel */
        
          assignTempBuf(chunk);
        }  
      );  

      currentClientPos = 0;
      currentServerPos = 0;
      currentClientIndex = 0;
      currentServerIndex = 0;

      forAllMS(strings[i],
        {
          CMSChunkX *chunk = (CMSChunkX*)item;

          if (chunk->flags&CHUNK_ALLOC_CLIENT)
            {
              chunk->setClientPosition(currentClientPos, currentClientIndex++);
              currentClientPos += SIZEOF_STRINGDOPE;
            }
            
          if (chunk->flags&CHUNK_ALLOC_SERVER)
            {
              chunk->setServerPosition(currentServerPos, currentServerIndex++);
              currentServerPos += SIZEOF_STRINGDOPE;
            }
        }  
      );  
      
      forAllMS(vars[i], 
        {
          ((CMSChunkX*)item)->setClientPosition(UNKNOWN, UNKNOWN);
          ((CMSChunkX*)item)->setServerPosition(UNKNOWN, UNKNOWN);
        }
      );
    }  

  finalized = true;  
}

void CMSConnectionX::finalize(int strdopeOffsetInBytes)

{
  int clientStringOffset = getMaxBytesRequired(false);
  
  assert((strdopeOffsetInBytes%4)==0);
  while (clientStringOffset%4)
    clientStringOffset++;

  for (int i=0;i<numChannels;i++)
    forAllMS(strings[i],
      {
        CMSChunkX *chunk = (CMSChunkX*)item;
      
        if (chunk->flags&CHUNK_ALLOC_SERVER)
          chunk->serverOffset += strdopeOffsetInBytes;

        if (chunk->flags&CHUNK_ALLOC_CLIENT)  
          chunk->clientOffset += clientStringOffset;
      }
    );  
}
