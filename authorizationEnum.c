
/*
 * authorizationEnum.c
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
 * Author:       Gareth Bestor 
 * Contributors: Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Enumeratuion support for Authorization provider for sfcb
 *
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"
#include "trace.h"

static char * _CLASSNAME = "SFC_Authorization";
static char * _CONFIGFILE = "/tmp/sfcb.authorizedusers";

/* External parsing routines */
extern void sfcAuthyyrestart( FILE * infile );
extern int sfcAuthyyparseinstance( CMPIInstance * instance );

#define _MAXNAMELENGTH 1024

/* Record containing essential config file info, namely the filename and file handle */
typedef struct {
   char name[_MAXNAMELENGTH];
   FILE * handle;
} _FILEINFO;


/* ---------------------------------------------------------------------------
 * CUSTOMIZED FUNCTION TO FORMAT INSTANCE DATA INTO TEXT
 * --------------------------------------------------------------------------- */

#define _MAXINSTANCELENGTH 1024

static int _instance2string( CMPIInstance * instance, char ** buffer )
{
   CMPIData data;                       /* General purpose CIM data storage for CIM property values */
   char string[_MAXINSTANCELENGTH];     /* General purpose string buffer for formatting values */
   char * str;                          /* General purpose string pointer */
                                                                                                                      
   _SFCB_ENTER(TRACE_PROVIDERS, "_instance2string");

   /* Create a text buffer to hold the new formatted instance data */
   *buffer = malloc(_MAXINSTANCELENGTH);
   strcpy(*buffer, "");

   /* Write out the formatted property values for this instance */
                                                                                                                      
   data = CMGetProperty(instance, "Username", NULL);
   if (!CMIsNullValue(data)) {
      str = CMGetCharPtr(data.value.string);
      if (*str != '\0') {
         strcat(*buffer, "[");
         strcat(*buffer, str);
      } else {
         _SFCB_TRACE(1,("_instance2string() : Username is not set!"));
         return 0;
      }
   }
   strcat(*buffer, " ");

   data = CMGetProperty(instance, "Classname", NULL);
   if (!CMIsNullValue(data)) {
      str = CMGetCharPtr(data.value.string);
      if (*str != '\0') {
         strcat(*buffer, str);
      } else {
         _SFCB_TRACE(1,("_instance2string() : Classname is not set!"));
         return 0;
      }
   }
   strcat(*buffer, "]\n");

   data = CMGetProperty(instance, "Enumerate", NULL);
   if (!CMIsNullValue(data)) {
      sprintf(string, "\tenumerate : %s\n", data.value.boolean? "TRUE":"FALSE");
      strcat(*buffer, string);
   }
                                                                            
   data = CMGetProperty(instance, "Get", NULL);
   if (!CMIsNullValue(data)) {
      sprintf(string, "\tget : %s\n", data.value.boolean? "TRUE":"FALSE");
      strcat(*buffer, string);
    }
   
   data = CMGetProperty(instance, "Set", NULL);
   if (!CMIsNullValue(data)) {
      sprintf(string, "\tset : %s\n", data.value.boolean? "TRUE":"FALSE");
      strcat(*buffer, string);
    }
   
   data = CMGetProperty(instance, "Create", NULL);
   if (!CMIsNullValue(data)) {
      sprintf(string, "\tcreate : %s\n", data.value.boolean? "TRUE":"FALSE");
      strcat(*buffer, string);
    }
   
   data = CMGetProperty(instance, "Delete", NULL);
   if (!CMIsNullValue(data)) {
      sprintf(string, "\tdelete : %s\n", data.value.boolean? "TRUE":"FALSE");
      strcat(*buffer, string);
    }

   data = CMGetProperty(instance, "Query", NULL);
   if (!CMIsNullValue(data)) {
      sprintf(string, "\tquery : %s\n", data.value.boolean? "TRUE":"FALSE");
      strcat(*buffer, string);
    }

   strcat(*buffer, "#\n");

   _SFCB_TRACE(1,("instance2string() : New instance entry is\nSTART-->%s<--END", *buffer));
   return 1;
}


/* ---------------------------------------------------------------------------
 * CUSTOMIZED INSTANCE ENUMERATION FUNCTIONS
 * --------------------------------------------------------------------------- */

/*
 * startReadingInstances
 */
static void * _startReadingInstances()
{
   _FILEINFO * fileinfo;		/* Filename and handle for the config file */ 

   _SFCB_ENTER(TRACE_PROVIDERS, "_startReadingInstances");

   /* Open the config file for reading */
   fileinfo = malloc(sizeof(_FILEINFO));
   strcpy(fileinfo->name, _CONFIGFILE);
   if ((fileinfo->handle = fopen(fileinfo->name,"r")) == NULL) {
      _SFCB_TRACE(1,("startReadingInstances() : Cannot open file %s for reading", fileinfo->name));
      free(fileinfo);
      return NULL;
   }

   /* Reset the parser to start reading from the beginning of the config file */
   sfcAuthyyrestart(fileinfo->handle);

   /* Return a handle to the config file that will subsequently be used to read in instances */
   return fileinfo;
}


