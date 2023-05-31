/*
  libcpf - C Plugin Framework

  plugin_manager.c - bind and load plugins to memory

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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <link.h>
#include <dirent.h>
#include <dlfcn.h>
#include <elf.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "plugin_manager.h"
#include "log.h"
#include "blake2.h"


static int
comparator( const void * p, const void * q )
{
  return strncmp( ( (plugin_t *)p)->path,
                  ( (plugin_t *)q)->path,
                  sizeof( ( (plugin_t *)p)->path ) );
}


void
sort_plugins( cpf_t * cpf )
{
  if ( ( cpf != NULL ) && ( cpf->num_plugins > 1 ) ) {
    qsort( cpf->plugin, cpf->num_plugins, sizeof( plugin_t ), comparator );
  }
}


void
load_plugins_2_reload( cpf_t * cpf )
{
  ElfW(Ehdr)      * elf_header;
  //ElfW(Phdr)      * prog_header;
  ElfW(Dyn)       * dynamic;
  ElfW(Sym)       * symtable;
  void            * strtable;
  void            * fcn_addr;
  uint16_t          i, j,
                    num_funcs,
                    p_count, // plugin counter
                    symtbltotalsize,
                    symtblentrysize = 0;
  struct link_map * lnkmap;


  if ( cpf == NULL ) {
    return;
  }
  if ( cpf->num_plugins == 0 ) {
    return;
  }

  for( p_count = 0 ; p_count < cpf->num_plugins ; p_count++ ) {
    num_funcs = 0;
    cpf->plugin[p_count].dlhandle =
      dlopen( cpf->plugin[p_count].path, RTLD_NOW | RTLD_GLOBAL );
    if ( cpf->plugin[p_count].dlhandle == NULL ) {
      LOG_ERROR( "dlopen(): %s", dlerror() )
      exit( EXIT_FAILURE );
    }

    if ( dlinfo(cpf->plugin[p_count].dlhandle, RTLD_DI_LINKMAP, &lnkmap) == -1 ) {
      LOG_ERROR( "RTLD_DI_LINKMAP failed: %s", dlerror() )
      exit( EXIT_FAILURE );
    }

    cpf->plugin[p_count].base_addr = (void *)lnkmap->l_addr;
    dynamic = lnkmap->l_ld;

    elf_header = (ElfW(Ehdr)*) cpf->plugin[p_count].base_addr;
	
    /* ELF magic number */
    if (memcmp(elf_header->e_ident, ELFMAG, SELFMAG) != 0) {
      LOG_ERROR( "ELF magic number not found!" )
      dlclose(cpf->plugin[p_count].dlhandle);
      exit( EXIT_FAILURE );
    }
	
    // AMD x86-64 architecture only
    if ( elf_header->e_machine != EM_X86_64 ) {
      LOG_ERROR( "Architecture not compatible!" )
      dlclose(cpf->plugin[p_count].dlhandle);
      exit( EXIT_FAILURE );
    }

    // Shared Library
    if ( elf_header->e_type != ET_DYN ) {
      LOG_ERROR( "This file \"%s\" isn't shared lib!", cpf->plugin[p_count].path )
      dlclose( cpf->plugin[p_count].dlhandle );
      exit( EXIT_FAILURE );
    }

    symtable = NULL;
    strtable = NULL;
    for ( i = 0 ; dynamic[i].d_tag != DT_NULL ; i++ ) {
      if ( dynamic[i].d_tag == DT_SYMTAB ) {
        symtable = (ElfW(Sym) *)dynamic[i].d_un.d_val;
        continue;
      }
      if (dynamic[i].d_tag == DT_STRTAB ) {
        strtable = (void *)dynamic[i].d_un.d_val;
	      continue;
	    }
      if ( dynamic[i].d_tag == DT_SYMENT ) {
        symtblentrysize = dynamic[i].d_un.d_val;
        continue;
      }
    }

    // count the number of plugin functions, without constructor, destructor and
    // context (ctx), and bind the constructor, destructor and ctx functions,
    // if exits.
    symtbltotalsize = ( (strtable - (void *)symtable)/symtblentrysize );
    for ( i = 0 ; i < symtbltotalsize ; i++ ) {
      if ( ( ELF64_ST_TYPE( symtable[i].st_info ) == STT_FUNC ) &&
           ( symtable[i].st_value > 0 ) ) {
        fcn_addr = cpf->plugin[p_count].base_addr + symtable[i].st_value;
        if ( ( symtable[i].st_name != 0 ) &&
             ( *(char *)(strtable + symtable[i].st_name) != 0 ) &&
             ( strcmp( (char *)(strtable + symtable[i].st_name),
                       PLUGIN_CONSTRUCTOR_FUNC ) == 0 ) ) {
          cpf->plugin[p_count].ctor = fcn_addr;
          continue;
        }
        if ( ( symtable[i].st_name != 0 ) &&
             ( *(char *)(strtable + symtable[i].st_name) != 0 ) &&
             ( strcmp( (char *)(strtable + symtable[i].st_name),
                       PLUGIN_DESTRUCTOR_FUNC ) == 0 ) ) {
          cpf->plugin[p_count].dtor = fcn_addr;
          continue;
        }
        if ( ( symtable[i].st_name != 0 ) &&
             ( *(char *)(strtable + symtable[i].st_name) != 0 ) &&
             ( strcmp( (char *)(strtable + symtable[i].st_name),
                       PLUGIN_INIT_CTX_FUNC ) == 0 ) ) {
          cpf->plugin[p_count].init_ctx = fcn_addr;
          // call CPF_init_ctx() from plugin, using function ptr "init_plugin_ctx"
          plugin_ctx_t * (*init_plugin_ctx)() = fcn_addr;
          if ( ( cpf->plugin[p_count].ctx = init_plugin_ctx() ) == NULL ) {
            LOG_ERROR( "Cannot initializate plugin \"%s"PLUGIN_EXTENSION"\" context! "
                       "Look at \""PLUGIN_INIT_CTX_FUNC"()\" function.",
                       cpf->plugin[p_count].name )
            exit( EXIT_FAILURE );
          }
          if ( cpf->plugin[p_count].ctx->deps == NULL ) {
            LOG_ERROR( "Cannot initializate plugin \"%s"PLUGIN_EXTENSION"\" dependencies! "
                       "Look at \""PLUGIN_INIT_CTX_FUNC"()\" function and "
                       "set the plugin context dependency!",
                       cpf->plugin[p_count].name )
            exit( EXIT_FAILURE );
          }         
          continue;
        }
        // "valid" function found
        num_funcs++;
      }
    }
    if ( cpf->plugin[p_count].init_ctx == NULL ) {
      LOG_ERROR( "\""PLUGIN_INIT_CTX_FUNC"\"() not found in plugin "
                 "\"%s"PLUGIN_EXTENSION"\".\n"
                 "Cannot initializate plugin system!",
                 cpf->plugin[p_count].name )
      exit( EXIT_FAILURE );
    }
    if ( cpf->plugin[p_count].ctx == NULL ) {
      LOG_ERROR( "Plugin context not defined in plugin "
                 "\"%s"PLUGIN_EXTENSION"\".\n"
                 "Cannot initializate plugin system!",
                 cpf->plugin[p_count].name )
      exit( EXIT_FAILURE );
    }
    // version default value, if not defined
    if ( cpf->plugin[p_count].ctx->version[0] == '\0' ) {
      strcpy(cpf->plugin[p_count].ctx->version, NOT_DEFINED);
    }

    if ( num_funcs == 0 ) {
      LOG_ERROR( "No functions found in plugin \"%s"PLUGIN_EXTENSION"\"!",
                 cpf->plugin[p_count].name )
      exit( EXIT_FAILURE );
    }

    // num_funcs + 1 = will be used to detect the end of struct (NULL value)
    cpf->plugin[p_count].lib_func = (func_t *)calloc( num_funcs + 1, sizeof( func_t ) );
    if ( cpf->plugin[p_count].lib_func == NULL ) {
      LOG_ERROR( "Cannot allocate memory for plugins' functions!!" )
      exit( EXIT_FAILURE );
    }

    j=0;
    for ( i = 0 ; i < symtbltotalsize ; i++ ) {
      fcn_addr = cpf->plugin[p_count].base_addr + symtable[i].st_value;
      if ( ( ELF64_ST_TYPE( symtable[i].st_info ) == STT_FUNC ) &&
           ( symtable[i].st_value > 0 ) &&
           ( cpf->plugin[p_count].ctor != fcn_addr ) &&
           ( cpf->plugin[p_count].dtor != fcn_addr ) &&
           ( cpf->plugin[p_count].init_ctx != fcn_addr ) ) {
        cpf->plugin[p_count].lib_func[j].func_addr = fcn_addr;
        cpf->plugin[p_count].lib_func[j].func_offset =
          (uint64_t)symtable[i].st_value;
        cpf->plugin[p_count].lib_func[j].func_name = NULL;
        if ( ( symtable[i].st_name != 0 ) &&
             ( *(char *)(strtable + symtable[i].st_name) != 0 ) ) {
          cpf->plugin[p_count].lib_func[j].func_name =
            (char *)(strtable + symtable[i].st_name);
        }
        j++;
      }
    }
    calc_blake2( &cpf->plugin[p_count] );
  } // end for
  sort_plugins( cpf );
}


