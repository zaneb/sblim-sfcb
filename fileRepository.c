
/*
 * fileRepository.c
 *
 * (C) Copyright IBM Corp. 2005
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Common Public License from
 * http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
 *
 * Author:        Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Directory/file based respository implementation.
 *
*/


#define CMPI_VERSION 90

#include <stdio.h>
#include <alloca.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "fileRepository.h"

#define BASE "repository"



void freeBlobIndex(BlobIndex **bip, int all)
{
   BlobIndex *bi;
   if (bip==NULL) return;
   bi=*bip;

   if (bi==NULL) return;
   if (bi->freed) return;
   if (bi->dir) { free(bi->dir); bi->dir=NULL; }
   if (bi->fnx) { free(bi->fnx); bi->fnx=NULL; }
   if (bi->fnd) { free(bi->fnd); bi->fnd=NULL; }
   if (all) if (bi->index)  { free(bi->index); bi->fnd=NULL; }
   bi->freed=-1;
   if (bi->fd) fclose(bi->fd);
   free(bi);
   *bip=NULL;
}

static int indxLocate(BlobIndex *bi,char *key)
{
   int n,dp,elen,kl=strlen(key),ekl;
   char *kp,ch,c;
   for (dp=0; dp<bi->dSize; ) {
      c=sscanf(bi->index+dp,"%d %d %1s%n",&elen,&ekl,&ch,&n);
      kp=bi->index+dp+n-1;
      if (kl==ekl) {
         if (strncmp(key,kp,kl)==0) {
            c=sscanf(bi->index+dp+n-1+ekl,"%d %d",&bi->blen,&bi->bofs);
            bi->pos=dp;
            bi->len=elen;
            return 1;
         }
      }
      dp+=elen;
   }
   return 0;
}

void* getFirst(BlobIndex *bi, int *len)
{
   int n,dp,elen,ekl,c;
   char ch,*buf;
   dp=bi->next=0;

   c=sscanf(bi->index+dp,"%d %d %1s%n",&elen,&ekl,&ch,&n);
   c=sscanf(bi->index+dp+n-1+ekl,"%d %d",&bi->blen,&bi->bofs);

   bi->fd=fopen(bi->fnd,"rb");
   fseek(bi->fd,bi->bofs,SEEK_SET);
   buf=(char*)malloc(bi->blen+8);
   fread(buf,bi->blen,1,bi->fd);
   if (len) *len=bi->blen;
   buf[bi->blen]=0;

   bi->next+=elen;
   return (void*)buf;
}

void* getNext(BlobIndex *bi, int *len)
{
   int n,dp=bi->next,elen,ekl,c;
   char ch,*buf;

   if (dp>=bi->dSize) {
      *len=0;
      fclose(bi->fd);
      bi->fd=NULL;
      return NULL;
   }

   c=sscanf(bi->index+dp,"%d %d %1s%n",&elen,&ekl,&ch,&n);
   c=sscanf(bi->index+dp+n-1+ekl,"%d %d",&bi->blen,&bi->bofs);

   fseek(bi->fd,bi->bofs,SEEK_SET);
   buf=(char*)malloc(bi->blen+8);
   fread(buf,bi->blen,1,bi->fd);
   if (len) *len=bi->blen;
   buf[bi->blen]=0;

   bi->next+=elen;
   return (void*)buf;
}

static void copy(FILE *o, FILE *i, int len, unsigned long ofs)
{
   char *buf=(char*)malloc(len);
   fseek(i,ofs,SEEK_SET);
   fread(buf,len,1,i);
   fwrite(buf,len,1,o);
   free(buf);
}

static int adjust(BlobIndex *bi, int pos, int adj)
{
   int dp=pos+bi->len,l,o,sl;
   char *p,*r,str[32]="                               ";
   while (dp<bi->dSize) {
      l=atoi(bi->index+dp);
      for (p=bi->index+dp+l-2; *p!=' '; p--) if (*p=='\r') r=p;
      o=atoi(++p);
      o-=adj;
      sl=sprintf(str+(r-p),"%d",o);
      memcpy(p,str+sl,r-p);
      dp+=l;
   }
   return 0;
}

