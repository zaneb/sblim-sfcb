#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "cmpidt.h"
#include "cmpimacs.h"
#include "fileRepository.h"
#include "instance.h"
#include "objectImpl.h"
#include "control.h"
#include "array.h"

#define VERSION "0.8.0"

static int opt_verbose = 0;
static int opt_version = 0;
static int opt_help = 0;
static int opt_from_mof = 0;
static int opt_only_from_mof = 0;
static int opt_truncate = 0;
static char outfilepath[3000] = {0};
static char sfcbcfg[1024] = SFCB_CONFDIR"/sfcb.cfg";
static char namespace[600] = {0};
static char classname[600] = {0};
static char altRepositoryDir[1024] = {0};
static char * valid_options = "n:c:g:o:r:tphvVmM";
#define dbg(x) if(opt_verbose) {x;}


extern char *sfcb_pathToChars(CMPIObjectPath * cop, CMPIStatus * rc, char *str);
extern CMPIObjectPath *getObjectPath(char *path, char **msg);
extern CMPIData __ift_getPropertyAt(const CMPIInstance * ci, CMPICount i, CMPIString ** name,
                                     CMPIStatus * rc);

static char *datatypeToString(CMPIData * d, int *isarray)
{
   if (isarray != NULL)
       *isarray = (d->type & CMPI_ARRAY);
   
   CMPIType type = d->type & ~CMPI_ARRAY;

   switch (type) {
       case CMPI_chars:
       case CMPI_string:
          return "string";
       case CMPI_real64:
          return "real64";
       case CMPI_real32:
          return "real32";
       case CMPI_sint64:
          return "sint64";
       case CMPI_uint64:
          return "uint64";
       case CMPI_sint32:
          return "sint32";
       case CMPI_uint32:
          return "uint32";
       case CMPI_sint16:
          return "sint16";
       case CMPI_uint16:
          return "uint16";
       case CMPI_uint8:
          return "uint8";
       case CMPI_sint8:
          return "sint8";
       case CMPI_boolean:
          return "boolean";
       case CMPI_char16:
          return "char16";
       case CMPI_ref:
          return "ref";
       case CMPI_dateTime:
          return "dateTime";
       default:
          dbg(printf("unknown type: %04x : %d\n", type, type));
          return "unknownType";
   }
}

char *escapeQuotes(char *in)
{
   int i, l, o;
   char *out;

   if (in == NULL)
       return (NULL);
   l = strlen(in);
   out = (char *) malloc((l * 2) + 1); // worst case scenario - it's all quotes

   for (i = 0, o = 0; i < l; i++) {
       if (in[i] == '\"') {
           out[o++]='\\';
           out[o++]='\"';
       }
       else {
           out[o++] = in[i];
       }
   }
   out[o] = '\0';
   return out;
}

