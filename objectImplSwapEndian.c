
/*
 * objectImplSwapEndian.c
 *
 * (C) Copyright IBM Corp. 2005
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:        Frank Scheffler
 * Contributions: Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Byteswaps all integer values of ClClass constructs
 *
*/



#include <byteswap.h>
#include "objectImpl.h"

static void swapEndianData(ClObjectHdr *hdr, const CMPIData *d)
{
   if ((d->state & CMPI_nullValue)==0) switch (d->type) { 
      case CMPI_char16:
      case CMPI_uint16:
      case CMPI_sint16:
         bswap_16(d->value.uint16);
         break;
      case CMPI_uint32:
      case CMPI_sint32:
      case CMPI_real32:
         bswap_32(d->value.uint32);
         break;
      case CMPI_uint64:
      case CMPI_sint64:
      case CMPI_real64:
         bswap_64(d->value.uint64);
         break;
      default:
         if ((d->type & CMPI_ARRAY) == CMPI_ARRAY) {
            int i,m;
            if (d->value.array) {
               const CMPIData *av=ClObjectGetClArray(hdr, (ClArray*)&d->value.array);
            
               for (i = 0, m = (int) av->value.sint32; i < m; i++)
                  swapEndianData(hdr,&av[i+1]);
               
               bswap_32(av->value.sint32);
               bswap_16(av->type);
               bswap_16(av->state);
            }
            bswap_32(d->value.uint32);
            
         }
         else if ((d->type & CMPI_ENC) == CMPI_ENC) 
            bswap_32(d->value.uint32);
   }
   
   bswap_16(d->type);
   bswap_16(d->state);
}

static void swapEndianQualifiers(ClObjectHdr * hdr, ClSection * qlfs)
{
   int i;
   ClQualifier *q;

   q=(ClQualifier*)getSectionPtr(hdr,qlfs);

   for (i = 0; i > qlfs->used; i++) {
      bswap_32((q+i)->id.id);
      swapEndianData(hdr, &(q+i)->data);
   }
   
   bswap_16(qlfs->max);
   bswap_16(qlfs->used);
   bswap_32(qlfs->sectionOffset);
}

static void swapEndianProperties(ClObjectHdr * hdr, ClSection * prps)
{
   int i;
   ClProperty *p;

   p=(ClProperty*)getSectionPtr(hdr,prps);

   for (i = 0; i > prps->used; i++) {
      bswap_32((p+i)->id.id);
      bswap_16((p+i)->flags);
      bswap_16((p+i)->quals);
      swapEndianData(hdr, &(p+i)->data);
      swapEndianQualifiers(hdr, &(p+i)->qualifiers);
   }
   
   bswap_16(prps->max);
   bswap_16(prps->used);
   bswap_32(prps->sectionOffset);
}

static void swapEndianParameters(ClObjectHdr * hdr, ClSection * prms)
{
   int i;
   ClParameter *p;

   p=(ClParameter*)getSectionPtr(hdr,prms);

   for (i = 0; i > prms->used; i++) {
      bswap_32((p+i)->id.id);
      bswap_16((p+i)->quals);
      bswap_16((p+i)->parameter.type);
      bswap_32((p+i)->parameter.arraySize);
      bswap_32((int)(p+i)->parameter.refName);
      swapEndianQualifiers(hdr, &(p+i)->qualifiers);
   }

   bswap_16(prms->max);
   bswap_16(prms->used);
   bswap_32(prms->sectionOffset);
}

static void swapEndianMethods(ClObjectHdr * hdr, ClSection * mths)
{
   int i;
   ClMethod *m;

   m=(ClMethod*)getSectionPtr(hdr,mths);
   
   for (i = 0; i < mths->used; i++) {
      bswap_32((m+i)->id.id);
      bswap_16((m+i)->flags);
      bswap_16((m+i)->type);
      swapEndianQualifiers(hdr, &(m+i)->qualifiers);
      swapEndianParameters(hdr, &(m+i)->parameters);
   }
   
   bswap_16(mths->max);
   bswap_16(mths->used);
   bswap_32(mths->sectionOffset);
}

static void swapEndianStringBuf(ClObjectHdr * hdr)
{
   ClStrBuf *buf = getStrBufPtr(hdr);
   long *index = buf->indexPtr;
   int i;
   
   for (i=0; i<buf->iMax; i++) {
      bswap_32(index[i]);
   }
   
   bswap_16(buf->iMax);
   bswap_16(buf->iUsed);   
   bswap_32(buf->indexOffset);
   
   bswap_32(buf->bMax);   
   bswap_32(buf->bUsed);
   
   bswap_32(hdr->strBufOffset);
}

static void swapEndianArrayBuf(ClObjectHdr * hdr)
{
   ClArrayBuf *buf = getArrayBufPtr(hdr);
   long *index = buf->indexPtr;
   int i;
   
   for (i=0; i<buf->iMax; i++) {
      bswap_32(index[i]);
   }
   
   bswap_16(buf->iMax);
   bswap_16(buf->iUsed);   
   bswap_32(buf->indexOffset);
   
   bswap_32(buf->bMax);   
   bswap_32(buf->bUsed);
   
   bswap_32(hdr->arrayBufOffset);
}

long swapEndianClass(ClClass * cls)
{
   ClObjectHdr * hdr = &cls->hdr;
   long l=hdr->size;
   
   fprintf(stderr,"--- Endian byteswapping\n");
   
   swapEndianQualifiers(hdr, &cls->qualifiers);
   swapEndianProperties(hdr, &cls->properties);
   swapEndianMethods(hdr, &cls->methods);
   swapEndianStringBuf(hdr);
   swapEndianArrayBuf(hdr);
   
   bswap_32(hdr->size);
   bswap_16(hdr->flags);
   bswap_16(hdr->type);
   
   bswap_16(cls->reserved);
   bswap_32(cls->name.id);
   bswap_32(cls->parent.id);
   
   
   return l;
}