static int rebuild(BlobIndex *bi, char *id, void *blob, int blen)
{
   int ofs,len,xt=0,dt=0;
   unsigned long pos;
   char *xn=alloca(strlen(bi->dir)+8);
   char *dn=alloca(strlen(bi->dir)+8);
   FILE *x,*d;

   strcpy(xn,bi->dir);
   strcat(xn,"idx");
   strcpy(dn,bi->dir);
   strcat(dn,"inst");
   x=fopen(xn,"wb");
   d=fopen(dn,"wb");

   if (bi->bofs) copy(d,bi->fd,bi->bofs,0);
   dt+=bi->bofs;
   len=bi->dlen-(bi->bofs+bi->blen);
   if (len) copy(d,bi->fd,bi->dlen-(bi->bofs+bi->blen),bi->bofs+bi->blen);
   dt+=len;
   pos=ftell(d);
   if (blen) fwrite(blob,blen,1,d);
   dt+=blen;
   fclose(d);

   adjust(bi,bi->pos,bi->blen);

   ofs=bi->pos+bi->len;
   if (bi->pos) fwrite(bi->index,bi->pos,1,x);
   xt+=bi->pos;
   len=bi->dSize-ofs;
   if (len) fwrite(bi->index+ofs,len,1,x);
   xt+=len;
   fclose(x);

   remove(bi->fnd);
   remove(bi->fnx);
   if (dt) rename(dn,bi->fnd);
   else remove(dn);
   if (xt) rename(xn,bi->fnx);
   else remove(xn);

   return 0;
}

int getIndex(char *ns, char *cls, int elen, int mki, BlobIndex **bip)
{
   BlobIndex *bi;
   char *fn=alloca(elen);
   char *p;

   bi=NEW(BlobIndex);

   strcpy(fn,BASE);
   strcat(fn,"/");
   p=fn+strlen(fn);
   strcat(fn,ns);
   strcat(fn,"/");
   while (*p) { *p=tolower(*p); p++; }
   bi->dir=strdup(fn);
   strcat(fn,cls);
   
   p=fn; 
   while (*p) { *p=tolower(*p); p++; }
   
   bi->fnd=strdup(fn);
   strcat(fn,".idx");
   bi->fnx=strdup(fn);    

   bi->fx=fopen(bi->fnx,"rb+");
   if (bi->fx==NULL) {
      if (mki==0) {
         freeBlobIndex(&bi,1);
         *bip=NULL;
         return 0;
      }
      bi->fx=fopen(bi->fnx,"wb");
      bi->aSize=elen;
      bi->dSize=0;
      bi->index=malloc(bi->aSize);
   }

   else {
      fseek(bi->fx,0,SEEK_END);
      bi->dSize=ftell(bi->fx);
      bi->aSize=bi->dSize+elen;
      bi->index=malloc(bi->aSize);
      fseek(bi->fx,0,SEEK_SET);
      fread(bi->index,bi->dSize,1,bi->fx);
    }
    *bip=bi;
    return 1;
}

int addBlob(char *ns, char * cls, char *id, void *blob, int len)
{
   int keyl=strlen(ns)+strlen(cls)+strlen(id)+strlen(BASE);
   int es,ep,rc;
   char *idxe=alloca(keyl+64);
   BlobIndex *bi;

   rc=getIndex(ns,cls,keyl+64,1,&bi);
   if (rc==0) return 1;

   if (bi->dSize==0) {
      bi->fd=fopen(bi->fnd,"wb");
      if (bi->fd==NULL) { freeBlobIndex(&bi,1); return -1; }
      fwrite(blob,len,1,bi->fd);
      fclose(bi->fd);
      bi->fd=NULL;

      es=sprintf(idxe,"    %d %s %d %d\r\n",strlen(id),id,len,0);
      ep=sprintf(idxe,"%d",es);
      idxe[ep]=' ';

      memcpy(bi->index,idxe,es);
      bi->dSize=es;
      fwrite(bi->index,bi->dSize,1,bi->fx);
      fclose(bi->fx);
      bi->fx=NULL;
   }

   else {

      if (indxLocate(bi,id)) {
         bi->fd=fopen(bi->fnd,"rb");
         fseek(bi->fd,0,SEEK_END);
         bi->dlen=ftell(bi->fd);
         es=sprintf(idxe,"    %d %s %d %lu\r\n",strlen(id),id,len,bi->dlen);
         ep=sprintf(idxe,"%d",es);
         idxe[ep]=' ';

         memcpy(bi->index+bi->dSize,idxe,es);
         bi->dSize+=es;
         rebuild(bi,id,blob,len);
      }

      else {
         bi->fd=fopen(bi->fnd,"ab+");
         if (bi->fd==NULL) bi->fd=fopen(bi->fnd,"wb+");
         fseek(bi->fd,0,SEEK_END);
         bi->fpos=ftell(bi->fd);
         fwrite(blob,len,1,bi->fd);
         fclose(bi->fd);
         bi->fd=NULL;

         es=sprintf(idxe,"    %d %s %d %lu\r\n",strlen(id),id,len,bi->fpos);
         ep=sprintf(idxe,"%d",es);
         idxe[ep]=' ';

         memcpy(bi->index+bi->dSize,idxe,es);
         bi->dSize+=es;
         fseek(bi->fx,0,SEEK_SET);
         fwrite(bi->index,bi->dSize,1,bi->fx);
         fclose(bi->fx);
         bi->fx=NULL;
     }
   }
   freeBlobIndex(&bi,1);
   return 0;
}