UtilStringBuffer * dataValueToStringBuf(CMPIData d, CMPIInstance *inst)
{
   UtilStringBuffer *sb = UtilFactory->newStrinBuffer(64);
   char str[256] = {0};// for string, we'll redirect the sp pointer,
                       // all other types should fit in this char-array
   char *sp = str;
   int needsQuotes = 0;
   int needsFree = 0;

   if (d.type & CMPI_ARRAY) 
   {
      sb->ft->appendChars(sb, "{");
      struct native_array * na = (struct native_array *)d.value.array;
     
      int i = 0; 
      for (i=0; i<na->size; i++)
      {
          if (i!=0)
              sb->ft->appendChars(sb,",");
          CMPIData dd;
          dd.type = d.type & ~CMPI_ARRAY;
          dd.state= na->data[i].state;
          dd.value = na->data[i].value;
          UtilStringBuffer *b = dataValueToStringBuf(dd, inst);
          sb->ft->appendChars(sb, b->ft->getCharPtr(b));
      }
      sb->ft->appendChars(sb, "}");
      return sb;
   }

   if((d.type & (CMPI_UINT|CMPI_SINT)) == CMPI_UINT) {
      unsigned long long ul = 0LL;
      switch (d.type) {
      case CMPI_uint8:
         ul = d.value.uint8;
         break;
      case CMPI_uint16:
         ul = d.value.uint16;
         break;
      case CMPI_uint32:
         ul = d.value.uint32;
         break;
      case CMPI_uint64:
         ul = d.value.uint64;
         break;
      }
      sprintf(str, "%llu", ul);
   }
   else if((d.type & (CMPI_UINT|CMPI_SINT)) == CMPI_SINT) {
      long long sl = 0LL;
      switch (d.type) {
      case CMPI_sint8:
         sl = d.value.sint8;
         break;
      case CMPI_sint16:
         sl = d.value.sint16;
         break;
      case CMPI_sint32:
         sl = d.value.sint32;
         break;
      case CMPI_sint64:
         sl = d.value.sint64;
         break;
      }
      sprintf(str, "%lld", sl);
   }
   else if (d.type & CMPI_REAL) {
      switch (d.type) {
      case CMPI_real32:
         sprintf(str, "%.7e", d.value.real32);
         break;
      case CMPI_real64:
         sprintf(str, "%.16e", d.value.real64);
         break;
      }
   }
   else if (d.type == CMPI_boolean) {
      sprintf(str, "%s", d.value.boolean ? "TRUE" : "FALSE");
   }
   else if (d.type == CMPI_char16) {
      sprintf(str, "%c", d.value.char16);
   }
   else if (d.type == CMPI_chars) {
      needsQuotes = 1;
      sp = (char *) d.value.string->hdl;
   }
   else if (d.type == CMPI_string) {
      needsQuotes = 1;
      sp = (char *) d.value.string->hdl;
   }
   else if (d.type == CMPI_dateTime) {
      if (!(d.state & CMPI_nullValue)) {
          needsQuotes = 1;
          dateTime2chars(d.value.dateTime, NULL, str);
      }
   }
   else if (d.type == CMPI_ref) {
      needsQuotes = 1;
      sfcb_pathToChars(d.value.ref, NULL, str);
   }
   else if (d.type == CMPI_instance) {
      needsQuotes = 1;
      sp = (char *) d.value.string->hdl;
   }
   else {
      sp = "Unknown type";
   }

   if (needsQuotes)
   {
      //also needs escaping
      sp=escapeQuotes(sp);
      needsFree = 1;
   }
   if (needsQuotes)
      sb->ft->appendChars(sb, "\"");
   sb->ft->appendChars(sb, sp);
   if (needsQuotes)
      sb->ft->appendChars(sb, "\"");
   if (needsFree)
      free(sp);
   return sb;
}


UtilStringBuffer *instanceToMofWithType(CMPIInstance * ci, int withType)
{
   unsigned int i, m;
   CMPIString *name;
   CMPIStatus rc = {0, NULL};
   UtilStringBuffer *sb = UtilFactory->newStrinBuffer(64);
   ClInstance *cli = (ClInstance *)ci->hdl;
   if (!cli)
       return NULL;

   SFCB_APPENDCHARS_BLOCK(sb, "Instance of ");
   sb->ft->appendChars(sb, instGetClassName(ci));
   SFCB_APPENDCHARS_BLOCK(sb, "\n{\n");
   for (i = 0, m = ClInstanceGetPropertyCount(cli); i < m; i++)
   {
       CMPIData d = __ift_getPropertyAt(ci, i, &name, &rc);
       if (rc.rc == CMPI_RC_OK)
       {
           if (!(d.state & CMPI_nullValue))
           {
                SFCB_APPENDCHARS_BLOCK(sb, "   ");
                if (withType)
                {
                    int isarray = 0;
                    sb->ft->appendChars(sb, datatypeToString(&d, &isarray));
                    if (isarray)
                        SFCB_APPENDCHARS_BLOCK(sb, "[]");
                    SFCB_APPENDCHARS_BLOCK(sb, " ");
                }
                sb->ft->appendChars(sb, name->hdl);
                SFCB_APPENDCHARS_BLOCK(sb, " = ");
                UtilStringBuffer *buff = dataValueToStringBuf(d, ci);
                sb->ft->appendChars(sb, buff->ft->getCharPtr(buff));
                SFCB_APPENDCHARS_BLOCK(sb, ";\n");
           }
       }
   }
   
   SFCB_APPENDCHARS_BLOCK(sb, "};\n");
   return sb;
}


