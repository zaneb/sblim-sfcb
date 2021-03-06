

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define INC "#pragma include "

char *path;
int err=0;

FILE *getFILE(char *str)
{
   FILE *f=NULL;
   char *p,*n,ch,fn[1024];
   
   if ((p=strchr(str,'"'))) {
      p++;
      if ((n=strchr(p,'"'))) {
         ch=*n;
         *n=0;
         strcpy(fn,path);
         strcat(fn,p);
         f=fopen(fn,"r");
         *n=ch;
      }      
   }
   
   return f;
}

int incOK(char *str, char **s, char **e, char **ifn, FILE **f)
{
   char ch,fn[1024];
   *f=NULL;
   *ifn=NULL;
   
   while (*str && *str<=' ') str++;
   if (*str && strncmp(str,"#pragma ",8)==0) {
      *s=str;
      str+=8;
      while (*str && *str<=' ') str++;
      if (*str && strncmp(str,"include",7)==0) {
         str+=7;
         while (*str && *str<=' ') str++;
         if (*str=='(') {
            str+=1;
            while (*str && *str<=' ') str++;
            if (*str=='"') {
               str++;
               if ((*e=strchr(str,'"'))) {
                  ch=**e;
                  **e=0;
                  strcpy(fn,path);
                  strcat(fn,str);
                  *ifn=strdup(fn);
                  **e=ch;
                  (*e)++;
                  while (**e && **e<=' ') (*e)++;
                  if (**e==')') 
                     *f=fopen(fn,"r");
                  return 1;   
               }
            }
         }
      }
   }
   return 0;
}

char* getLineEnding(char* s) {
  char* e = NULL;
  if ((e = strstr(s, "\r\n"))) {
    e+=2;
  } else if ((e = strstr(s, "\n"))) {
    e+=1;
  }
  return e;
}

void processFile(char *fn, FILE *in, FILE *out)
{
   char *s,*e,*es,rec[10000],*ifn=NULL;
   FILE *incFile;
   int comment=0;
   int nl=0;
   
   while (fgets(rec, sizeof(rec), in)) {
      nl++;
      if (comment==0 && incOK(rec,&s,&e,&ifn,&incFile)) {
         if (incFile) {
            fprintf(out,"// resolved:  >>>> %s",s);
            processFile(ifn,incFile,out);
            fprintf(out,"// back from: >>>> %s",s);
         }
         else {
            fprintf(stderr,"%s:%d File not found: %s\n",fn,nl,s);
         }
      }
      else {
         s = rec;

         if (comment == 1) { /* check for single-line comment */
           e = getLineEnding(s);
           if (e) {
             strcpy(s, e);
             comment = 0;
           }
         } else if (comment == 2) { /* check for block comment */
           if ((e = strstr(s, "*/"))) {
             strcpy(s, getLineEnding(e));
             comment = 0;
           } else {
             continue;
           }
         }

         /* skip whitespace */
         while ((*s == ' ') || (*s == '\t')) {
           s++;
         }
         es = s;
         /* is this line a quoted string? */
         if (*s == '"') {
           /* find end of the string */
           es++;
           while ((s = strstr(es, "\""))) {
             es = s+1; /* end of quoted string */
           }
         }

         while ((s = strstr(es, "/"))) {
           if (*(s + 1) == '/') {
             e = getLineEnding(s);
             if (e) {
               strcpy(s, e);
             } else {
               *s = 0;
               comment = 1;
               break;
             }
           } else if (*(s + 1) == '*') {
             if ((e = strstr(s + 2, "*/"))) {
               strcpy(s, getLineEnding(e));
             } else {
               *s = 0;
               comment = 2;
               break;
             }
           } else {
             es++;
           }
         } 

         fprintf(out,"%s",rec);
      }
   }
   free(fn);
   fclose(in);
}

int main(int argc, char *argv[])
{
   char *fn=NULL,*p=NULL,ch;
   FILE *in;
   
   if (argc!=2) {
      fprintf(stderr,"usage: %s filename\n",argv[0]);
      return 2;
   }
   
   fn=strdup(argv[1]);
   
   if ((p=strrchr(fn,'/'))) {
      ch=*(p+1);
      *(p+1)=0;
      path=strdup(fn);
      *(p+1)=ch;
   }
   else path="";
   
   in=fopen(fn,"r");
   
   if (in) processFile(strdup(fn),in,stdout);
   else {
      fprintf(stderr,"file %s not found\n",fn);
      err=1;
   }
   
   if (*path)  free(path);
   if (*fn) free(fn);
   return err;
}