int deleteBlob(char *ns, char * cls, char *id)
{
   int keyl=strlen(ns)+strlen(cls)+strlen(id)+strlen(BASE);
   BlobIndex *bi;
   int rc;

   rc=getIndex(ns,cls,keyl+64,0,&bi);

   if (rc) {
      if (indxLocate(bi,id)) {
         bi->fd=fopen(bi->fnd,"rb");
         fseek(bi->fd,0,SEEK_END);
         bi->dlen=ftell(bi->fd);
         rebuild(bi,id,NULL,0);
         freeBlobIndex(&bi,1);
         return 0;
      }
   }
   freeBlobIndex(&bi,1);
   return 1;
}

int existingBlob(char *ns, char * cls, char *id)
{
   int keyl=strlen(ns)+strlen(cls)+strlen(id)+strlen(BASE);
   BlobIndex *bi;
   int rc=0,r=0;

   rc=getIndex(ns,cls,keyl+64,0,&bi);

   if (rc)
      if (indxLocate(bi,id)) r=1;

   freeBlobIndex(&bi,1);
   return r;
}

void *getBlob(char *ns, char *cls, char *id, int *len)
{
   int keyl=strlen(ns)+strlen(cls)+strlen(id)+strlen(BASE);
   BlobIndex *bi;
   char *buf;
   int rc=0;

   rc=getIndex(ns,cls,keyl+64,0,&bi);

   if (rc) {
      if (indxLocate(bi,id)) {
         bi->fd=fopen(bi->fnd,"rb");
         if (bi->fd==NULL) {
            fprintf(stderr,"*** Repository error for %s \n",bi->fnd);
            perror("Repository error: ");
            exit(5);
         }
         fseek(bi->fd,bi->bofs,SEEK_SET);
         buf=(char*)malloc(bi->blen+8);
         fread(buf,bi->blen,1,bi->fd);
         if (len) *len=bi->blen;
         buf[bi->blen]=0;
         freeBlobIndex(&bi,1);
         return (void*)buf;
      }
   }
   freeBlobIndex(&bi,1);
   return NULL;
}

int existingNameSpace(char *ns)
{
   int keyl=strlen(ns)+strlen(BASE);
   char *fn=alloca(keyl+64),*p;
   FILE *ft=NULL;

   strcpy(fn,BASE);
   strcat(fn,"/");
   p=fn+strlen(fn);
   strcat(fn,ns);
   strcat(fn,"/");
   strcat(fn,"__test__");

   while (*p) { *p=tolower(*p); p++; }

#ifdef __MAIN__
   printf("--- testing %s \n",fn);
#endif
   ft=fopen(fn,"w");
   if (ft==NULL) return 0;
   fclose(ft);
   remove(fn);
   return 1;
}

#ifdef __MAIN__

char *o1="first-object";
char *o2="second-object";
char *o3="third-object";
char *o4="fourth-object";
char *ns="root";

int main()
{
   if (existingNameSpace(ns)==0) {
      printf("--- namspace %s does not exist\n",ns);
      exit(1);
   }
   addBlob(ns,"class1",o1,o1,strlen(o1));
   addBlob(ns,"class1",o2,o2,strlen(o2));
   addBlob(ns,"class1",o3,o3,strlen(o3));
   addBlob(ns,"class1",o4,o4,strlen(o4));
   addBlob(ns,"class1",o2,o2,strlen(o2));
   deleteBlob(ns,"class1",o2);
   deleteBlob(ns,"class1",o1);
   deleteBlob(ns,"class1",o4);
   printf("--- %s\n",(char*)getBlob(ns,"class1",o3,NULL));
   deleteBlob(ns,"class1",o3);
}

#endif