void
check_and_set_dep( cpf_t * cpf )
{
  uint16_t  i, j,
            p_count; // plugin counter
  bool      dep_ok;


  // check all libs dependencies AND set dep functions pointers
  for( p_count = 0 ; p_count < cpf->num_plugins ; p_count++ ) {
    //for( i = 0 ; i < calc_num_dep( cpf->plugin[p_count].ctx->deps ) ; i++ ) {
    for( i = 0 ; (void *)(*(uint64_t *)(cpf->plugin[p_count].ctx->deps+i)) != NULL ; i++ ) {
      dep_ok = false;
      for( j = 0 ; j < cpf->num_plugins ; j++ ) {
        if ( ( cpf->plugin[p_count].ctx->deps[i].dep_lib_name != NULL ) &&
             ( strcmp( cpf->plugin[j].name,
                       cpf->plugin[p_count].ctx->deps[i].dep_lib_name ) == 0 ) ) {
          if ( p_count == j ) {
            LOG_ERROR("Dependency check error in plugin \"%s"PLUGIN_EXTENSION"\": "
                      "same dependency declared!",
                      cpf->plugin[j].name )
            exit( EXIT_FAILURE );
          }
          cpf->plugin[p_count].ctx->deps[i].funcs = cpf->plugin[j].lib_func;
          dep_ok = true;
          break;
        }
      }
      if ( dep_ok == false ) {
        LOG_ERROR(
          "Dependency check error: \"%s\" not found in \"%s"PLUGIN_EXTENSION"\"!",
          cpf->plugin[p_count].ctx->deps[i].dep_lib_name,
          cpf->plugin[p_count].name )
        exit( EXIT_FAILURE );
      }
    }
  }
}