UtilStringBuffer *instanceToMof(CMPIInstance * ci)
{
    return instanceToMofWithType(ci, 0);
}


static CMPIInstance *instifyBlob(void * blob) {
    CMPIInstance *inst;
    int id;

    if (blob==NULL) {
        return NULL;
    }
    else {
        inst=relocateSerializedInstance(blob);
        memAdd(blob, &id);
        return inst;
    }
}


static BlobIndex *_getIndex(const char *ns, const char *cn)
{
    BlobIndex *bi;
    if (getIndex(ns,cn,strlen(ns)+strlen(cn)+64,0,&bi))
        return bi;
    else return NULL;
}


static CMPIInstance* ipGetFirst(BlobIndex *bi, int *len, char** keyb, size_t *keybl) {
    void *blob=getFirst(bi, len, keyb, keybl);
    return instifyBlob(blob);
}

static CMPIInstance* ipGetNext(BlobIndex *bi, int *len, char** keyb, size_t *keybl) {
    void *blob=getNext(bi, len, keyb, keybl);
    return instifyBlob(blob);
}



static int parse_options(int argc, char * argv[])
{
    int opt;
    while ((opt=getopt(argc,argv,valid_options)) != -1) 
    {
        switch (opt) 
        {
            case 'n': // namespace, required
                strncpy(namespace,optarg,sizeof(namespace));
                break;
            case 'c': // classname, required
                strncpy(classname,optarg,sizeof(classname));
                break;
            case 'g': // sfcb configuration - default = SFCB_CONFIR = (/usr/local)/etc/sfcb/sfcb.cfg
                      // this configuration specifies the repository directory
                strncpy(sfcbcfg,optarg,sizeof(sfcbcfg));
                break;
            case 'o':
                strncpy(outfilepath,optarg,sizeof(outfilepath));
                break;
            case 'r':
                strncpy(altRepositoryDir,optarg,sizeof(altRepositoryDir));
                break;
            case 't': // default is append, not truncate
                opt_truncate = 1;
                break;
            case 'm':
                opt_from_mof = 1;
                break;
            case 'M':
                opt_only_from_mof = 1;
                break;
            case 'v':
                opt_verbose = 1;
                break;
            case 'V':
                opt_version = 1;
                break;
            case 'h':
                opt_help = 1;
                break;
            default:
                return -1;
        }
    }
    return optind;
}


static void version(char *name)
{
    printf("%s - Version %s  -  Dump static instances to mof\n", name, VERSION);
}

static void usage(char *name)
{
    printf("%s <options> [-n namespace] [-c classname]\n", name);
}

static void help(char *name)
{
    usage(name);
    version(name);
    printf(" Allowed options are\n");
    printf("  -h                 display this message\n");
    printf("  -v                 verbose: print some extra processing information to stdout\n");
    printf("  -V                 print version information\n");
    printf("  -n <namespace>     [REQUIRED]\n");
    printf("  -c <classname>     [REQUIRED] - actual class name instrumented - not a parent class\n");
    printf("  -g <sfcb cfg file> (default=%s/sfcb.cfg)\n", SFCB_CONFDIR);
    printf("  -o <output file>   full path to output file. Path must exist, with write rights.  (default = stdout)\n");
    printf("  -t                 truncate output file before writing to it (default = append)\n");
    printf("  -m                 output instances that were created in MOF, in addition to instances created interactively\n");
    printf("  -M                 output ONLY instances that were created in MOF (don't include instances created interactively)\n");
    printf("  -r <repository dir> use alternate repository (override sfcb cfg file). (default = %s/registration/repository)\n", SFCB_STATEDIR);
}


