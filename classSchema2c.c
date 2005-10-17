#include "utilft.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

//#include "classRegister.h"
#include "constClass.h"
#include "providerRegister.h"
#include "trace.h"
#include "control.h"

#define NEW(x) ((x *) malloc(sizeof(x)))

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpiftx.h"
#include "cmpimacs.h"
#include "cmpimacsx.h"
#include "objectImpl.h"
#include "mrwlock.h"

#include <time.h>

#define LOCALCLASSNAME "ClassProvider"

extern char * configfile;
extern ProviderRegister *pReg;

typedef struct _Class_Register_FT Class_Register_FT;
struct _ClassRegister {
   void *hdl;
   Class_Register_FT *ft;
   ClVersionRecord *vr;
   int assocs,topAssocs;
   char *fn;
};
typedef struct _ClassRegister ClassRegister;

typedef struct _ClassBase {
   UtilHashTable *ht;
   UtilHashTable *it;
   MRWLOCK mrwLock;
} ClassBase;

struct _Class_Register_FT {
   int version;
   void (*release) (ClassRegister * br);
   ClassRegister *(*clone) (ClassRegister * br);
   CMPIConstClass *(*getClass) (ClassRegister * br, const char *clsName);
   int (*putClass) (ClassRegister * br, CMPIConstClass * cls);
   int (*removeClass) (ClassRegister * br, const char *className);
   UtilList *(*getChildren) (ClassRegister * br, const char *className);
   void (*rLock)(ClassRegister * cr);
   void (*wLock)(ClassRegister * cr);
   void (*rUnLock)(ClassRegister * cr);
   void (*wUnLock)(ClassRegister * cr);
};

//extern Class_Register_FT *ClassRegisterFT;


typedef struct nameSpaces {
   int next,max,blen;
   char *base;
   char *names[1];
} NameSpaces;


static void buildClassSource(ClassRegister * cr, char *ns)
{
   ClassBase *cb = (ClassBase *) (cr + 1);
   UtilHashTable *ct = cb->ht, *it;
   HashTableIterator *i;
   char *cn;
   CMPIConstClass *cc;
   ClObjectHdr *hdr;
   unsigned char *buf;
   UtilList *ul;
   int j;

   it = cb->it = UtilFactory->newHashTable(61,
             UtilHashTable_charKey | UtilHashTable_ignoreKeyCase);

   fprintf(stdout,"\n#include <stdio.h>\n\n"); 
   fprintf(stdout,"typedef struct classDir {\n");
   fprintf(stdout,"   char *name;\n");
   fprintf(stdout,"   void *hdr;\n");
   fprintf(stdout,"} ClassDir;\n\n");

   fprintf(stdout,"typedef struct classSchema {\n");
   fprintf(stdout,"   void *versionRecord;\n");
   fprintf(stdout,"   ClassDir *classes;\n");
   fprintf(stdout,"} ClassSchema; \n\n");        

   
   fprintf(stdout,"static unsigned char version[]={"); 
   
   buf=(unsigned char*)cr->vr;
   if (buf) {
      fprintf(stdout,"0x%x",buf[0]);
      for (j=1; j<sizeof(ClVersionRecord); j++)
         fprintf(stdout,",0x%x",buf[j]);
      fprintf(stdout,"};\n\n"); 
   }
   
   for (i = ct->ft->getFirst(ct, (void **) &cn, (void **) &cc); i;
        i = ct->ft->getNext(ct, i, (void **) &cn, (void **) &cc)) {

      hdr=(ClObjectHdr*)cc->hdl; 
      buf=(unsigned char*)cc->hdl;   
      fprintf(stderr,"Class: %s - %d\n",  cc->ft->getCharClassName(cc),hdr->size); 
      
      fprintf(stdout,"static unsigned char %s[]={",cc->ft->getCharClassName(cc)); 
      fprintf(stdout,"0x%x",buf[0]);
      for (j=1; j<hdr->size; j++)
         fprintf(stdout,",0x%x",buf[j]);
      fprintf(stdout,"};\n"); 
   }
   
   fprintf(stdout,"\nstatic ClassDir classes[]={\n"); 
   for (i = ct->ft->getFirst(ct, (void **) &cn, (void **) &cc); i;
        i = ct->ft->getNext(ct, i, (void **) &cn, (void **) &cc)) {
      fprintf(stdout,"\t{\"%s\",%s},\n",cc->ft->getCharClassName(cc),cc->ft->getCharClassName(cc)); 
   }        
   fprintf(stdout,"\t{NULL,NULL}};\n"); 
   fprintf(stdout,"\nClassSchema %s_classes[]={%s,classes};\n\n",ns,cr->vr ? "version" : "NULL");
       
}


