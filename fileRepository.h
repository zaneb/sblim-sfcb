
/*
 * fileRepository.h
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
 * Author:        Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Directory/file based respository implementation.
 *
*/


#ifndef _FILEREPOSITORY_
#define _FILEREPOSITORY_

#define BASE "repository"

typedef struct blobIndex {
   int freed;
   char *fnx,*fnd,*dir,*id;
   FILE *fx,*fd;
   char *index;
   int dSize,aSize;
   int pos,len,blen,bofs,next;
   unsigned long fpos;
   unsigned long dlen;
} BlobIndex;

#define NEW(td) (td*)calloc(sizeof(td),1)

extern void freeBlobIndex(BlobIndex **bi, int all);
extern int getIndex(char *ns, char *cls, int elen, int mki, BlobIndex** bip);
extern int addBlob(char *ns, char * cls, char *id, void *blob, int len);
extern int deleteBlob(char *ns, char * cls, char *id);
extern void *getBlob(char *ns, char *cls, char *id, int *len);
extern int existingBlob(char *ns, char * cls, char *id);
extern int existingNameSpace(char *ns);
extern void* getFirst(BlobIndex *bi, int *len);
extern void* getNext(BlobIndex *bi, int *len);

#endif
