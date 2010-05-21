/*
 * cimslpUtil.h
 *
 * (C) Copyright IBM Corp. 2006
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:        Sven Schuetz <sven@de.ibm.com>
 * Contributions:
 *
 * Description:
 *
 * some helper functions
 *
 */

#ifndef _cimslpUtil_h
#define _cimslpUtil_h

#include "cmpidt.h"
#include "cmpimacs.h"
/*
 * #define freeArr(arr) \ {int freeStrArrN=0;if (arr) {while
 * (arr[freeStrArrN]) free(arr[freeStrArrN++]);free(arr);}} 
 */
#define freeStr(str) \
	{if (str) free(str);}

void            freeInstArr(CMPIInstance **arr);
void            freeArr(char **arr);

#endif
/* MODELINES */
/* DO NOT EDIT BELOW THIS COMMENT */
/* Modelines are added by 'make pretty' */
/* -*- Mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* vi:set ts=2 sts=2 sw=2 expandtab: */
