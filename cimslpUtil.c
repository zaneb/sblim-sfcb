/*
 * cimslpUtil.c
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

#include "cimslpUtil.h"

#include <stdlib.h>

/*
 * void freeStrArr(char ** arr) { int n=0; if (arr) {while (arr[n])
 * free(arr[n++]);free(arr);} }
 */

void
freeInstArr(CMPIInstance **arr)
{
  int             i = 0;
  if (arr) {
    while (arr[i]) {
      CMRelease(arr[i]);
      i++;
    }
    free(arr);
  }
}

void
freeArr(char **arr)
{
  int             i = 0;
  if (arr) {
    while (arr[i]) {
      free(arr[i]);
      i++;
    }
    free(arr);
  }
}
/* MODELINES */
/* DO NOT EDIT BELOW THIS COMMENT */
/* Modelines are added by 'make pretty' */
/* -*- Mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* vi:set ts=2 sts=2 sw=2 expandtab: */
