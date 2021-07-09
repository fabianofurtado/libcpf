/*
  libcpf - C Plugin Framework

  cpf.c - main plugins functions

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
#include <dlfcn.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpf.h"
#include "plugin_manager.h"
#include "sha1.h"


static void
CPF_free_all_plugins( cpf_t * cpf, bool call_destructor )
{
  unsigned short i;
  ctor_dtor_t ctor_dtor;


  if ( cpf == NULL )
    return;
  if ( cpf->number_of_plugins == 0 )
    return;
  // close the shared object handlers and free heap memory
  for ( i = 0 ; i < cpf->number_of_plugins ; i++ ) {
    // Call the possible destructor
    if  ( ( call_destructor == true ) &&
          ( cpf->plugin[i].destructor_addr != NULL ) ) {
      ctor_dtor = cpf->plugin[i].destructor_addr;
      ctor_dtor( &(cpf->plugin[i]) );
    }
    DLCLOSE( cpf->plugin[i].dlhandle )
    FREE( cpf->plugin[i].funcs )
  }
  FREE( cpf->plugin )
  cpf->number_of_plugins = 0;
}


void
CPF_cleanup( cpf_t ** cpf, bool call_destructor )
{
  if ( (*cpf) == NULL )
    return;
  CPF_free_all_plugins( (*cpf), call_destructor );
  FREE( (*cpf) )
}


cpf_t *
CPF_init( char * directory_name, bool call_constructor_destructor )
{
  cpf_t * cpf = NULL;


  cpf = (cpf_t *)calloc( 1, sizeof( cpf_t ) );
  if ( cpf == NULL ) {
    LOG_ERROR( "CPF_init(): Cannot allocate memory for plugin framework!" )
    exit( EXIT_FAILURE );
  }

  if ( directory_name == NULL ) { // local directory with default PLUGIN_DIRNAME path
    if ( getcwd( cpf->path, sizeof( cpf->path ) ) == NULL) {
      LOG_ERROR( "CPF_init(): getcwd() error!" )
      exit( EXIT_FAILURE );
    }

    if ( strlen( cpf->path ) + 2 > sizeof( cpf->path ) ) {
      LOG_ERROR( "CPF_init(): Plugin path will be truncated!" )
      exit(EXIT_FAILURE);
    }
    strcat( cpf->path, "/" );

    if ( strlen( cpf->path ) + sizeof( PLUGIN_DIRNAME ) > sizeof( cpf->path ) ) {
      LOG_ERROR( "CPF_init(): Plugin path size cannot be greather than %ld bytes!",
                 sizeof( cpf->path ) )
      exit( EXIT_FAILURE );
    }
    strcat( cpf->path, PLUGIN_DIRNAME );
  }
  else {
    if ( directory_name[0] == '/' ) { // Full path specified
      if ( strlen( directory_name ) + 1 > sizeof( cpf->path ) ) {
        LOG_ERROR( "CPF_init(): Plugin path size cannot be greather than %ld bytes!",
                    sizeof( cpf->path ) )
        exit( EXIT_FAILURE );
      }
      snprintf( cpf->path,
                sizeof( cpf->path ),
                "%s",
                directory_name );
    }
    else { // local directory with custom path
      if ( getcwd( cpf->path, sizeof( cpf->path ) ) == NULL) {
        LOG_ERROR( "CPF_init(): getcwd() error!" )
        exit( EXIT_FAILURE );
      }

      if ( strlen( cpf->path ) + 2 > sizeof( cpf->path ) ) {
        LOG_ERROR( "CPF_init(): Plugin path will be truncated!" )
        exit(EXIT_FAILURE);
      }
      strcat( cpf->path, "/" );

      if ( strlen( cpf->path ) + strlen( directory_name ) + 1 > sizeof( cpf->path ) ) {
        LOG_ERROR( "CPF_init(): Plugin path size cannot be greather than %ld bytes!",
                  sizeof( cpf->path ) )
        exit( EXIT_FAILURE );
      }
      strcat( cpf->path, directory_name );
    }
  }

  if ( bind_plugins( cpf ) == 0 ) {
    CPF_free_all_plugins( cpf, call_constructor_destructor );
  }
  else {
    load_plugins( cpf, call_constructor_destructor );
  }
  return cpf;
}


void
CPF_print_loaded_libs( cpf_t * cpf )
{
  unsigned short i, j;
  char * str_msg = "    Func name: %s\t | Offset: 0x%lx\t | Address: %p\n"; 


  if ( cpf->number_of_plugins == 0 ) {
    LOG_INFO( "No plugins loaded in memory!" )
    return;
  }

  printf( "Total of loaded plugins: %d | Base path: %s\n",
          cpf->number_of_plugins,
          cpf->path );

  for ( i=0 ; i < cpf->number_of_plugins ; i++ ) {
    printf( "  %s version %s has %d functions:\n    ( base address: %p | sha1: ",
            cpf->plugin[i].path,
            cpf->plugin[i].version,
            cpf->plugin[i].total_funcs,
            cpf->plugin[i].base_addr);
    print_sha1( &cpf->plugin[i] );
    printf( " )\n" );
    for ( j=0 ; j < cpf->plugin[i].total_funcs; j++ )
      if ( cpf->plugin[i].funcs[j].func_name == NULL )
        printf( str_msg,
                NOT_DEFINED,
                cpf->plugin[i].funcs[j].func_offset,
                cpf->plugin[i].funcs[j].func_addr );
      else
        printf( str_msg,
                cpf->plugin[i].funcs[j].func_name,
                cpf->plugin[i].funcs[j].func_offset,
                cpf->plugin[i].funcs[j].func_addr );
  }
  printf( "\n\n" );
}


static void *
CPF_get_plugin_base_addr( cpf_t * cpf, char * plugin_name )
{
  unsigned short i;


  if ( plugin_name == NULL ) {
    LOG_ERROR( "CPF_get_plugin_base_addr(): Parameter cannot be NULL!" )
    return NULL;
  }

  for ( i=0 ; i < cpf->number_of_plugins ; i++ ) {
    if ( strcmp( plugin_name, cpf->plugin[i].name ) == 0 ) {
      return cpf->plugin[i].base_addr;
    }
  }
  LOG_ERROR( "CPF_get_plugin_base_addr(): Cannot get plugin base address!" )
  return NULL;
}


void *
CPF_get_func_addr( cpf_t * cpf, char * plugin_name, char * func_name )
{
  unsigned short i, j;


  if ( ( plugin_name == NULL  ) || ( func_name == NULL )) {
    LOG_ERROR( "CPF_get_func_addr(): Parameters cannot be NULL!" )
    return NULL;
  }

  for ( i=0 ; i < cpf->number_of_plugins ; i++ ) {
    if ( strcmp( plugin_name, cpf->plugin[i].name ) == 0 ) {
      for ( j=0 ; j < cpf->plugin[i].total_funcs; j++ ) {
        if ( ( cpf->plugin[i].funcs[j].func_name != NULL ) && 
             ( strcmp( func_name, cpf->plugin[i].funcs[j].func_name ) == 0 ) ) {
          return cpf->plugin[i].funcs[j].func_addr;
        }
      }
    }
  }
  LOG_ERROR( "CPF_get_func_addr(): Cannot get function address!" )
  return NULL;
}


uint64_t
CPF_get_func_offset( cpf_t * cpf, char * plugin_name, char * func_name )
{
  unsigned short i, j;


  if ( ( plugin_name == NULL  ) || ( func_name == NULL )) {
    LOG_ERROR( "CPF_get_func_offset(): Parameters cannot be NULL!" )
    return 0;
  }

  for ( i=0 ; i < cpf->number_of_plugins ; i++ ) {
    if ( strcmp( plugin_name, cpf->plugin[i].name ) == 0 ) {
      for ( j=0 ; j < cpf->plugin[i].total_funcs; j++ ) {
        if ( ( cpf->plugin[i].funcs[j].func_name != NULL ) && 
             ( strcmp( func_name, cpf->plugin[i].funcs[j].func_name ) == 0 ) ) {
          return cpf->plugin[i].funcs[j].func_offset;
        }
      }
    }
  }
  LOG_ERROR( "CPF_get_func_offset(): Cannot get function offset!" )
  return 0;
}


void *
CPF_call_func_by_addr( void * func_addr, enum func_prototype_t fproto, ... )
{
  va_list varglist;
  void * ret = NULL;


  if ( func_addr == NULL) {
    LOG_ERROR( "CPF_call_func_by_addr(): function address cannot be NULL!" )
    return NULL;
  }

  va_start( varglist, fproto );
  ret = CPF_wrapper_call_func_by_addr( func_addr, fproto, varglist );
  va_end( varglist );

  return ret;
}


void *
CPF_call_func_by_offset( cpf_t * cpf,
                         char * plugin_name,
                         uint64_t func_offset,
                         enum func_prototype_t fproto,
                         ... )
{
  va_list varglist;
  void * base_addr;
  void * ret = NULL;


  if ( func_offset == 0 )
    return NULL;

  if ( ( base_addr = CPF_get_plugin_base_addr( cpf, plugin_name ) ) == NULL )
    return NULL;

  va_start( varglist, fproto );
  ret = CPF_wrapper_call_func_by_addr( base_addr + func_offset, fproto, varglist );
  va_end( varglist );

  return ret;
}


void *
CPF_call_func_by_name( cpf_t * cpf,
                       char * plugin_name,
                       char * func_name,
                       enum func_prototype_t fproto,
                       ... )
{
  va_list varglist;
  void * func_addr;
  void * ret = NULL;


  if ( ( func_addr = CPF_get_func_addr( cpf, plugin_name, func_name ) ) == NULL )
    return NULL;

  va_start( varglist, fproto );
  ret = CPF_wrapper_call_func_by_addr( func_addr, fproto, varglist );
  va_end( varglist );

  return ret;
}


/*
 * Reload process: all libs in the cpf->path directory will be analysed.
 * New libs will be included in the list.
 * Unmodified libs will remain unchanged in memory.
 * Old libs will be removed.
 * Modified libs will be updated in memory.
*/
int
CPF_reload_libs( cpf_t ** cpf, bool display_report )
{
  cpf_t * cpf_reloaded;
  cpf_t * cpf_tmp;
  unsigned short  num_plugins, c, l, r;
  unsigned char * status_l; // loaded: currently in use
  unsigned char * status_r; // reloaded: will be loaded
  ctor_dtor_t ctor_dtor;


  if ( ( cpf_reloaded = CPF_init( (*cpf)->path, false ) ) == NULL ) {
    LOG_ERROR( "CPF_reload_libs(): Cannot initialize plugin framework to reload shared libs!" )
    return EXIT_FAILURE;
  }

  if ( cpf_reloaded->number_of_plugins == 0 ) {
    // unload all current libs from memory
    CPF_cleanup( &cpf_reloaded, false );
    CPF_cleanup( cpf, false );
    return EXIT_SUCCESS;
  }

  status_l = (unsigned char *)malloc( (*cpf)->number_of_plugins * sizeof( unsigned char ) );
  if ( status_l == NULL ) {
    LOG_ERROR( "CPF_reload_libs(): Cannot allocate memory for reload plugins!" )
    exit( EXIT_FAILURE );
  }

  status_r = (unsigned char *)malloc( cpf_reloaded->number_of_plugins * sizeof( unsigned char ) );
  if ( status_r == NULL ) {
    LOG_ERROR( "CPF_reload_libs(): Cannot allocate memory for reload plugins!" )
    exit( EXIT_FAILURE );
  }
  // Set the new lib status:
  //   * (R)eload: The loaded lib was modified ( same name and different sha1 )
  //   * (D)elete: The loaded lib doesn't exist anymore ( lib deleted from directory )
  //   * (U)nmodified: The loaded lib wasn't modified ( same name and sha1 )
  //   * (N)ew: The lib's name isn't in the current list ( new lib/name to load in the list )

  // Set to Delete 'D' the default status of current plugin
  // ===> All possible flags/status: 'D', 'R' or 'U'
  memset( status_l, 'D', (*cpf)->number_of_plugins * sizeof( unsigned char ) );
  // Set to New 'N' the default status of reloaded plugin
  // ===> All possible flags/status: 'N', 'R' or 'U'
  memset( status_r, 'N', cpf_reloaded->number_of_plugins * sizeof( unsigned char ) );

  // Seek for (U)nmodified libs and set the 'U' flag status:
  for ( r = 0 ; r < cpf_reloaded->number_of_plugins ; r++ ) {
    for ( l = 0 ; l < (*cpf)->number_of_plugins; l++ ) {
      if ( ( status_l[l] == 'D' ) &&
           ( strncmp( (*cpf)->plugin[l].name, cpf_reloaded->plugin[r].name, sizeof( (*cpf)->plugin->name ) ) == 0 ) &&
           ( memcmp( (*cpf)->plugin[l].sha1, cpf_reloaded->plugin[r].sha1, sizeof( (*cpf)->plugin->sha1 ) ) == 0 ) ) {
        status_l[l] = 'U';
        status_r[r] = 'U';
      }
    }
  }

  // Modified libs will be updated in memory:
  // set to (R)eload 'R' flash/status if the lib name is the same and sha1 is different
  // status_l: possible flags status here: 'D' or 'U'
  // status_r: possible flags status here: 'N' or 'U'
  for ( r = 0 ; r < cpf_reloaded->number_of_plugins ; r++ ) {
    if ( status_r[r] != 'N' ) // here, status_r[r] == 'U'
      continue;
    for ( l = 0 ; l < (*cpf)->number_of_plugins; l++ ) {
      if ( ( status_l[l] == 'D' ) &&
           ( strncmp( (*cpf)->plugin[l].name, cpf_reloaded->plugin[r].name, sizeof( (*cpf)->plugin->name ) ) == 0 ) &&
           ( memcmp( (*cpf)->plugin[l].sha1, cpf_reloaded->plugin[r].sha1, sizeof( (*cpf)->plugin->sha1 ) ) != 0 ) ) {
        status_l[l] = 'R';
        status_r[r] = 'R';
        // Do the reload process:

        // Call the possible destructor, before reload
        if ( (*cpf)->plugin[l].destructor_addr != NULL ) {
          ctor_dtor = (*cpf)->plugin[l].destructor_addr;
          ctor_dtor( &((*cpf)->plugin[l]) );
        }
        // Free current lib dynamic allocation
        DLCLOSE( (*cpf)->plugin[l].dlhandle )
        FREE( (*cpf)->plugin[l].funcs )
        // Copy all plugin struct
        memcpy( &(*cpf)->plugin[l], &cpf_reloaded->plugin[r], sizeof( plugin_t ) );
        // Set to NULL the "old" reloaded allocated structs
        cpf_reloaded->plugin[r].dlhandle = NULL;
        cpf_reloaded->plugin[r].funcs = NULL;
        // Call the possible constructor, after reload
        if ( (*cpf)->plugin[l].constructor_addr != NULL ) {
          ctor_dtor = (*cpf)->plugin[l].constructor_addr;
          ctor_dtor( &((*cpf)->plugin[l]) );
        }
        break;
      }
    }
  }

  // Calculate the new plugins' total number and alloc dynamic memory:
  // From current lib: 'R' and 'U' flags status
  // From reloaded lib: 'N' flag status
  if ( display_report == true) {
    LOG_INFO( "Reloaded libs in \"%s\":",
              (*cpf)->path )
  }
  num_plugins = 0;
  for ( l = 0 ; l < (*cpf)->number_of_plugins ; l++ ) {
    if ( status_l[l] != 'D' ) { // only 'R' and 'U' flags
      num_plugins++;
    }
    if ( ( status_l[l] == 'D' ) &&
         ( (*cpf)->plugin[l].destructor_addr != NULL )  ) {
      // Call the destructor for marked deleted lib
      ctor_dtor = (*cpf)->plugin[l].destructor_addr;
      ctor_dtor( &( (*cpf)->plugin[l] ) );
    }
    if ( display_report == true ) {
      switch( status_l[l] )
      {
        case 'R':
          LOG_INFO("* %s (Reloaded)", (*cpf)->plugin[l].name )
          break;
        case 'D':
          LOG_INFO("* %s (Deleted)", (*cpf)->plugin[l].name )
          break;
        case 'U':
          LOG_INFO("* %s (Unmodified)", (*cpf)->plugin[l].name )
          break;
        default: // do nothing
          break;
      }
    }
  }

  for ( r = 0 ; r < cpf_reloaded->number_of_plugins ; r++ ) {
    if ( status_r[r] == 'N' ) {
      num_plugins++;
      // call the constructor for the (N) plugin
      if ( cpf_reloaded->plugin[r].constructor_addr != NULL ) {
        ctor_dtor = cpf_reloaded->plugin[r].constructor_addr;
        ctor_dtor( &(cpf_reloaded->plugin[r]) );
      }
      if ( display_report == true ) {
        LOG_INFO("* %s (New)", cpf_reloaded->plugin[r].name )
      }
    }
  }

  // Alloc "num_plugins" dynamic memory for new plugins
  cpf_tmp = ( cpf_t * )calloc( num_plugins, sizeof( cpf_t ) );
  if ( cpf_tmp == NULL ) {
    LOG_ERROR( "CPF_reload_libs(): Cannot allocate memory for cpf_tmp!" )
    exit( EXIT_FAILURE );
  }

  // Set cpf_t struct
  cpf_tmp->number_of_plugins = num_plugins;
  snprintf( cpf_tmp->path,
            sizeof( cpf_tmp->path ),
            "%s",
            (*cpf)->path );

  // cpf_tmp will receive only (R), (U) and (N) plugins
  cpf_tmp->plugin = ( plugin_t * )calloc( num_plugins, sizeof( plugin_t ) );
  if ( cpf_tmp->plugin == NULL ) {
    LOG_ERROR( "Cannot allocate memory for tmp plugins!" )
    exit( EXIT_FAILURE );
  }

  // memcpy the (R), (U) and (N) plugins
  for( c = 0 ; c < num_plugins ; ) {
    for ( l = 0 ; l < (*cpf)->number_of_plugins ; l++ ) {
      if ( status_l[l] != 'D' ) { // only 'R' and 'U' flags
        memcpy( &cpf_tmp->plugin[c], &(*cpf)->plugin[l], sizeof( plugin_t ) );
        (*cpf)->plugin[l].dlhandle = NULL;
        (*cpf)->plugin[l].funcs = NULL;
        c++;
      }
    }
    for ( r = 0 ; r < cpf_reloaded->number_of_plugins ; r++ ) {
      if ( status_r[r] == 'N' ) {
        memcpy( &cpf_tmp->plugin[c], &cpf_reloaded->plugin[r], sizeof( plugin_t ) );
        cpf_reloaded->plugin[r].dlhandle = NULL;
        cpf_reloaded->plugin[r].funcs = NULL;
        c++;
      }
    }
  }

  FREE( status_r )
  FREE( status_l )
  CPF_cleanup( &cpf_reloaded, false );
  CPF_cleanup( cpf, false );
  sort_plugins( cpf_tmp );
  *cpf = cpf_tmp;
  return EXIT_SUCCESS;
}


void
CPF_unload_libs( cpf_t * cpf )
{
  if ( cpf->number_of_plugins == 0 ) {
    LOG_INFO( "CPF_unload_libs(): There is no plugins loaded in memory!" )
    return;
  }
  CPF_free_all_plugins( cpf, true );
}