void
load_plugins( cpf_t * cpf )
{
  load_plugins_2_reload( cpf );
  check_and_set_dep( cpf );
}


static uint16_t
dir_content( cpf_t * cpf, char * path, uint16_t * binded_plugins )
{
  DIR * d; // open the path
  struct dirent * dir_entry; // for the directory entries
  char d_path[MAX_PLUGIN_PATH_SIZE];
  uint16_t number_of_plugins = 0;


  if ( ( d = opendir( path ) ) == NULL ) {
    LOG_ERROR( "Cannot open directory \"%s/\"!", path )
    return number_of_plugins;
  }

  while ( ( dir_entry = readdir( d ) ) != NULL )
  {
    if ( dir_entry->d_type == DT_DIR ) { // directory found
      if ( ( strcmp( dir_entry->d_name, "." ) == 0 ) ||
           ( strcmp( dir_entry->d_name, ".." ) == 0 ) )
        continue;
      if ( strlen( path ) + strlen( dir_entry->d_name ) + 2 /* "\0" and "/" */ >
           sizeof( cpf->plugin->path ) ) {
        LOG_ERROR( "Plugin directory path will be truncated!" )
        FREE( cpf )
        exit( EXIT_FAILURE );
      }
      if ( snprintf( d_path,
                     sizeof( cpf->plugin->path ),
                     "%s/%s",
                     path,
                     dir_entry->d_name ) < 0 ) {
        LOG_ERROR( "snprintf() error!" )
        FREE( cpf )
        exit( EXIT_FAILURE );
      }
      // recall dir_content with the new path
      number_of_plugins += dir_content( cpf, d_path, binded_plugins );
    }
    else { // file found
      if ( strstr( dir_entry->d_name, PLUGIN_EXTENSION ) != NULL ) {
        number_of_plugins++;
        if ( cpf != NULL ) {
          if ( strlen( path ) + strlen( dir_entry->d_name ) + 2 /* "\0" and "/" */ >
               sizeof( cpf->plugin->path ) ) {
            LOG_ERROR( "Plugin full path will be truncated!" )
            FREE( cpf )
            exit( EXIT_FAILURE );
          }
          if ( snprintf( d_path,
                         sizeof( cpf->plugin->path ),
                         "%s/%s",
                         path,
                         dir_entry->d_name ) < 0 ) {
            LOG_ERROR( "snprintf() error!" )
            exit( EXIT_FAILURE );
          }
          if ( snprintf( cpf->plugin[*binded_plugins].path,
                         sizeof( cpf->plugin->path ),
                         "%s",
                         d_path ) < 0 ) {
            LOG_ERROR( "snprintf() error!" )
            exit( EXIT_FAILURE );
          }
          // copy plugin name without extension
          if ( snprintf( cpf->plugin[*binded_plugins].name,
                         strlen( d_path ) - strlen( cpf->path ) - sizeof(PLUGIN_EXTENSION)+1,
                         "%s",
                         d_path + strlen( cpf->path ) + 1 ) < 0 ) {
            LOG_ERROR( "snprintf() error!" )
            exit( EXIT_FAILURE );
          }
          *binded_plugins += 1;
        }
      }
    }
  }
  closedir(d);
  return number_of_plugins;
}


