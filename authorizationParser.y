/* DEFINITIONS SECTION */
/* Everything between %{ ... %} is copied verbatim to the start of the parser generated C code. */

%{

/*
 * authorizationParser.y
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
 *
 * Description:
 * Authorization provider lexer for sfcb
 *
*/

#include <stdlib.h>
#include "cmpimacs.h"			/* Contains CMSetProperty() */

#define RC_OK 0
#define RC_EOF EOF 
                                                                                                       
/* DEFINE ANY GLOBAL VARS HERE */
static CMPIInstance * _INSTANCE;	/* The current instance that is being read into */
%}

/* List all possible CIM property types that can be returned by the lexer */
%union { 
   CMPIBoolean          boolean;
   /* Note - we override the CIM definition of string to make this data type
      easier to handle in the lexer/parser. Instead implemented as simple text string. */
   char *		string;
}

/* DEFINE SIMPLE (UNTYPED) LEXICAL TOKENS */
%token ENUMERATE GET SET CREATE DELETE QUERY ENDOFFILE

/* DEFINE LEXICAL TOKENS THAT RETURN A VALUE AND THEIR RETURN TYPE (OBTAINED FROM THE %union ABOVE) */
%token <string> USERNAME
%token <string> CLASSNAME
%token <boolean> PERMISSION

/* END OF DEFINITIONS SECTION */
%%

/* RULES SECTION */
/* DESCRIBE THE SYNTAX OF EACH INSTANCE ENTRY IN THE SOURCE FILE */

/* Each authorization stanza contains the username, classname, followed by a list of permissions */
instance:	/* empty */
	|	'[' USERNAME CLASSNAME ']'
			{
			CMSetProperty( _INSTANCE, "Username", $2, CMPI_chars );
			free($2); /* must free string because it was originally strdup'd */
                        CMSetProperty( _INSTANCE, "Classname", $3, CMPI_chars );
			free($3); /* must free string because it was originally strdup'd */
			}
		permissions
			{
			/* Return after reading in each instance */
                        return RC_OK;
			}

	| 	ENDOFFILE { return RC_EOF; }
	;

/* List of class operation permissions */
permissions:	/* empty */
	|	'#'
	|	ENDOFFILE
	|	permission
	|	permission permissions
	;

permission:	ENUMERATE ':' PERMISSION
			{
                        CMSetProperty( _INSTANCE, "Enumerate", &($3), CMPI_boolean );
			}
	|	GET ':' PERMISSION
			{
                        CMSetProperty( _INSTANCE, "Get", &($3), CMPI_boolean );
			}
	|	SET ':' PERMISSION
			{
                        CMSetProperty( _INSTANCE, "Set", &($3), CMPI_boolean );
			}
	|	CREATE ':' PERMISSION
			{
                        CMSetProperty( _INSTANCE, "Create", &($3), CMPI_boolean );
			}
	|	DELETE ':' PERMISSION
			{
                        CMSetProperty( _INSTANCE, "Delete", &($3), CMPI_boolean );
			}
	|	QUERY ':' PERMISSION
			{
                        CMSetProperty( _INSTANCE, "Query", &($3), CMPI_boolean );
			}
	;

/* END OF RULES SECTION */
%%

/* USER SUBROUTINE SECTION */

int sfcAuthyyparseinstance( CMPIInstance * instance )
{
   _INSTANCE = instance;
   /* Parse the next instance */
   return(sfcAuthyyparse());
}

