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
#define _GNU_SOURCE
#include <link.h>
#include <dirent.h>
#include <dlfcn.h>
#include <elf.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "plugin_manager.h"
#include "log.h"
#include "sha1.h"


static int
comparator( const void * p, const void * q )
{
  return strncmp( ( (plugin_t *)p)->path,
                  ( (plugin_t *)q)->path,
                  sizeof( ( (plugin_t *)p)->path ) );
}


void sort_plugins( cpf_t * cpf ) {
  if ( ( cpf != NULL ) && ( cpf->number_of_plugins > 1 ) ) {
    qsort( cpf->plugin, cpf->number_of_plugins, sizeof( plugin_t ), comparator );
  }
}


void
load_plugins( cpf_t * cpf )
{
  ElfW(Ehdr)      *elf_header;
  //ElfW(Phdr)      *prog_header;
  ElfW(Dyn)       *dynamic;
  ElfW(Sym)       *symtable;
  void            *strtable;
  unsigned short  i, j,
                  plugin_counter,
                  symtbltotalsize,
                  symtblentrysize = 0;
  struct link_map *lnkmap;

  for( plugin_counter=0 ; plugin_counter < cpf->number_of_plugins ; plugin_counter++ ) {
    cpf->plugin[plugin_counter].total_funcs = 0;
    cpf->plugin[plugin_counter].dlhandle = dlopen( cpf->plugin[plugin_counter].path, RTLD_LAZY | RTLD_GLOBAL );
    if ( cpf->plugin[plugin_counter].dlhandle == NULL ) {
      LOG_ERROR( "dlopen(): %s", dlerror() )
      exit( EXIT_FAILURE );
    }

    if ( dlinfo(cpf->plugin[plugin_counter].dlhandle, RTLD_DI_LINKMAP, &lnkmap) == -1 ) {
      LOG_ERROR( "RTLD_DI_LINKMAP failed: %s", dlerror() )
      exit( EXIT_FAILURE );
    }

    cpf->plugin[plugin_counter].base_addr = (void *)lnkmap->l_addr;
    dynamic = lnkmap->l_ld;

    elf_header = (ElfW(Ehdr)*) cpf->plugin[plugin_counter].base_addr;
	
    /* ELF magic number */
    if (memcmp(elf_header->e_ident, ELFMAG, SELFMAG) != 0) {
      LOG_ERROR( "ELF magic number not found!" )
      dlclose(cpf->plugin[plugin_counter].dlhandle);
      exit( EXIT_FAILURE );
    }
	
    // AMD x86-64 architecture only
    if ( elf_header->e_machine != EM_X86_64 ) {
      LOG_ERROR( "Architecture not compatible!" )
      dlclose(cpf->plugin[plugin_counter].dlhandle);
      exit( EXIT_FAILURE );
    }

    // Shared Library
    if ( elf_header->e_type != ET_DYN ) {
      LOG_ERROR( "This file \"%s\" isn't shared lib!", cpf->plugin[plugin_counter].path )
      dlclose( cpf->plugin[plugin_counter].dlhandle );
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

    symtbltotalsize = ((strtable - (void *)symtable)/symtblentrysize);
    for ( i = 0 ; i < symtbltotalsize ; i++ )
      if ( ( ELF64_ST_TYPE( symtable[i].st_info ) == STT_FUNC ) &&
           ( symtable[i].st_value > 0 ) )
        cpf->plugin[plugin_counter].total_funcs++;

    cpf->plugin[plugin_counter].funcs = (func_t *)calloc( cpf->plugin[plugin_counter].total_funcs, sizeof( func_t ) );
    if ( cpf->plugin[plugin_counter].funcs == NULL ) {
      LOG_ERROR( "Cannot allocate memory for plugins' functions!!" )
      exit( EXIT_FAILURE );
    }

    j=0;
    for ( i = 0 ; i < symtbltotalsize ; i++ ) {
      if ( ( ELF64_ST_TYPE( symtable[i].st_info ) == STT_FUNC ) &&
           ( symtable[i].st_value > 0 ) ) {
        cpf->plugin[plugin_counter].funcs[j].func_name = NULL;
        if ( ( symtable[i].st_name != 0 ) &&
             ( *(char *)(strtable + symtable[i].st_name) != 0 ) )
          cpf->plugin[plugin_counter].funcs[j].func_name = (char *)(strtable + symtable[i].st_name);
        cpf->plugin[plugin_counter].funcs[j].func_offset = (uint64_t)symtable[i].st_value;
        cpf->plugin[plugin_counter].funcs[j].func_addr = cpf->plugin[plugin_counter].base_addr + symtable[i].st_value;
        j++;
      }
    }
    calc_sha1( &cpf->plugin[plugin_counter] );
  } // end for
  sort_plugins( cpf );
}


static int
dir_content( cpf_t * cpf, char * path, unsigned short * binded_plugins )
{
  DIR * d; // open the path
  struct dirent * dir_entry; // for the directory entries
  char d_path[MAX_PLUGIN_PATH_SIZE];
  unsigned short number_of_plugins = 0;

  if ( ( d = opendir( path ) ) == NULL ) {
    LOG_ERROR( "Cannot open directory \"%s\"!", path )
    FREE( cpf )
    exit( EXIT_FAILURE );
  }

  while ( ( dir_entry = readdir( d ) ) != NULL )
  {
    if ( dir_entry->d_type == DT_DIR ) { // directory found
      if ( ( strcmp( dir_entry->d_name, "." ) == 0 ) ||
           ( strcmp( dir_entry->d_name, ".." ) == 0 ) )
        continue;
      if ( strlen( path ) + strlen( dir_entry->d_name ) + 2 > sizeof( cpf->plugin->path ) ) {
        LOG_ERROR( "Plugin directory path will be truncated!" )
        FREE( cpf )
        exit( EXIT_FAILURE );
      }
      snprintf( d_path, sizeof( d_path ), "%s/%s", path, dir_entry->d_name );
      number_of_plugins += dir_content( cpf, d_path, binded_plugins ); // recall with the new path
    }
    else { // file found
      if ( strstr( dir_entry->d_name, PLUGIN_EXTENSION ) != NULL ) {
        number_of_plugins++;
        if ( cpf != NULL ) {
          if ( strlen( path ) + strlen( dir_entry->d_name ) + 2 > sizeof( cpf->plugin->path ) ) {
            LOG_ERROR( "Plugin full path will be truncated!" )
            FREE( cpf )
            exit( EXIT_FAILURE );
          }
          snprintf( d_path, sizeof( cpf->plugin->path ), "%s/%s", path, dir_entry->d_name );
          snprintf( cpf->plugin[*binded_plugins].path,
                    sizeof( cpf->plugin->path ),
                    "%s",
                    d_path );
          // copy plugin name without extension
          snprintf( cpf->plugin[*binded_plugins].name,
                    strlen( d_path ) - strlen( cpf->path ) - sizeof(PLUGIN_EXTENSION)+1,
                    "%s",
                    d_path + strlen( cpf->path ) + 1 );
          *binded_plugins += 1;
        }
      }
    }
  }
  closedir(d);
  return number_of_plugins;
}


unsigned short
bind_plugins( cpf_t * cpf )
{
  char path[MAX_PLUGIN_PATH_SIZE];
  unsigned short binded_plugins = 0; // will be used in dir_content internally


  if ( cpf == NULL) {
    LOG_ERROR( "bind_plugins(): CPF not initializated!" )
    exit( EXIT_FAILURE );
  }

  snprintf( path, sizeof( path ), "%s", cpf->path );
  if ( ( cpf->number_of_plugins = dir_content( NULL, path, NULL ) ) == 0 ) {
    return 0;
  }

  cpf->plugin = ( plugin_t * )calloc( cpf->number_of_plugins, sizeof( plugin_t ) );
  if ( cpf->plugin == NULL ) {
    FREE( cpf );
    LOG_ERROR( "bind_plugins(): Cannot allocate memory for plugins!" )
    exit( EXIT_FAILURE );
  }

  snprintf( path, sizeof( path ), "%s", cpf->path );
  cpf->number_of_plugins = dir_content( cpf, path, &binded_plugins );

  if ( cpf->number_of_plugins != binded_plugins ) {
    LOG_ERROR( "Something goes wrong! Check your plugins' directory \"%s\"!\n" \
               "        * Number of \""PLUGIN_EXTENSION"\" files found inside plugins' directory: %d\n" \
               "        * Number of binded plugins: %d",
               cpf->path,
               cpf->number_of_plugins,
               binded_plugins )
    exit( EXIT_FAILURE );
  }

  return binded_plugins;
}