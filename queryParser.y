/*
**==============================================================================
**
** Includes
**
**==============================================================================
*/


%{

/*
 * queryParser.l
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
 * Based on concepts from pegasus project 
 *
 * Description:
 *
 * WQL query parser for sfcb.
 *
*/


//%2005////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, 2001, 2002 BMC Software; Hewlett-Packard Development
// Company, L.P.; IBM Corp.; The Open Group; Tivoli Systems.
// Copyright (c) 2003 BMC Software; Hewlett-Packard Development Company, L.P.;
// IBM Corp.; EMC Corporation, The Open Group.
// Copyright (c) 2004 BMC Software; Hewlett-Packard Development Company, L.P.;
// IBM Corp.; EMC Corporation; VERITAS Software Corporation; The Open Group.
// Copyright (c) 2005 Hewlett-Packard Development Company, L.P.; IBM Corp.;
// EMC Corporation; VERITAS Software Corporation; The Open Group.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// THE ABOVE COPYRIGHT NOTICE AND THIS PERMISSION NOTICE SHALL BE INCLUDED IN
// ALL COPIES OR SUBSTANTIAL PORTIONS OF THE SOFTWARE. THE SOFTWARE IS PROVIDED
// "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "queryOperation.h"
#include "mlog.h"

#define YYPARSE_PARAM parm
#define YYLEX_PARAM parm
#define YYERROR_VERBOSE 1 
 
extern int yylex();
extern void sfcQueryErr(char*,char*,char*);
extern void yyError(char*);
extern void yyerror(char*);
extern void sfcQueryError(char* s1);
extern char * qsStrDup(QLStatement *qs, char *str);
extern QLOperand* newNameQueryOperand(QLStatement *qs, char* val);


#define sfcQueryerror sfcQueryError
#define yyErr sfcQueryErr

#define QS ((QLControl*)parm)->statement
#define QC ((QLControl*)parm)->collector

%}

/*
**==============================================================================
**
** Union used to pass tokens from Lexer to this Parser.
**
**==============================================================================
*/

%union
{
   long intValue;
   double doubleValue;
   char* strValue;
   void* nodeValue;
   
   QLOperand* propertyName;
   QLOperand* comparisonTerm;
   QLOperation* searchCondition;
   QLOperation* predicate;
}

/*
**==============================================================================
**
** Tokens, types, and associative rules.
**
**==============================================================================
*/

%token <intValue> TOK_INTEGER
%token <doubleValue> TOK_DOUBLE

%token <strValue> TOK_STRING
%token <intValue> TOK_TRUE
%token <intValue> TOK_FALSE
%token <intValue> TOK_NULL

%token <intValue> TOK_EQ
%token <intValue> TOK_NE
%token <intValue> TOK_LT
%token <intValue> TOK_LE
%token <intValue> TOK_GT
%token <intValue> TOK_GE

%token <intValue> TOK_NOT
%token <intValue> TOK_OR
%token <intValue> TOK_AND
%token <intValue> TOK_IS
%token <intValue> TOK_ISA

%token <strValue> TOK_REF
%token <strValue> TOK_KEY
%token <strValue> TOK_CLASS
%token <strValue> TOK_NAMESPACE
%token <strValue> TOK_VERSION

%token <strValue> TOK_IDENTIFIER
%token <intValue> TOK_SELECT
%token <intValue> TOK_WHERE
%token <intValue> TOK_FROM

%token <intValue> TOK_UNEXPECTED_CHAR

%type <propertyName>  classPropertyName
%type <strValue>  propertyIdentifier
%type <strValue> classAlias

%type <comparisonTerm>  classId
%type <comparisonTerm>  comparisonTerm
%type <searchCondition> searchCondition
%type <whereClause> whereClause

%type <predicate> predicate
%type <predicate> comparisonPredicate
%type <predicate> nullPredicate

%type <strValue> propertyName
%type <nodeValue> propertyList
%type <nodeValue> fromClause
%type <nodeValue> selectStatement
%type <nodeValue> selectList
%type <nodeValue> selectExpression
%type <strValue> className
%type <intValue> truthValue

%left TOK_OR
%left TOK_AND
%nonassoc TOK_NOT

%%

/*
**==============================================================================
**
** The grammar itself.
**
**==============================================================================
*/

start
    : selectStatement
    {
    }

selectStatement
    : TOK_SELECT selectList selectExpression
    {

    }

selectList
    : '*'
    {
       QS->ft->setAllProperties(QS,1);
    }
    | propertyList
    {

    }

propertyList
    : propertyName
    {
       QS->ft->appendSelectPropertyName(QS,$1);
    }
    | propertyList ',' propertyName
    {
       QS->ft->appendSelectPropertyName(QS,$3);
    }

selectExpression
    : fromClause whereClause
    {

    }
    | fromClause
    {
    }

fromClause
    : TOK_FROM classList
    {
    }

classList
    : classListEntry
    {
    }
    | classList ',' classListEntry
    {
    }
   
classListEntry 
    : className classAlias
    {
       QS->ft->addFromClass(QS,$1,$2);
    }
    | className
    {
        QS->ft->addFromClass(QS,$1,NULL);
    }
    