static ClassRegister *newClassRegister(char *fname, char *ns)
{
   ClassRegister *cr =
       (ClassRegister *) malloc(sizeof(ClassRegister) + sizeof(ClassBase));
   ClassBase *cb = (ClassBase *) (cr + 1);
   FILE *in;
   char fin[1024];
   long s, total=0;
   ClObjectHdr hdr;
   ClVersionRecord *vrp=(ClVersionRecord*)&hdr;
   int vRec=0,first=1;
   
   cr->hdl = cb;
//   cr->ft = ClassRegisterFT;
   cr->vr = NULL;
   cr->assocs = cr->topAssocs = 0;
   
   strcpy(fin, fname);
   strcat(fin, "/classSchemas");
   fprintf(stderr,"Opening %s\n",fname);
   in = fopen(fin, "r");
   
   if (in == NULL) {
      fprintf(stderr,"-*- classSchema directory %s not found\n",fname);
      return NULL;
   }

   cr->fn = strdup(fin);
   cb->ht = UtilFactory->newHashTable(61,
               UtilHashTable_charKey | UtilHashTable_ignoreKeyCase);
   MRWInit(&cb->mrwLock);

   while ((s = fread(&hdr, 1, sizeof(hdr), in)) == sizeof(hdr)) {
      CMPIConstClass *cc=NULL;
      char *buf=NULL;
      char *cn;
      
      if (first) {
         if (vrp->size==sizeof(ClVersionRecord) && vrp->type==HDR_Version &&
            strcmp(vrp->id,"sfcd-rep")==0) vRec=1;
         first=0;
      }
      
      if (vRec==0 && hdr.type!=HDR_Class) {
         mlogf(M_ERROR,M_SHOW,"--- %s contains non-class record(s) - directory skipped\n",fin);
         return NULL;
     }
      
      buf = (char *) malloc(hdr.size);
      if (buf==NULL) {
         mlogf(M_ERROR,M_SHOW,"--- %s contains record(s) that are too long - directory skipped\n",fin);
         return NULL;
      }
      
      s=hdr.size;
      *((ClObjectHdr *) buf) = hdr;
      
      if (fread(buf + sizeof(hdr), 1, hdr.size - sizeof(hdr), in) == hdr.size - sizeof(hdr)) {
         if (vRec) {
            cr->vr=(ClVersionRecord*)buf;
            continue;
         }
         cc = NEW(CMPIConstClass);
         cc->hdl = buf;
         cc->ft = CMPIConstClassFT;
         cc->ft->relocate(cc);
         cn=(char*)cc->ft->getCharClassName(cc);
         if (strncmp(cn,"DMY_",4)!=0) {
            total+=s;
            cb->ht->ft->put(cb->ht, cn, cc);
            if (cc->ft->isAssociation(cc)) {
               cr->assocs++;
               if (cc->ft->getCharSuperClassName(cc) == NULL) cr->topAssocs++;
            }   
         } 
      }
      else {
         mlogf(M_ERROR,M_SHOW,"--- %s contains invalid record(s) - directory skipped\n",fin);
         return NULL;
      }
   }
 
   if (cr->vr) {
      mlogf(M_INFO,M_SHOW,"--- ClassProvider for %s (%d.%d) using %ld bytes\n", 
          fname, cr->vr->version, cr->vr->level, total);
   }
   else mlogf(M_INFO,M_SHOW,"--- ClassProvider for %s (no-version) using %ld bytes\n", fname, total);

   buildClassSource(cr,ns);
   
   return cr;
}

int main(int args, char *argv[])
{
   char *ns,*fname;
   time_t tme;
   int i;
   
   if (args<2) {
      fprintf(stderr,"-*- classSchema2c: Bad invocation\n"
                     "    usage: classSchema2c <classSchema-directory> [<classSchema-prefix>]\n");
      return -1;
   }
   fname=argv[1];
   
   if (args==3) ns=argv[2];
   else {
      for (i=strlen(fname); i && fname[i]!='/'; i--);
      ns=fname+i+1; 
   }
   
   tme=time(NULL);
   fprintf(stdout,"// File generated by class2c %s",ctime(&tme)); 
   fprintf(stdout,"// input: %s\n",fname); 
   fprintf(stdout,"// ns:    %s\n\n",ns); 
  
   if (newClassRegister(fname,ns)==NULL) {
         return -1;
   }
   
   return 0;
}
