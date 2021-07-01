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
#define FREE( ptr ) do { free( ptr ); ptr = NULL; } while (0);
#define DLCLOSE( ptr ) do { if ( ptr != NULL ) { dlclose( ptr ); ptr = NULL; } } while (0);

// defines
#define MAX_PLUGIN_PATH_SIZE   2048
#define MAX_PLUGIN_NAME_SIZE   512
#define PLUGIN_DIRNAME         "plugins" // default directory name
#define PLUGIN_EXTENSION       ".so"     // default plugin extension
#define NOT_DEFINED            "<NOT DEFINED>"

// typedefs and structs
typedef struct {
  char *   func_name;
  uint64_t func_offset;
  void *   func_addr;
} func_t;

typedef struct {
  char path[MAX_PLUGIN_PATH_SIZE];        // plugin path + name with extension
                                          // Ex: /tmp/app/plugins/myplugin.so and
                                          //     /tmp/app/plugins/dir1/myplugin.so
  char name[MAX_PLUGIN_NAME_SIZE];        // base path + plugin name without extension
                                          // Ex: "myplugin" and "dir1/myplugin"
  void * dlhandle;                        // pointer to "dl" functions
  void * base_addr;                       // plugin (.so) base addr when loaded into memory
  func_t * funcs;                         // pointer to funcs
  unsigned short total_funcs;             // number of functions found in plugin (shared lib)
  unsigned char  sha1[SHA_DIGEST_LENGTH]; // plugin (.so) SHA1 hash
} plugin_t;

typedef struct {
  plugin_t * plugin;
  char path[MAX_PLUGIN_PATH_SIZE];        // plugin path without plugin name
  unsigned short number_of_plugins;
} cpf_t;

cpf_t *  CPF_init( char * directory_name );
void     CPF_cleanup( cpf_t ** cpf );
void *   CPF_call_func_by_addr( void * func_addr, enum func_prototype_t fproto, ... );
void *   CPF_call_func_by_name( cpf_t * cpf, char * plugin_name, char * func_name, enum func_prototype_t fproto, ... );
void *   CPF_call_func_by_offset( cpf_t * cpf, char * plugin_name, uint64_t func_offset, enum func_prototype_t fproto, ... );
void *   CPF_get_func_addr( cpf_t * cpf, char * plugin_name, char * func_name );
uint64_t CPF_get_func_offset( cpf_t * cpf, char * plugin_name, char * func_name );
void     CPF_print_loaded_libs( cpf_t * cpf );
int      CPF_reload_libs( cpf_t ** cpf, bool display_report );
void     CPF_unload_libs( cpf_t * cpf );

#endif