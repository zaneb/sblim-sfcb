/*
 * utilTypeCk.c
 *
 * (C) Copyright IBM Corp. 2011
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:        Narasimha Sharoff <nsharoff@us.ibm.com>
 *
 * Description:
 *
 * CIM type validation routines : return 1 on failure
 *
 */

#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <errno.h>
#include "cmpi/cmpidt.h"

/* checks for unsigned integers uint8 - uint64 */
int 
invalid_uint(const char *v, const CMPIType type)
{
	unsigned long long int a = 0L;
	int base = 0; /* allows decimal, octal, and hex values */
	char *endptr = NULL;
	int rc = 0;

	if (strlen(v) == 0) return 1;
	errno = 0;
	a = strtoull(v, &endptr, base);
	if ((errno == ERANGE && (a == ULONG_MAX))
	    || (errno != 0 && a == 0)) return 1;
	if (endptr[0] != '\0') return 1;
	switch (type) {
	case CMPI_uint8:
		if ( a > UCHAR_MAX ) rc = 1;
		break;
	case CMPI_uint16:
		if ( a > USHRT_MAX ) rc = 1;
		break;
	case CMPI_uint32:
		if ( a > UINT_MAX ) rc = 1;
		break;
	case CMPI_uint64:
		if ( a > ULONG_MAX ) rc = 1;
		break;
	default:
		rc = 1;
		break;
	}
	return rc;
}

/* checks for integers int8 - int64 */
int 
invalid_int(const char *v, const CMPIType type)
{
	long long int a = 0L;
	int base = 0;
	char *endptr = NULL;
	int rc = 0;

	if (strlen(v) == 0) return 1;
	errno = 0;
	a = strtoll(v, &endptr, base);
	if (endptr[0] != '\0') return 1;
	if ((errno == ERANGE && (a == LONG_MAX || a == LONG_MIN))
	    || (errno != 0 && a == 0)) return 1;
	switch (type) {
	case CMPI_sint8:
		if ( a > SCHAR_MAX || a < SCHAR_MIN ) rc = 1;
		break;
	case CMPI_sint16:
		if ( a > SHRT_MAX || a < SHRT_MIN ) rc = 1;
		break;
	case CMPI_sint32:
		if ( a > INT_MAX  || a < INT_MIN ) rc = 1;
		break;
	case CMPI_sint64:
		if ( a > LONG_MAX  || a < LONG_MIN ) rc = 1;
		break;
	default:
		rc = 1;
		break;
	}
	return rc;
}

/* checks for real32 and real64 */
int 
invalid_real(const char *v, const CMPIType type)
{
	float a = 0;
	double d = 0;
	char *endptr = NULL;
	int rc = 0;

	if (strlen(v) == 0) return 1;
	errno = 0;
        switch (type) {
	   case CMPI_real32:
		a = strtof(v, &endptr);
		if (endptr[0] != '\0') return 1;
		if ((a == 0) && (v == endptr)) return 1;
		if ((errno == ERANGE /*&& (a == HUGE_VALF || a == -HUGE_VALF)*/)
		    || (errno != 0 && a == 0)) return 1;
		break;
	   case CMPI_real64:
		d = strtod(v, &endptr);
		if (endptr[0] != '\0') return 1;
		if ((d == 0) && (v == endptr)) return 1;
		if ((errno == ERANGE /*&& (a == HUGE_VAL || a == -HUGE_VAL)*/)
		    || (errno != 0 && d == 0)) {printf("Nsn\n");return 1;}
		break;
	   default:
		rc = 1;
		break;
	}
	return rc;
}

/* case insensitive check for boolean "true" or "false" */
int
invalid_boolean(const char *v, const CMPIType type)
{
	if ((strcasecmp(v, "false") == 0) ||
		(strcasecmp(v, "true") == 0)) return 0;

	return 1;
}

/* MODELINES */
/* DO NOT EDIT BELOW THIS COMMENT */
/* Modelines are added by 'make pretty' */
/* -*- Mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* vi:set ts=2 sts=2 sw=2 expandtab: */
