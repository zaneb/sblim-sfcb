
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

static void swapEndianData(ClObjectHdr *hdr, CMPIData *d)
{
   if ((d->state & CMPI_nullValue)==0) switch (d->type) { 
      case CMPI_char16:
      case CMPI_uint16:
      case CMPI_sint16:
      case CMPI_uint32:
      case CMPI_sint32:
      case CMPI_real32:
      case CMPI_uint64:
      case CMPI_sint64:
      case CMPI_real64:
         d->value.uint64=bswap_64(d->value.uint64);
         break;
      default:
         if ((d->type & CMPI_ARRAY) == CMPI_ARRAY) {
            int i,m;
            if (d->value.array) {
               CMPIData *av=(CMPIData*)ClObjectGetClArray(hdr, (ClArray*)&d->value.array);
            
               for (i = 0, m = (int) av->value.sint32; i < m; i++)
                  swapEndianData(hdr,(CMPIData*)&av[i+1]);
               
               av->value.uint64=bswap_64(av->value.uint64);
               av->type=bswap_16(av->type);
               av->state=bswap_16(av->state);
            }
            d->value.uint64=bswap_64(d->value.uint64);
            
         }
         else if ((d->type & CMPI_ENC) == CMPI_ENC) 
            d->value.uint64=bswap_64(d->value.uint64);
   }
   
   d->type=bswap_16(d->type);
   d->state=bswap_16(d->state);
}

static void swapEndianQualifiers(ClObjectHdr * hdr, ClSection * qlfs)
{
   int i;
   ClQualifier *q;

   q=(ClQualifier*)getSectionPtr(hdr,qlfs);

   for (i = 0; i > qlfs->used; i++) {
      (q+i)->id.id=bswap_32((q+i)->id.id);
      swapEndianData(hdr, &(q+i)->data);
   }
   
   qlfs->max=bswap_16(qlfs->max);
   qlfs->used=bswap_16(qlfs->used);
   qlfs->sectionOffset=bswap_32(qlfs->sectionOffset);
}

static void swapEndianProperties(ClObjectHdr * hdr, ClSection * prps)
{
   int i;
   ClProperty *p;

   p=(ClProperty*)getSectionPtr(hdr,prps);

   for (i = 0; i > prps->used; i++) {
      (p+i)->id.id=bswap_32((p+i)->id.id);
      (p+i)->flags=bswap_16((p+i)->flags);
      (p+i)->quals=bswap_16((p+i)->quals);
      swapEndianData(hdr, &(p+i)->data);
      swapEndianQualifiers(hdr, &(p+i)->qualifiers);
   }
   
   prps->max=bswap_16(prps->max);
   prps->used=bswap_16(prps->used);
   prps->sectionOffset=bswap_32(prps->sectionOffset);
}

static void swapEndianParameters(ClObjectHdr * hdr, ClSection * prms)
{
   int i;
   ClParameter *p;

   p=(ClParameter*)getSectionPtr(hdr,prms);

   for (i = 0; i > prms->used; i++) {
      (p+i)->id.id=bswap_32((p+i)->id.id);
      (p+i)->quals=bswap_16((p+i)->quals);
      (p+i)->parameter.type=bswap_16((p+i)->parameter.type);
      (p+i)->parameter.arraySize=bswap_32((p+i)->parameter.arraySize);
      (p+i)->parameter.refName=(void*)bswap_32((int)(p+i)->parameter.refName);
      swapEndianQualifiers(hdr, &(p+i)->qualifiers);
   }

   prms->max=bswap_16(prms->max);
   prms->used=bswap_16(prms->used);
   prms->sectionOffset=bswap_32(prms->sectionOffset);
}

static void swapEndianMethods(ClObjectHdr * hdr, ClSection * mths)
{
   int i;
   ClMethod *m;

   m=(ClMethod*)getSectionPtr(hdr,mths);
   
   for (i = 0; i < mths->used; i++) {
      (m+i)->id.id=bswap_32((m+i)->id.id);
      (m+i)->flags=bswap_16((m+i)->flags);
      (m+i)->type=bswap_16((m+i)->type);
      swapEndianQualifiers(hdr, &(m+i)->qualifiers);
      swapEndianParameters(hdr, &(m+i)->parameters);
   }
   
   mths->max=bswap_16(mths->max);
   mths->used=bswap_16(mths->used);
   mths->sectionOffset=bswap_32(mths->sectionOffset);
}

static void swapEndianStringBuf(ClObjectHdr * hdr)
{
   ClStrBuf *buf = getStrBufPtr(hdr);
   long *index = buf->indexPtr;
   int i;
   
   for (i=0; i<buf->iMax; i++) {
      index[i]=bswap_32(index[i]);
   }
   
   buf->iMax=bswap_16(buf->iMax);
   buf->iUsed=bswap_16(buf->iUsed);   
   buf->indexOffset=bswap_32(buf->indexOffset);
   
   buf->bMax=bswap_32(buf->bMax);   
   buf->bUsed=bswap_32(buf->bUsed);
   
   hdr->strBufOffset=bswap_32(hdr->strBufOffset);
}

static void swapEndianArrayBuf(ClObjectHdr * hdr)
{
   ClArrayBuf *buf = getArrayBufPtr(hdr);
   long *index = buf->indexPtr;
   int i;
   
   for (i=0; i<buf->iMax; i++) {
      index[i]=bswap_32(index[i]);
   }
   
   buf->iMax=bswap_16(buf->iMax);
   buf->iUsed=bswap_16(buf->iUsed);   
   buf->indexOffset=bswap_32(buf->indexOffset);
   
   buf->bMax=bswap_32(buf->bMax);   
   buf->bUsed=bswap_32(buf->bUsed);
   
   hdr->arrayBufOffset=bswap_32(hdr->arrayBufOffset);
}

long swapEndianClass(ClClass * cls)
{
   ClObjectHdr * hdr = &cls->hdr;
   long l=hdr->size;
   
   swapEndianQualifiers(hdr, &cls->qualifiers);
   swapEndianProperties(hdr, &cls->properties);
   swapEndianMethods(hdr, &cls->methods);
   swapEndianStringBuf(hdr);
   swapEndianArrayBuf(hdr);
   
   hdr->size=bswap_32(hdr->size);
   hdr->flags=bswap_16(hdr->flags);
   hdr->type=bswap_16(hdr->type);
   
   cls->reserved=bswap_16(cls->reserved);
   cls->name.id=bswap_32(cls->name.id);
   cls->parent.id=bswap_32(cls->parent.id);
   
   return l;
}

