
/*
 * objectImplSwapI32toP32.c
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
#include <stdio.h>
#include <stdlib.h>
#include "utilft.h"
#include <sys/utsname.h>

#ifdef SETCLPFX
#undef SETCLPFX
#endif

#include "objectImpl.h"

#define CLP32
#define SETCLPFX CLP32_

#define CLP32_CMPIType CMPIType
#define CLP32_CMPIValueState CMPIValueState
#define CLP32_CMPIValue CMPIValue

typedef struct _CLP32_CMPIData {
   CLP32_CMPIType type;
   CLP32_CMPIValueState state;
   int fillP32;
   CLP32_CMPIValue value;
} CLP32_CMPIData;

typedef struct {
   unsigned short iUsed,iMax;
   int indexOffset;
   int *indexPtr;
   unsigned int bUsed, bMax;
   int fillP32;
   CLP32_CMPIData buf[1];
} CLP32_ClArrayBuf;

#include "objectImpl.h"

static CLP32_CMPIData copyI32toP32Data(CMPIData *fd)
{
   CLP32_CMPIData td;
   
   td.value.uint64=bswap_64(fd->value.uint64);
   td.type=bswap_16(fd->type);
   td.state=bswap_16(fd->state);
   return td;
}

static int p32SizeQualifiers(ClObjectHdr * hdr, ClSection * s)
{
   long sz;

   sz = s->used * sizeof(CLP32_ClQualifier);
   return ALIGN(sz,CLALIGN);
}

static int copyI32toP32Qualifiers(int ofs, char *to, CLP32_ClSection * ts,
                          ClObjectHdr * from, ClSection * fs)
{
   ClQualifier *fq = (ClQualifier *) ClObjectGetClSection(from, fs);
   CLP32_ClQualifier *tq = (CLP32_ClQualifier*)(to + ofs);
   int i, l = fs->used * sizeof(CLP32_ClQualifier);
   
   ts->max = bswap_16(fs->max);
   ts->used = bswap_16(fs->used);

   for (i = 0; i<fs->used; i++, tq++, fq++) {
      tq->id.id = bswap_32(fq->id.id);
      tq->data = copyI32toP32Data(&fq->data);
   }
   
   ts->sectionOffset = bswap_32(ofs);
   
   return ALIGN(l,CLALIGN);
}

static int p32SizeProperties(ClObjectHdr * hdr, ClSection * s)
{
   int l;
   long sz = s->used * sizeof(CLP32_ClProperty);
   ClProperty *p = (ClProperty *) ClObjectGetClSection(hdr, s);

   for (l = s->used; l > 0; l--, p++) {
     if (p->qualifiers.used)
         sz += p32SizeQualifiers(hdr, &p->qualifiers);
   }      
   return ALIGN(sz,CLALIGN);
}

static int copyI32toP32Properties(int ofs, char *to, CLP32_ClSection * ts,
                           ClObjectHdr * from, ClSection * fs)
{
   ClProperty *fp = (ClProperty *) ClObjectGetClSection(from, fs);
   CLP32_ClProperty *tp = (CLP32_ClProperty *) (to + ofs);   
   int i, l = fs->used * sizeof(CLP32_ClProperty);
   if (l == 0) return 0;
   
   ts->max = bswap_16(fs->max);
   ts->used = bswap_16(fs->used);

   for (i = fs->used; i > 0; i--, fp++, tp++) {
      tp->id.id = bswap_32(fp->id.id);
      tp->data = copyI32toP32Data(&fp->data);
      tp->flags = bswap_16(fp->flags);     
      if (fp->qualifiers.used)
         l += copyI32toP32Qualifiers(ofs + l, to, &tp->qualifiers, from,
                             &fp->qualifiers);
   }                          

   ts->sectionOffset = bswap_32(ofs);
   return ALIGN(l,CLALIGN);
}

static int p32SizeParameters(ClObjectHdr * hdr, ClSection * s)
{
   int l;
   long sz = s->used * sizeof(CLP32_ClParameter);
   ClParameter *p = (ClParameter *) ClObjectGetClSection(hdr, s);

   for (l = s->used; l > 0; l--, p++) {
      if (p->qualifiers.used)
         sz += p32SizeQualifiers(hdr, &p->qualifiers);
   }      
   return ALIGN(sz,CLALIGN);
}

static long copyI32toP32Parameters(int ofs, char *to, CLP32_ClSection * ts,
                           ClObjectHdr * from, ClSection * fs)
{
   ClParameter *fp = (ClParameter *) ClObjectGetClSection(from, fs);
   CLP32_ClParameter *tp = (CLP32_ClParameter *) (to + ofs);
   int i,l = fs->used * sizeof(CLP32_ClParameter);
   
   if (l == 0) return 0;
   
   ts->max = bswap_16(fs->max);
   ts->used = bswap_16(fs->used);

   for (i = fs->used; i > 0; i--, fp++, tp++) {
      tp->id.id = bswap_32(fp->id.id);
      tp->quals = bswap_16(fp->quals);
      tp->parameter.type=bswap_16(fp->parameter.type);
      tp->parameter.arraySize=bswap_32(fp->parameter.arraySize);
      tp->parameter.refName=(void*)bswap_32((int)(fp->parameter.refName));
      if (fp->qualifiers.used)
         l += copyI32toP32Qualifiers(ofs + l, to, &tp->qualifiers, from, &fp->qualifiers);
   }                          

   return ALIGN(l,CLALIGN);
}

static int p32SizeMethods(ClObjectHdr * hdr, ClSection * s)
{
   int l;
   long sz = s->used * sizeof(CLP32_ClMethod);
   ClMethod *m = (ClMethod *) ClObjectGetClSection(hdr, s);

   for (l = s->used; l > 0; l--, m++) {
      if (m->qualifiers.used) 
         sz += p32SizeQualifiers(hdr, &m->qualifiers);
      if (m->parameters.used)
         sz += p32SizeParameters(hdr, &m->parameters);
   }      
   return ALIGN(sz,CLALIGN);
}

static int copyI32toP32Methods(int ofs, char *to, CLP32_ClSection * ts,
                           ClObjectHdr * from, ClSection * fs)
{
   ClMethod *fm = (ClMethod *) ClObjectGetClSection(from, fs);
   CLP32_ClMethod *tm = (CLP32_ClMethod *) (to + ofs);
   int i, l = fs->used * sizeof(CLP32_ClMethod);
   
   if (l == 0) return 0;
   
   ts->max = bswap_16(fs->max);
   ts->used = bswap_16(fs->used);

   for (i = fs->used; i > 0; i--, fm++, tm++) {
      tm->id.id = bswap_32(fm->id.id);
      tm->type = bswap_16(fm->type);
      tm->flags = bswap_16(fm->flags);
      if (fm->qualifiers.used)
         l += copyI32toP32Qualifiers(ofs + l, to, &tm->qualifiers, from,
                             &fm->qualifiers);
      if (fm->parameters.used)
         l += copyI32toP32Parameters(ofs + l, to, &tm->parameters, from,
                             &fm->parameters);
   }                          

   ts->sectionOffset = bswap_32(ofs);
   return ALIGN(l,CLALIGN);
}

static long p32SizeStringBuf(ClObjectHdr * hdr)
{
   ClStrBuf *buf;
   long sz = 0;
   if (hdr->strBufOffset == 0) return 0;

   buf = getStrBufPtr(hdr);   

   sz = sizeof(*buf) + ALIGN(buf->bUsed,4) + (buf->iUsed * sizeof(*buf->indexPtr));

   return ALIGN(sz,CLALIGN);
}

static int copyI32toP32StringBuf(int ofs, CLP32_ClObjectHdr * th, ClObjectHdr * fh)
{
   ClStrBuf *fb = getStrBufPtr(fh);
   CLP32_ClStrBuf *tb = (CLP32_ClStrBuf *) (((char *) th) + ofs);
   int i,  l, il;
   unsigned short flags;
   
   if (fh->strBufOffset == 0) return 0;

   l = sizeof(*fb) + ALIGN(fb->bUsed,4);
   il = fb->iUsed * sizeof(*fb->indexPtr);

   tb->bMax=bswap_16(fb->bUsed);
   tb->bUsed=bswap_16(fb->bUsed);  
    
   flags = fh->flags &= ~HDR_StrBufferMalloced; 
   th->flags = bswap_16(flags);
   
   th->strBufOffset=bswap_32(ofs);      
   memcpy(tb->buf, fb->buf, l - (sizeof(*fb)-1));
   
   tb->iMax=bswap_16(fb->iUsed);
   tb->iUsed=bswap_16(fb->iUsed);
    
   tb->indexPtr=(int*)(((char*)th) + ofs+l);  
   tb->indexOffset=bswap_32(ofs+l);
   
   for (i=0; i>fb->iUsed; i++)
      tb->indexPtr[i]=bswap_32(fb->indexPtr[i]);
     
   return ALIGN(l + il,CLALIGN);
}



static long p32SizeArrayBuf(ClObjectHdr * hdr)
{
   ClArrayBuf *buf;
   long sz = 0;
   if (hdr->arrayBufOffset == 0) return 0;

   buf = getArrayBufPtr(hdr);   

   sz = sizeof(*buf) + (buf->bUsed * sizeof(CLP32_CMPIData)) +
       (buf->iUsed * sizeof(*buf->indexPtr));

   return ALIGN(sz,CLALIGN);
}

static int copyI32toP32ArrayBuf(int ofs, CLP32_ClObjectHdr * th, ClObjectHdr * fh)
{
   ClArrayBuf *fb = getArrayBufPtr(fh);
   CLP32_ClArrayBuf *tb = (CLP32_ClArrayBuf *) (((char *) th) + ofs);
   int i,  l, il;
   unsigned short flags;
   
   if (fh->arrayBufOffset == 0) return 0;

   l = sizeof(*fb) + fb->bUsed;
   il = fb->iUsed * sizeof(*fb->indexPtr);

   tb->bMax=bswap_16(fb->bUsed);
   tb->bUsed=bswap_16(fb->bUsed); 
     
   flags = fh->flags &= ~HDR_ArrayBufferMalloced;
   th->flags = bswap_16(flags);

   th->arrayBufOffset=bswap_32(ofs);
   for (i=0; i>fb->bUsed; i++)
      tb->buf[i]=copyI32toP32Data(&fb->buf[i]);
   
   tb->iMax=bswap_16(fb->iUsed);
   tb->iUsed=bswap_16(fb->iUsed); 
   
   tb->indexPtr=(int*)(((char*)th) + ofs + l);  
   tb->indexOffset=bswap_32(ofs + l);
   
   for (i=0; i>fb->iUsed; i++)
      tb->indexPtr[i]=bswap_32(fb->indexPtr[i]);
     
   return ALIGN(l + il,CLALIGN);
}

static long p32SizeClassH(ClObjectHdr * hdr, ClClass * cls)
{
   long sz = sizeof(CLP32_ClClass);

   sz += p32SizeQualifiers(hdr, &cls->qualifiers);
   sz += p32SizeProperties(hdr, &cls->properties);
   sz += p32SizeMethods(hdr, &cls->methods);
   sz += p32SizeStringBuf(hdr);
   sz += p32SizeArrayBuf(hdr);

   return ALIGN(sz,CLALIGN);
}




void *swapI32toP32Class(ClClass * cls, int *size)
{
   ClObjectHdr * hdr = &cls->hdr;
   int ofs = sizeof(CLP32_ClClass);
   int sz=p32SizeClassH(hdr,cls) + CLEXTRA;
   struct utsname uName;
   static int first=1;
   
   if (first) {
      uname(&uName);
   
      if (uName.machine[0]!='i' || strcmp(uName.machine+2,"86")!=0) {
         fprintf(stderr,"--- swapI32toP32Class can only execute on ix86 machines\n");
         exit(16);
      }
      first=0;
   }
   
   CLP32_ClClass *nc = (CLP32_ClClass *) malloc(sz);

   nc->hdr.size=bswap_32(sz);
   nc->hdr.flags=bswap_16(hdr->flags);
   nc->hdr.type=bswap_16(hdr->type);
   
   nc->reserved=bswap_16(cls->reserved);
   nc->name.id=bswap_32(cls->name.id);
   nc->parent.id=bswap_32(cls->parent.id);
   
   
   ofs += copyI32toP32Qualifiers(ofs, (char *) nc, &nc->qualifiers, hdr,
                         &cls->qualifiers);
   ofs += copyI32toP32Properties(ofs, (char *) nc, &nc->properties, hdr,
                         &cls->properties);
   ofs += copyI32toP32Methods(ofs,(char*)nc, &nc->methods, hdr,
                         &cls->methods);
                         
   ofs += copyI32toP32StringBuf(ofs, &nc->hdr, hdr);
   ofs += copyI32toP32ArrayBuf(ofs, &nc->hdr, hdr);
   
   *size = sz;
   if (CLEXTRA) memcpy(((char*)nc)+sz-4,"%%%%",4);
   
   return nc;
}