/*
 * readNextInstance
 */
static int _readNextInstance( void * instances, CMPIInstance ** instance, char * namespace )
{
   CMPIStatus status = {CMPI_RC_OK, NULL};	/* Return status of CIM operations */
   CMPIObjectPath * objectpath;			/* CIM object path of the current instance */
   int rc;					/* Return code from the parser */

   _SFCB_ENTER(TRACE_PROVIDERS, "_readNextInstance");

   /* Create a new CIM object path for this instance */
   objectpath = CMNewObjectPath(_BROKER, namespace, _CLASSNAME, &status);
   if (status.rc != CMPI_RC_OK) {
      _SFCB_TRACE(1,("readNextInstance() : Failed to create new object path - %s", CMGetCharPtr(status.msg)));
      *instance = NULL;
      _SFCB_RETURN(EOF);
   }

   /* Create a new CIM instance for the new object path */
   *instance = CMNewInstance(_BROKER, objectpath, &status);
   if (status.rc != CMPI_RC_OK) {
      _SFCB_TRACE(1,("readNextInstance() : Failed to create new instance - %s", CMGetCharPtr(status.msg)));
      *instance = NULL;
      _SFCB_RETURN(EOF);
   }

   /* Parse the next instance in the config file (and in doing so set its property values) */
   rc = sfcAuthyyparseinstance(*instance);
   if (rc == EOF) {
      _SFCB_TRACE(1,("readNextInstance() : End of config file"));
      *instance = NULL;
      _SFCB_RETURN(EOF);
   } else if (rc != 0) {
      _SFCB_TRACE(1,("readNextInstance() : Error occurred when parsing next instance"));
      *instance = NULL;
      _SFCB_RETURN(0);
   }
   _SFCB_RETURN(1);
}


/*
 * endReadingInstances
 */
static void _endReadingInstances( void * instances )
{
   /* Cleanup the config file */
   if (instances != NULL) {
      fclose(((_FILEINFO *)instances)->handle);
      free(instances);
   }
}

/* ----------------------------------------------------------------------------------- */

/*
 * startWritingInstances
 */
static void * _startWritingInstances()
{
   _FILEINFO * newfileinfo;	/* Filename and handle for the new config file */

   _SFCB_ENTER(TRACE_PROVIDERS, "_startWritingInstances");

   /* Open a temp file to contain the new config file */
   newfileinfo = malloc(sizeof(_FILEINFO));
   tmpnam(newfileinfo->name);
   if ((newfileinfo->handle = fopen(newfileinfo->name,"w")) == NULL) {
      _SFCB_TRACE(1,("startWritingInstances() : Cannot open file %s for writing", newfileinfo->name));
      free(newfileinfo);
      return NULL;
   }
   return newfileinfo;
}


/*
 * writeNextInstance
 */
static int _writeNextInstance( void * newinstances, CMPIInstance * instance )
{
   char * textbuffer = NULL;	/* Buffer containing config file text representation of the instance */
   int rc = 0;			/* Return status */

   if (!CMIsNullObject(instance)) {
      /* Convert the CIM instance to formatted text */
      if (_instance2string(instance, &textbuffer)) {
         /* Append the formatted instance data into the new file */
         if (fputs(textbuffer, ((_FILEINFO *)newinstances)->handle) != EOF) rc = 1;
      } 
      if (textbuffer != NULL) free(textbuffer);
   }
   return rc; 
}


/*
 * endWritingInstances
 */
static void _endWritingInstances( void * newinstances, int commit )
{
   char * command;			/* Shell 'cp' command */
  
   _SFCB_ENTER(TRACE_PROVIDERS, "_endWritingInstances");

   if ( newinstances != NULL) {
     fclose(((_FILEINFO *)newinstances)->handle);

     if (commit) {
        /* Copy the new config file over the original config file */
        _SFCB_TRACE(1,("endWritingInstances() : Commiting changes to config file"));
        command = malloc(strlen(((_FILEINFO *)newinstances)->name)+strlen(_CONFIGFILE)+9);
        sprintf(command, "cp -f %s %s\n", ((_FILEINFO *)newinstances)->name, _CONFIGFILE);
        if (system(command) != 0) {
           _SFCB_TRACE(1,("endWritingInstances() : Failed to overwrite config file with changes"));
	}
        free(command);
     } else {
	_SFCB_TRACE(1,("endWritingInstances() : Config file unchanged"));
     }

     remove(((_FILEINFO *)newinstances)->name);
     free(newinstances);
   }
}