void
bind_plugins( cpf_t * cpf )
{
  char path[MAX_PLUGIN_PATH_SIZE];
  uint16_t binded_plugins = 0; // will be used in dir_content internally


  if ( cpf == NULL) {
    LOG_ERROR( "bind_plugins(): CPF not initializated!" )
    exit( EXIT_FAILURE );
  }

  if ( snprintf( path, sizeof( path ), "%s", cpf->path ) < 0 ) {
    LOG_ERROR( "snprintf() error!" )
    exit( EXIT_FAILURE );
  }
  if ( ( cpf->num_plugins = dir_content( NULL, path, NULL ) ) == 0 ) {
    return;
  }

  cpf->plugin = ( plugin_t * )calloc( cpf->num_plugins, sizeof( plugin_t ) );
  if ( cpf->plugin == NULL ) {
    FREE( cpf );
    LOG_ERROR( "bind_plugins(): Cannot allocate memory for plugins!" )
    exit( EXIT_FAILURE );
  }

  if ( snprintf( path, sizeof( path ), "%s", cpf->path ) < 0 ) {
    LOG_ERROR( "snprintf() error!" )
    exit( EXIT_FAILURE );
  }
  cpf->num_plugins = dir_content( cpf, path, &binded_plugins );

  if ( cpf->num_plugins != binded_plugins ) {
    LOG_ERROR( "Something goes wrong! Check your plugins' directory \"%s\"!\n" \
               "        * Number of \""PLUGIN_EXTENSION"\" files found inside plugins' directory: %d\n" \
               "        * Number of binded plugins: %d",
               cpf->path,
               cpf->num_plugins,
               binded_plugins )
    exit( EXIT_FAILURE );
  }
}