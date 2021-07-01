/*
  libcpf - C Plugin Framework

  fp_prototype.h - header file

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
#ifndef __FP_PROTOTYPE_H__
#define __FP_PROTOTYPE_H__

/*
****************************************************************
  CREATING A FUNCTION PROTOTYPE
****************************************************************

Let's create a function prototype for "void * func_name(char * c, int i);"

This function returns a void pointer (void *) and has a char pointer c (char * c)
and an integer i (int i) as parameters.
To make the process easier, we'll create a format/pattern to define the function
pointer reference, formed by the return type and the parameters type.
For the "func_name()" above, this format/pattern is: VOIDPTR_CHARPTR_INT

1) We need to modify the func_prototype_t enum, adding this new format/pattern
   to enumerated data list: the FP_VOIDPTR_CHARPTR_INT entry:

  enum func_prototype_t {
    FP_UNDEFINED=0,
    FP_INT_INT,
    FP_CHARPTR,
    FP_VOIDPTR_CHARPTR_INT  // new entry
  };

  This enum entry represents the format/pattern "function's return type (VOIDPTR)
  followed by '_', the first parameter (CHARPTR) followed by '_', and the second
  parameter (INT)", and it will be used later in CPF_wrapper_call_func_by_addr().

2) In "fp_prototype.c" file, add a FP_WRAPPER macro:

  FP_WRAPPER( FP_VOIDPTR_CHARPTR_INT, void *, (void * func_addr, char * c, int i), (char *, int), (c, i) )

  * 1st parameter: enum func_prototype_t, in the enums format/pattern described above.
  * 2nd parameter: functions return type
  * 3rd parameter: "void * func_addr" followed by function parameters prototype "char * c, int i"
  * 4th parameter: function parameters datatypes "(char *, int)"
  * 5th parameter: function parameters variables "(c, i)"

  This macro creates the "FP_VOIDPTR_CHARPTR_INT_wrapper" function and it expands to

  static void * FP_VOIDPTR_CHARPTR_INT_wrapper (void * func_addr, char * c, int i) {
    void * (*FP_VOIDPTR_CHARPTR_INT_t)(char *, int) = func_addr;
    return (*FP_VOIDPTR_CHARPTR_INT_t)(c, i);
  }

  After the expansion, the macro format became more clear to understand.

3) In "fp_prototype.c" file, modify the CPF_wrapper_call_func_by_addr() function,
   adding a case statement to call the specific wrapper function, previously
   created with FP_WRAPPER macro (see item 2):
  ...
  case FP_VOIDPTR_CHARPTR_INT: {
      char * cp  = va_arg( varglist, char * );
      int    i   = va_arg( varglist, int );
      ret = FP_VOIDPTR_CHARPTR_INT_wrapper ( func_addr, cp, i );
    } break;
  ...

  The name of the wrapper function is formed by enum func_prototype_t, followed by
  "_wrapper".

  The optional parameters must be set before the wrapper function call, binding
  each parameter with one variable declared inside the case statement. The
  variable data type is important to bind it correctly.

  The "{" and "}" are important to avoid variable name conflict with other "case"
  declarations.

  You may need to typecast the return of the wrapper function, in order to set
  the "ret" variable correctly. In this specific case, it wasn't necessary because
  the wrapper function returns "void *" and the "ret" variable is declared as
  "void *".

4) You are not required to use the wrapper function call, if you don't want to.
   Instead of that, you can use the classic way to call a function pointer:
    ...
    case FP_VOIDPTR_CHARPTR_INT: {
        char * cp  = va_arg( varglist, char * );
        int    i   = va_arg( varglist, int );
        void * (*some_func_prototype_t)(char *, int) = func_addr;
        ret = (*some_func_prototype_t)( cp, i );
      } break;
    ...

****************************************************************
Here's another example using integer pointer as a parameter:
This FP_WRAPPER macro...
FP_WRAPPER( FP_INT_INT_INTPTR,
            int,
            (void * func_addr, int i, int * ip),
            (int, int *),
            (i, &ip)
          )
... will expand to ...
static int INT_INT_INTPTR_wrapper (void * func_addr, int i, int * ip) {
  int (*INT_INT_INTPTR_t)(int, int *) = func_addr;
  return (*INT_INT_INTPTR_t)(i, &ip);
}
****************************************************************
*/


// macros
#define FP_WRAPPER( FUNC_PROTOTYPE_T, FUNC_RETURN_TYPE, FUNC_HEADER, FUNC_ARGS_TYPE, FUNC_ARGS_VAR ) \
  static FUNC_RETURN_TYPE FUNC_PROTOTYPE_T ## _wrapper FUNC_HEADER { \
    FUNC_RETURN_TYPE (*FUNC_PROTOTYPE_T ## _t)FUNC_ARGS_TYPE = func_addr; \
    return (*FUNC_PROTOTYPE_T ## _t)FUNC_ARGS_VAR; \
  }


enum func_prototype_t {
  FP_UNDEFINED=0,
  FP_INT_INT,
  FP_CHARPTR,
  FP_VOIDPTR_CHARPTR_INT
};

/*
// NOT USED!
enum param_type_t {
  PT_UNDEFINED=0,
  PT_VOID,
  PT_BOOL,
  PT_CHAR,
  PT_UNSIGNED_CHAR,
  PT_SHORT_INT,
  PT_UNSIGNED_SHORT_INT,
  PT_INT,
  PT_UNSIGNED_INT,
  PT_LONG,
  PT_UNSIGNED_LONG,
  PT_FLOAT,
  PT_DOUBLE,
  PT_LONG_DOUBLE,
  PT_POINTER_TO_VOID,
  PT_POINTER_TO_CHAR,
  PT_POINTER_TO_UNSIGNED_CHAR,
  PT_POINTER_TO_INT,
  PT_POINTER_TO_UNSIGNED_INT,
  PT_POINTER_TO_SHORT_INT,
  PT_POINTER_TO_UNSIGNED_SHORT_INT,
  PT_POINTER_TO_LONG,
  PT_POINTER_TO_UNSIGNED_LONG,
  PT_POINTER_TO_FLOAT,
  PT_POINTER_TO_DOUBLE,
  PT_POINTER_TO_LONG_DOUBLE
};
*/

void * CPF_wrapper_call_func_by_addr( void * func_addr,
                                      enum func_prototype_t fproto,
                                      va_list varglist );


#endif