whereClause
    : TOK_WHERE searchCondition
    {
       QS->ft->setWhereCondition(QS,(QLOperation*)$2);
    }

searchCondition
    : searchCondition TOK_OR searchCondition
    {
       $$=newOrOperation(QS,$1,$3);
       QL_TRACE(fprintf(stderr,"- - searchCondition: %p (%p-%p)\n",$$,$1,$3));
    }
    | searchCondition TOK_AND searchCondition
    {
       $$=newAndOperation(QS,$1,$3);

    }
    | TOK_NOT searchCondition
    {
       $$=newNotOperation(QS,$2);
    }
    | '(' searchCondition ')'
    {
       $$=$2;
    }
    | predicate
    {
       $$=newBinaryOperation(QS,$1);
    }
    | predicate TOK_IS truthValue
    {
       if (QS->wql) {
          if ($3);
          else $1->ft->eliminateNots($1,1);
          $$=newBinaryOperation(QS,$1);
      }
       else {
          yyErr("Three state logic not supported: IS ",$3?"TRUE":"FALSE","");
          YYERROR;
       }
    }
    | predicate TOK_IS TOK_NOT truthValue
    {
       if (QS->wql) {
          if ($4) $1->ft->eliminateNots($1,1);
           $$=newBinaryOperation(QS,$1);
      }
       else {
          yyErr("Three state logic not supported: IS NOT ",$4?"TRUE":"FALSE","");
          YYERROR;
       }
    }

truthValue
    : TOK_TRUE
    {
       $$ = 1;
    }
    | TOK_FALSE
    {
       $$ = 0;
    }


/******************************************************************************/

predicate
    : comparisonPredicate
    {
       $$=$1;
    }
    | nullPredicate
    {
       $$=$1;
    }

comparisonPredicate
    : comparisonTerm TOK_LT comparisonTerm
    {
       $$=newLtOperation(QS,$1,$3);
    }
    | comparisonTerm TOK_GT comparisonTerm
    {
       $$=newGtOperation(QS,$1,$3);
    }
    | comparisonTerm TOK_LE comparisonTerm
    {
       $$=newLeOperation(QS,$1,$3);
    }
    | comparisonTerm TOK_GE comparisonTerm
    {
       $$=newGeOperation(QS,$1,$3);
    }
    | comparisonTerm TOK_EQ comparisonTerm
    {
       $$=newEqOperation(QS,$1,$3);
       QL_TRACE(fprintf(stderr,"- - comparisonPredicate: %p (%p-%p)\n",$$,$1,$3));
    }
    | comparisonTerm TOK_NE comparisonTerm
    {
       $$=newNeOperation(QS,$1,$3);
    }
    | comparisonTerm TOK_ISA classId
    {
       $$=newIsaOperation(QS,$1,$3);
    }

nullPredicate
    : comparisonTerm TOK_IS TOK_NULL
    {
       $$=newIsNullOperation(QS,$1);
    }
    | comparisonTerm TOK_IS TOK_NOT TOK_NULL
    {
       $$=newIsNotNullOperation(QS,$1);
    }


classPropertyName
    : className '.' classPropertyNameList
    {
       if (QS->wql) {
          mlogf(M_ERROR,M_SHOW,"components ?\n"); 
//          yyErr("Bad property-identifier-1: ",$1,"...");
//          YYERROR;
          $$=QC->addPnClass(QC,QS,$1,0);
          QC->resetName(QC);
       }
       else if (QS->ft->testPropertyClass(QS,$1)==0) {
          yyErr("class-identifier not in From clause: ",$1,"");
          YYERROR;
       }
       else {
          $$=QC->addPnClass(QC,QS,$1,0);
          QC->resetName(QC);
       }
    }
    | propertyIdentifier
    {
       if (QS->wql) {
          $$=QC->addPnClass(QC,QS,$1,1);
          QC->resetName(QC);
       }
       else {
          yyErr("Bad class-property-identifier-2: ",$1,"");
          YYERROR;
       }
    }

classPropertyNameList
    : propertyIdentifier
    {
    }
    | classPropertyNameList '.' propertyIdentifier
    {
    }

PropertyName
    : TOK_IDENTIFIER
    {
       QC->addPnPart(QC,QS,$1);
    }

propertyIdentifier
    : PropertyName
    {
    }
    | TOK_KEY
    {
    }
    | TOK_CLASS
    {
    }
    | TOK_VERSION
    {
    }
    | TOK_NAMESPACE
    {
    }


propertyName
    : TOK_IDENTIFIER
    {
    }

className : TOK_IDENTIFIER
    {
       $$ = $1;
    }
    
classAlias : TOK_IDENTIFIER
    {
       $$ = $1;
    }


classId
    : className
    {
       $$=newNameQueryOperand(QS,$1);
    }

comparisonTerm
    : classPropertyName
    {
       $$=$1;
    }
    | TOK_INTEGER
    {
       $$=newIntQueryOperand(QS,$1);
    }
    | TOK_DOUBLE
    {
       $$=newDoubleQueryOperand(QS,$1);
    }
    | TOK_STRING
    {
       $$=newCharsQueryOperand(QS,qsStrDup(QS,$1));
       free($1);
       
    }
    | truthValue
    {
       $$=newBooleanQueryOperand(QS,$1);
    }

%%
