/*
  libcpf - C Plugin Framework

  fp_prototype.c - functions prototyes to be defined

  Copyright (C) 2021 libcpf authors

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "fp_prototype.h"
#include "log.h"

//////////////////////////////////////////////////////////////////////////
// Function pointer prototypes must be defined here with FP_WRAPPER macro!

// Fuction prototype: "char * FP_CHARPTR_wrapper( void* func_addr );"
FP_WRAPPER( FP_CHARPTR, char *, (void* func_addr), (), () )
// Fuction prototype: "int FP_INT_INT_wrapper( void* func_addr, int i );"
FP_WRAPPER( FP_INT_INT, int, (void* func_addr, int i), (int), (i) )
// Fuction prototype: "void * VOIDPTR_CHARPTR_INT_wrapper( void* func_addr, char * c, int i );"
FP_WRAPPER( FP_VOIDPTR_CHARPTR_INT,
            void *,
            (void * func_addr, char * c, int i),
            (char *, int),
            (c, i) )
//////////////////////////////////////////////////////////////////////////


void *
CPF_wrapper_call_func_by_addr( void * func_addr,
                               enum func_prototype_t fproto,
                               va_list varglist )
{
  void * ret = NULL;

  switch( fproto ) {
    case FP_INT_INT: {
        int    i   = va_arg( varglist, int );
        ret = (void *)(uint64_t)FP_INT_INT_wrapper ( func_addr, i );
      } break;
    case FP_CHARPTR: {
        ret = (void *)FP_CHARPTR_wrapper ( func_addr );
      } break;
    case FP_VOIDPTR_CHARPTR_INT: {
        char * cp  = va_arg( varglist, char * );
        int    i   = va_arg( varglist, int );
        ret = FP_VOIDPTR_CHARPTR_INT_wrapper ( func_addr, cp, i );
      } break;
    default:
      LOG_ERROR( "Function prototype not found!" )
      break;
  }

  return ret;
}