int main(int argc, char *argv[])
{
    int argidx;
    if ((argidx=parse_options(argc,argv)) == -1) 
    {
        help(argv[0]);
        return 1;
    }
    if (opt_version) 
    {
        version(argv[0]);
        return 0;
    }
    if (opt_help) 
    {
        help(argv[0]);
        return 0;
    }
    if (opt_verbose) 
    {
        version(argv[0]);
        printf("Parsing %s for instances, output to %s\n", classname, *outfilepath?outfilepath:"stdout");
        if (opt_only_from_mof) 
        {
            printf("  Dumping only instances created in MOF\n");
        }
        else if (opt_from_mof)
        {
            printf("  Dump includes instances created from MOF\n");
        }
        else
        {
            printf("  Dumping only instances not created from MOF\n");
        }
    }

    if ( *classname==0 || *namespace==0)
    {
        printf("--> You must provide a namespace and a classname\n");
        help(argv[0]);
        return 0;
    }


    // now let's get to work
    char *ns = namespace;
    char *clsname = classname;
    char *cfg = sfcbcfg;

    char *msg = NULL;
    char *kp = NULL;
    size_t ekl = 0;
    CMPIObjectPath *cop = NULL;
    CMPIInstance *inst = NULL;
    char copKey[8192]={0};
    BlobIndex *bi = NULL;
   
    setupControl(cfg);
    if (*altRepositoryDir)
    {
        // must have trailing '/'... if not there, add it
        int len = strlen(altRepositoryDir);
        if (altRepositoryDir[len-1] != '/')
            strcat(altRepositoryDir, "/");
        useAlternateRepository(altRepositoryDir);
        dbg(printf("> Using alternate repository dir: %s\n", altRepositoryDir));
    }
    
    dbg(printf("> NS: %s\n", ns));
    dbg(printf("> CN: %s\n", clsname));

    if ((bi=_getIndex(ns,clsname))!=NULL) 
    {
        inst = ipGetFirst(bi,NULL,&kp,&ekl);
        if (inst) 
        {
            FILE *fp = NULL;
            if (*outfilepath)
            {
                if (opt_truncate)
                    fp = fopen(outfilepath, "w");
                else
                    fp = fopen(outfilepath, "a");
            }

            while(1) 
            {
                dbg(printf("> got instance\n"));
                
                strcpy(copKey,ns);
                strcat(copKey,":");
                strcat(copKey,clsname);
                strcat(copKey,".");
                strncat(copKey,kp,ekl);

                cop=(CMPIObjectPath *)getObjectPath(copKey,&msg);
                if (msg)
                    dbg(printf(" ! got error msg getting cop: %s\n", msg));
                if (cop)
                {
                    int fromMof = 0;
                    char copstr[4096] = { 0 };
                    dbg(printf("> > got cop: %s\n", sfcb_pathToChars(cop, NULL, copstr)));
                    ClInstance *cli = (ClInstance *) (inst->hdl);
                    dbg(showClHdr((void *)&cli));
                    if (cli->hdr.flags & HDR_FromMof)
                    {
                        fromMof = 1;
                        dbg(printf("> > Instance is from Mof\n"));
                    }
                    else
                    {
                        fromMof = 0;
                        dbg(printf("> > Instance is Not from Mof\n"));
                    }
                    
                    if ((!fromMof && (!opt_only_from_mof)) || 
                        (fromMof && (opt_from_mof || opt_only_from_mof)))
                    {
                        dbg(printf("> > Including instance in output.\n"));
                        UtilStringBuffer *sb = instanceToMof(inst);
                        
                        char *str=(char *)sb->ft->getCharPtr(sb);
                        if (fp)
                        {
                            fputs((const char *)str, fp);
                            fputs("\n\n", fp);
                        }
                        else
                        {
                            printf("%s",(const char *) str);
                            printf("\n\n");
                        }
                    }
                }
                else 
                {
                    return -1;
                }
                if ((bi->next < bi->dSize) && (inst=ipGetNext(bi,NULL,&kp,&ekl)))
                {
                    continue;
                }
                break;
            }
            if (fp)
                fclose(fp);
        }
    }
    freeBlobIndex(&bi,1);
    
    return 0;
}
