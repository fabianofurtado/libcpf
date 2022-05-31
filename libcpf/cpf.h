/*
  libcpf - C Plugin Framework

  cpf.h - header file

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
#ifndef __CPF_H__
#define __CPF_H__

#include <openssl/sha.h>
#include <stdbool.h>
#include "fp_prototype.h"
#include "log.h"

// macros
#define FREE( ptr ) do { free( ptr ); ptr = NULL; } while (0); // avoid dangling pointer
#define DLCLOSE( ptr ) do { if ( ptr != NULL ) { dlclose( ptr ); ptr = NULL; } } while (0);

// defines
#define LIBCPF_VERSION          "0.0.4"
#define MAX_PLUGIN_PATH_SIZE    2048
#define MAX_PLUGIN_NAME_SIZE    512
#define MAX_VERSIN_SIZE_NAME    64
#define PLUGIN_DIRNAME          "plugins"         // default directory name
#define PLUGIN_EXTENSION        ".so"             // default plugin extension
#define PLUGIN_INIT_CTX_FUNC    "CPF_init_ctx"    // default plugin init context func name
#define PLUGIN_CONSTRUCTOR_FUNC "CPF_constructor" // default plugin constructor func name
#define PLUGIN_DESTRUCTOR_FUNC  "CPF_destructor"  // default plugin destructor func name
#define NOT_DEFINED             "<NOT DEFINED>"


// typedefs and structs
typedef struct {                          // functions definitions
  void *   func_addr;                     // 1st struct field!!!
  uint64_t func_offset;
  char *   func_name;                     // Can be NULL! Can't be 1st struct element!!!
} func_t;

typedef struct {                          // Dependencies
  char   * dep_lib_name;                  // 1st struct field!!!
  func_t * funcs;
} deps_t;

typedef struct {
  char     version[MAX_VERSIN_SIZE_NAME]; // optional plugin version
  deps_t * deps;
} plugin_ctx_t;

typedef struct {
  void         * dlhandle;                // ptr to "dl" functions
  void         * base_addr;               // plugin (.so) base addr when loaded into memory
  func_t       * lib_func;                // ptr to library functions
  plugin_ctx_t * ctx;                     // ptr to plugin context, defined inside the lib
  void         * ctor;                    // ptr to PLUGIN_CONSTRUCTOR_FUNC function
  void         * dtor;                    // ptr to PLUGIN_DESTRUCTOR_FUNC function
  void         * init_ctx;                // ptr to PLUGIN_INIT_CTX_FUNC (init context) fcn
  uint8_t  sha1[SHA_DIGEST_LENGTH];       // plugin (.so) SHA1 hash
  char     path[MAX_PLUGIN_PATH_SIZE];    // plugin path + name with extension
                                          // Ex: /tmp/app/plugins/myplugin.so and
                                          //     /tmp/app/plugins/dir1/myplugin.so
  char     name[MAX_PLUGIN_NAME_SIZE];    // base path + plugin name without extension
                                          // Ex: "myplugin" and "dir1/myplugin"
} plugin_t;

typedef struct {
  plugin_t * plugin;
  char path[MAX_PLUGIN_PATH_SIZE];        // plugin path without plugin name
  uint16_t num_plugins;                   // number of plugins loaded
} cpf_t;

// constructor and destructor typedef
typedef void ( *ctor_dtor_t ) ( plugin_t * );


extern cpf_t *   CPF_init( char * directory_name );
extern void      CPF_call_ctor( cpf_t * cpf );
extern void      CPF_free( cpf_t ** cpf );
extern void      CPF_call_dtor( cpf_t * cpf );
extern void *    CPF_call_func_by_addr( void * func_addr,
                                        enum func_prototype_t fproto,
                                        ... );
extern void *    CPF_call_func_by_name( cpf_t * cpf, char * plugin_name,
                                        char * func_name,
                                        enum func_prototype_t fproto,
                                        ... );
extern void *    CPF_call_func_by_offset( cpf_t * cpf,
                                          char * plugin_name,
                                          uint64_t func_offset,
                                          enum func_prototype_t fproto,
                                          ... );
extern void *    CPF_get_func_addr( cpf_t * cpf, char * plugin_name, char * func_name );
extern void *    CPF_get_extern_lib_func_by_dep( deps_t * d,
                                                 char * plugin_name,
                                                 char * func_name );
extern uint64_t  CPF_get_func_offset( cpf_t * cpf, char * plugin_name, char * func_name );
extern void      CPF_print_loaded_libs( cpf_t * cpf );
extern int       CPF_reload_libs( cpf_t ** cpf, bool display_report );
extern void      CPF_unload_libs( cpf_t * cpf );

#endif