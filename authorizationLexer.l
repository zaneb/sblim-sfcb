%{

/*
 * authorizationLexer.l
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
 *
 * Authorization provider lexer for sfcb
 *
*/


#include "cmpidt.h"
#include <string.h>

#include "authorizationParser.h"

#ifndef YY_FLEX_LEX_COMPAT
int yylineno = 1;
#endif 
%} 

		/* Some useful regular expressions to use in the RULES section */
DIGIT		([0-9])
ALPHA		([A-Za-z])
ALPHANUMERIC	([A-Za-z0-9])
PUNCTUATION	([\,\.\:\(\)\-\!\"\'\`\^])
ARITHMETIC	([\+\-\*\/\<\>\=])
WHITESPACE	([\t ]+)
EOLN		(\n)
NUMBER		([0-9]+)
INTEGER		(-?[0-9]+)
REAL		(-?[0-9]+\.[0-9]+)
WORD		([A-Za-z]+)
QUOTEDTEXT	(\"[^"\n]*["\n])
FILENAME	([A-Za-z0-9\.][A-Za-z0-9\.\-\+\_]*)
PATHNAME	(\/?[A-Za-z0-9\.][A-Za-z0-9\.\-\+\_]*(\/[A-Za-z0-9\.][A-Za-z0-9\.\-\+\_]*)*\/?) 
HOSTNAME	([A-Za-z][A-Za-z0-9\-\_]*(\.[A-Za-z][A-Za-z0-9\-\_]*)*) 
IPADDR		([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)

	/* Special mode to read CLASSNAME after read in USERNAME (otherwise cannot distinguish these tokens) */
%x READCLASSNAME 

	/* Use the default yywrap() behavior */
%option  noyywrap

	/* END OF DEFINITIONS SECTION */
%%
	/* RULES SECTION */
	/* DESCRIBE EVERY LEXICAL TOKEN/PATTERN FOLLOWED BY THE APPROPRIATE ACTION.
	   TYPICALLY THE ACTION RETURNS THE TOKEN IDENTIFIER AND ITS VALUE (IN YYLVAL) */

	/* The following simple lexical tokens only return the token identifier */

Enumerate |
enumerate |
enum |
enumerateinstances |
ei |
enumerateinstancenames |
ein	{
	return(ENUMERATE);
	}

getinstance | 
gi |
Get |
get 	{
	return(GET);  
	}

setinstance |
si |
modifyinstance |
mi |
Set |
set 	{
	return(SET);
	}

createinstance |
ci |
Create |
create	{
	return(CREATE);
	}

deleteinstance |
di |
Delete |
delete	{
	return(DELETE);
	}

queryinstances |
qi |
execquery |
eq |
exec |
Query |
query	{
	return(QUERY);
	}

	/* The following lexical tokens return the token identifier and the value of the token (in yylval) */

true |
TRUE |
yes |
YES 	{
	yylval.boolean = 1;
	return(PERMISSION);
	}

false |
FALSE |
no |
NO 	{
	yylval.boolean = 0;
	return(PERMISSION);
	}
	
        /* Username */
[A-Za-z][A-Za-z0-9]* {
	BEGIN READCLASSNAME; /* Read in classname as next token */ 
	yylval.string = (char *)strdup(yytext);
	return(USERNAME);
	}

	/* Classname */
	/* NOTE - this rule only applies after a USERNAME has been read in */
<READCLASSNAME>[A-Za-z][A-Za-z0-9_]* {
	BEGIN INITIAL; /* Go back to normal parsing rules now */
	yylval.string = (char *)strdup(yytext);
	return(CLASSNAME);
	}

	/* Ignore end-of-line */
{EOLN}	{
	yylineno++;
	}

<<EOF>> {
	return(ENDOFFILE);
	}
                                                                                                       
	/* Ignore whitespace */
{WHITESPACE} ;

	/* Return single character punctuation characters as-is */
.       {
	return(yytext[0]);
	}

	/* END OF RULES SECTION */
%%
	/* USER SUBROUTINE SECTION */
	/* Everything below is copied verbatim to the end of the lex generated C code. */

void yyerror(char *errmsg)
{
   fprintf(stderr, "error line %d: %s in '%s'\n", yylineno, errmsg, yytext);
}

