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
#include "blake2.h"


static void
CPF_free_close_plugin( plugin_t * p )
{
  if ( p == NULL ) {
    return;
  }
  DLCLOSE( p->dlhandle )
  FREE( p->lib_func )
}


static void
CPF_free_plugins( cpf_t * cpf )
{
  uint16_t i;


  if ( cpf == NULL ) {
    return;
  }
  // close the shared object handlers and free heap memory
  for ( i = 0 ; i < cpf->num_plugins ; i++ ) {
    CPF_free_close_plugin( &(cpf->plugin[i]) );
  }
  FREE( cpf->plugin )
  cpf->num_plugins = 0;
}


void
CPF_free( cpf_t ** cpf )
{
  if ( (*cpf) == NULL ) {
    return;
  }
  CPF_free_plugins( (*cpf) );
  FREE( (*cpf) )
}


static void
CPF_call_plugin_dtor( plugin_t * p )
{
  ctor_dtor_t ctor_dtor;

  if ( p == NULL) {
    return;
  }
  if ( p->dtor != NULL ) {
    ctor_dtor = p->dtor;
    ctor_dtor( p );
  }
}


void
CPF_call_dtor( cpf_t * cpf )
{
  uint16_t p_count;


  if ( cpf == NULL ) {
    return;
  }
  // close the shared object handlers and free heap memory
  for ( p_count = 0 ; p_count < cpf->num_plugins ; p_count++ ) {
    CPF_call_plugin_dtor( &(cpf->plugin[p_count]) );
  }
}


static void
CPF_call_plugin_ctor( plugin_t * p )
{
  ctor_dtor_t ctor_dtor;

  if ( p == NULL) {
    return;
  }
  if ( p->ctor != NULL ) {
    ctor_dtor = p->ctor;
    ctor_dtor( p );
  }
}


static void
call_ctor( cpf_t * cpf )
{
  uint16_t p_count; // plugin counter


  if ( cpf == NULL ) {
    return;
  }
  for( p_count=0 ; p_count < cpf->num_plugins ; p_count++ ) {
    CPF_call_plugin_ctor( &(cpf->plugin[p_count]) );
  }
}

static cpf_t *
init_general( char * directory_name )
{
  cpf_t * cpf;


  cpf = (cpf_t *)calloc( 1, sizeof( cpf_t ) );
  if ( cpf == NULL ) {
    LOG_ERROR( "Cannot allocate memory for plugin framework!" )
    exit( EXIT_FAILURE );
  }

  if ( directory_name == NULL ) { // local directory with default PLUGIN_DIRNAME path
    if ( getcwd( cpf->path, sizeof( cpf->path ) ) == NULL) {
      LOG_ERROR( "getcwd() error!" )
      exit( EXIT_FAILURE );
    }

    if ( strlen( cpf->path ) + 2 > sizeof( cpf->path ) ) {
      LOG_ERROR( "Plugin path will be truncated!" )
      exit(EXIT_FAILURE);
    }
    strcat( cpf->path, "/" );

    if ( strlen( cpf->path ) + sizeof( PLUGIN_DIRNAME ) + 1 > sizeof( cpf->path ) ) {
      LOG_ERROR( "Plugin path size cannot be greather than %ld bytes!",
                 sizeof( cpf->path ) )
      exit( EXIT_FAILURE );
    }
    strcat( cpf->path, PLUGIN_DIRNAME );
  }
  else {
    if ( directory_name[0] == '/' ) { // Full path specified
      if ( strlen( directory_name ) + 1 > sizeof( cpf->path ) ) {
        LOG_ERROR( "Plugin path size cannot be greather than %ld bytes!",
                    sizeof( cpf->path ) )
        exit( EXIT_FAILURE );
      }
      if ( snprintf( cpf->path,
                     sizeof( cpf->path ),
                     "%s",
                     directory_name ) < 0 ) {
        LOG_ERROR( "snprintf() error!" )
        exit( EXIT_FAILURE );
      }
    }
    else { // local directory with custom path
      if ( getcwd( cpf->path, sizeof( cpf->path ) ) == NULL) {
        LOG_ERROR( "getcwd() error!" )
        exit( EXIT_FAILURE );
      }

      if ( strlen( cpf->path ) + 2 > sizeof( cpf->path ) ) {
        LOG_ERROR( "Plugin path will be truncated!" )
        exit(EXIT_FAILURE);
      }
      strcat( cpf->path, "/" );

      if ( strlen( cpf->path ) + strlen( directory_name ) + 1 > sizeof( cpf->path ) ) {
        LOG_ERROR( "Plugin path size cannot be greather than %ld bytes!",
                  sizeof( cpf->path ) )
        exit( EXIT_FAILURE );
      }
      strcat( cpf->path, directory_name );
    }
  }

  bind_plugins( cpf );

  return cpf;
}


static cpf_t *
init_to_reload( char * directory_name )
{
  cpf_t * cpf;
  cpf = init_general( directory_name );
  load_plugins_2_reload( cpf );

  return cpf;
}


cpf_t *
CPF_init( char * directory_name )
{
  cpf_t * cpf;
  cpf = init_general( directory_name );
  load_plugins( cpf );
  call_ctor( cpf );

  return cpf;
}


static uint16_t
calc_num_dep( deps_t * d )
{
  uint16_t c;


  // these 2 "fors" below generates the same ASM code!!!
  //for ( c = 0 ; d[c].dep_lib_name != NULL ; c++ ); // OK
  // OK without "dep_lib_name" reference (only ptr arithmetic)
  for( c = 0 ; (void *)(*(uint64_t *)(d+c)) != NULL ; c++ );
  //  printf( "\n%p  %s\n",
  //               (void *)(*(uint64_t *)(d+c)),
  //               (char *)(*(uint64_t *)(d+c)) );

  return c;
}


static uint16_t
calc_plugin_num_funcs( func_t * f )
{
  uint16_t c;


  for ( c = 0 ; (void *)(*(uint64_t *)(f+c)) != NULL ; c++ );

  return c;
}


void
CPF_print_loaded_libs( cpf_t * cpf )
{
  uint16_t c, d, f, i, j;
  char * str_msg = "      * \"%s\" (offset 0x%lx / address %p)\n"; 


  if ( cpf->num_plugins == 0 ) {
    LOG_INFO( "No plugins loaded in memory!" )
    return;
  }

  printf( "Total of %d loaded plugins in \"%s/\":\n",
          cpf->num_plugins,
          cpf->path );

  for ( i=0 ; i < cpf->num_plugins ; i++ ) {
    d = calc_num_dep( cpf->plugin[i].ctx->deps );
    printf( "  %.2d) \"%s\" (%s) version %s has:\n",
            i+1,
            cpf->plugin[i].name,
            cpf->plugin[i].path,
            cpf->plugin[i].ctx->version);
    printf( "    * base address: %p\n",
          cpf->plugin[i].base_addr);
    printf( "    * hash id blake2: ");
    print_blake2( &cpf->plugin[i] );
    printf( "\n    * %d dependenc", d );
    if ( d == 1 ) {
      printf( "y" );
    } else {
      printf( "ies" );
    }
    if ( d > 0 ) {
      printf( ": ");
      for ( c = 0; c < d ; c++ ) { 
        printf( "\"%s\"  ", cpf->plugin[i].ctx->deps[c].dep_lib_name );
      }
    }
    f = calc_plugin_num_funcs( cpf->plugin[i].lib_func );
    printf( "\n    * %d functions:\n", f );
    for ( j=0 ; j < f; j++ )
      if ( cpf->plugin[i].lib_func[j].func_name == NULL )
        printf( str_msg,
                NOT_DEFINED,
                cpf->plugin[i].lib_func[j].func_offset,
                cpf->plugin[i].lib_func[j].func_addr );
      else
        printf( str_msg,
                cpf->plugin[i].lib_func[j].func_name,
                cpf->plugin[i].lib_func[j].func_offset,
                cpf->plugin[i].lib_func[j].func_addr );
  }
  printf( "\n\n" );
}


static void *
CPF_get_plugin_base_addr( cpf_t * cpf, char * plugin_name )
{
  uint16_t i;


  if ( plugin_name == NULL ) {
    LOG_ERROR( "CPF_get_plugin_base_addr(): Parameter cannot be NULL!" )
    return NULL;
  }

  for ( i=0 ; i < cpf->num_plugins ; i++ ) {
    if ( strcmp( plugin_name, cpf->plugin[i].name ) == 0 ) {
      return cpf->plugin[i].base_addr;
    }
  }
  LOG_ERROR( "CPF_get_plugin_base_addr(): Cannot get plugin base address!" )
  return NULL;
}


static void *
get_func_addr_by_func( func_t * func, char * func_name )
{
  uint16_t f, i;

  if ( ( func == NULL ) || ( func_name == NULL ) ) {
    LOG_ERROR( "get_func_addr_by_func(): function name cannot be NULL!" )
    return NULL;
  }

  f = calc_plugin_num_funcs( func );
  for ( i=0 ; i < f; i++ ) {
    if ( ( func[i].func_name != NULL ) &&
          ( strcmp( func_name, func[i].func_name ) == 0 ) ) {
      return func[i].func_addr;
    }
  }
  return NULL;
}


void *
CPF_get_func_addr( cpf_t * cpf, char * plugin_name, char * func_name )
{
  uint16_t i;
  void *   func_addr;

  if ( cpf->num_plugins == 0 ) {
    LOG_ERROR( "CPF_get_func_addr(): There is no plugin loaded!" )
    return NULL;
  }

  if ( plugin_name == NULL  ) {
    LOG_ERROR( "CPF_get_func_addr(): Parameter cannot be NULL!" )
    return NULL;
  }

  for ( i=0 ; i < cpf->num_plugins ; i++ ) {
    if ( strcmp( plugin_name, cpf->plugin[i].name ) == 0 ) {
      func_addr = get_func_addr_by_func( cpf->plugin[i].lib_func, 
                                         func_name );
      if ( func_addr == NULL ) {
        break;
      }
      return func_addr;
    }
  }
  LOG_ERROR( "CPF_get_func_addr(): Cannot get function address!" )
  return NULL;
}


void *
CPF_get_extern_lib_func_by_dep( deps_t * d,
                                char * plugin_name,
                                char * func_name )
{
  uint16_t  i, j;


  if ( d == NULL) {
    LOG_ERROR("Dependency not defined!")
    exit( EXIT_FAILURE );
  }

  for ( i = 0; (void *)(*(uint64_t *)(d+i)) != NULL ; i++ ) {
  //for ( i = 0; d[i].dep_lib_name != NULL ; i++ ) {
    if ( strcmp( d[i].dep_lib_name, plugin_name ) == 0 ) {
      if ( d[i].funcs == NULL ) {
        LOG_ERROR("Functions are not defined!" )
        exit( EXIT_FAILURE );
      }
      // Search for function name
      //for ( j = 0 ; d[i].funcs[j].func_addr != NULL ; j++ ) {
      for ( j = 0 ; (void *)(*(uint64_t *)((d[i].funcs)+j)) != NULL ; j++ ) {
        if ( strcmp( d[i].funcs[j].func_name, func_name ) == 0 ) {
          return d[i].funcs[j].func_addr;
        }
      }
      LOG_ERROR("Function \"%s\" is not defined in plugin \"%s\"!",
                func_name,
                plugin_name )
      exit( EXIT_FAILURE );
    }
  }
  LOG_ERROR("Error calling plugin \"%s\": not found!", plugin_name )
  exit( EXIT_FAILURE );

  /* NOT REACHED */
  return NULL;
}


uint64_t
CPF_get_func_offset( cpf_t * cpf, char * plugin_name, char * func_name )
{
  uint16_t i, j;

  if ( cpf->num_plugins == 0 ) {
    LOG_ERROR( "CPF_get_func_offset(): There is no plugin loaded!" )
    return 0;
  }

  if ( ( plugin_name == NULL  ) || ( func_name == NULL )) {
    LOG_ERROR( "CPF_get_func_offset(): Parameters cannot be NULL!" )
    return 0;
  }

  for ( i=0 ; i < cpf->num_plugins ; i++ ) {
    if ( strcmp( plugin_name, cpf->plugin[i].name ) == 0 ) {
      for ( j=0 ; j < calc_plugin_num_funcs( cpf->plugin[i].lib_func) ; j++ ) {
        if ( ( cpf->plugin[i].lib_func[j].func_name != NULL ) && 
             ( strcmp( func_name, cpf->plugin[i].lib_func[j].func_name ) == 0 ) ) {
          return cpf->plugin[i].lib_func[j].func_offset;
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
  uint16_t num_plugins,
           c,
           l,
           r;
  uint8_t * status_l; // loaded: currently in use
  uint8_t * status_r; // reloaded: will be loaded


  if ( (*cpf) == NULL ) {
    LOG_ERROR( "CPF_reload_libs(): Plugins are not initialized yet!" )
    return EXIT_FAILURE;
  }

  if ( ( cpf_reloaded = init_to_reload( (*cpf)->path ) ) == NULL ) {
    LOG_ERROR( "CPF_reload_libs(): Cannot initialize plugin framework to reload shared libs!" )
    return EXIT_FAILURE;
  }

  if ( cpf_reloaded->num_plugins == 0 ) {
    LOG_INFO( "There's no plugin to be reloaded!" );
    CPF_free( &cpf_reloaded );
    return EXIT_SUCCESS;
  }

  status_l = (uint8_t *)malloc( (*cpf)->num_plugins * sizeof( uint8_t ) );
  if ( status_l == NULL ) {
    LOG_ERROR( "CPF_reload_libs(): Cannot allocate memory for reload plugins!" )
    exit( EXIT_FAILURE );
  }

  status_r = (uint8_t *)malloc( cpf_reloaded->num_plugins * sizeof( uint8_t ) );
  if ( status_r == NULL ) {
    LOG_ERROR( "CPF_reload_libs(): Cannot allocate memory for reload plugins!" )
    exit( EXIT_FAILURE );
  }
  if ( display_report == true ) {
    LOG_INFO( "Reloaded libs in \"%s\":", (*cpf)->path )
  }
  // Set the new lib status:
  //   * (R)eload: The loaded lib was modified ( same name and different Message Digest )
  //   * (D)elete: The loaded lib doesn't exist anymore ( lib deleted from directory )
  //   * (U)nmodified: The loaded lib wasn't modified ( same name and Message Digest )
  //   * (N)ew: The lib's name isn't in the current list ( new lib/name to load in the list )

  // Set to Delete 'D' the default status of current plugin
  // ===> All possible flags/status: 'D', 'R' or 'U'
  memset( status_l, 'D', (*cpf)->num_plugins * sizeof( uint8_t ) );
  // Set to New 'N' the default status of reloaded plugin
  // ===> All possible flags/status: 'N', 'R' or 'U'
  memset( status_r, 'N', cpf_reloaded->num_plugins * sizeof( uint8_t ) );

  // Seek for (U)nmodified libs and set the 'U' flag status:
  for ( r = 0 ; r < cpf_reloaded->num_plugins ; r++ ) {
    for ( l = 0 ; l < (*cpf)->num_plugins; l++ ) {
      if ( ( status_l[l] == 'D' ) &&
           ( strncmp( (*cpf)->plugin[l].name, cpf_reloaded->plugin[r].name, sizeof( (*cpf)->plugin->name ) ) == 0 ) &&
           ( memcmp( (*cpf)->plugin[l].blake2s256, cpf_reloaded->plugin[r].blake2s256, sizeof( (*cpf)->plugin->blake2s256 ) ) == 0 ) ) {
        status_l[l] = 'U';
        status_r[r] = 'U';
      }
    }
  }

  // Modified libs will be updated in memory:
  // set to (R)eload 'R' flash/status if the lib name is the same and Message Digest is different
  // status_l: possible flags status here: 'D' or 'U'
  // status_r: possible flags status here: 'N' or 'U'
  for ( r = 0 ; r < cpf_reloaded->num_plugins ; r++ ) {
    if ( status_r[r] != 'N' ) // here, status_r[r] == 'U'
      continue;
    for ( l = 0 ; l < (*cpf)->num_plugins; l++ ) {
      if ( ( status_l[l] == 'D' ) &&
           ( strncmp( (*cpf)->plugin[l].name, cpf_reloaded->plugin[r].name, sizeof( (*cpf)->plugin->name ) ) == 0 ) &&
           ( memcmp( (*cpf)->plugin[l].blake2s256, cpf_reloaded->plugin[r].blake2s256, sizeof( (*cpf)->plugin->blake2s256 ) ) != 0 ) ) {
        status_l[l] = 'R';
        status_r[r] = 'R';
        // Do the (R)eload process:

        // Call the possible destructor, before (R)eload
        CPF_call_plugin_dtor( &((*cpf)->plugin[l]) );
        // Free current lib dynamic allocation and close dlhandle
        CPF_free_close_plugin( &((*cpf)->plugin[l]) );
        // Copy all plugins struct
        memcpy( &(*cpf)->plugin[l], &cpf_reloaded->plugin[r], sizeof( plugin_t ) );
        // Set to NULL the "old" reloaded allocated structs, to avoid FREE
        // on these fields (the new dlhandle and funcs will be used).
        // Protect cpf_tmp against CPF_free( &cpf_reloaded ) at the end.
        cpf_reloaded->plugin[r].dlhandle = NULL;
        cpf_reloaded->plugin[r].lib_func = NULL;
        // Call the possible constructor, after (R)eload
        CPF_call_plugin_ctor( &((*cpf)->plugin[l]) );
        break;
      }
    }
  }

  // Calculate the new plugins' total number and alloc dynamic memory:
  // From current lib: 'R' and 'U' flags status
  // From reloaded lib: 'N' flag status
  num_plugins = 0;
  for ( l = 0 ; l < (*cpf)->num_plugins ; l++ ) {
    if ( status_l[l] != 'D' ) { // only 'R' and 'U' flags
      num_plugins++;
    }
    if ( status_l[l] == 'D' ) { // Call the destructor for marked (D)eleted lib
       CPF_call_plugin_dtor( &((*cpf)->plugin[l]) );
    }
    if ( display_report == true ) {
      switch( status_l[l] )
      {
        case 'D':
          LOG_INFO("Deleted: %s", (*cpf)->plugin[l].name )
          break;
        case 'R':
          LOG_INFO("Reloaded: %s", (*cpf)->plugin[l].name )
          break;
        case 'U':
          LOG_INFO("Unmodified: %s", (*cpf)->plugin[l].name )
          break;
        default:
          // NOT REACHED
          break;
      }
    }
  }

  // count the (N)ew plugins
  for ( r = 0 ; r < cpf_reloaded->num_plugins ; r++ ) {
    if ( status_r[r] == 'N' ) {
      num_plugins++;
      // call the constructor for the (N)ew plugin
      CPF_call_plugin_ctor( &(cpf_reloaded->plugin[r]) );
      if ( display_report == true ) {
        LOG_INFO("New: %s", cpf_reloaded->plugin[r].name )
      }
    }
  }

  // Alloc "num_plugins" dynamic memory for new plugins
  cpf_tmp = ( cpf_t * )calloc( num_plugins, sizeof( cpf_t ) );
  if ( cpf_tmp == NULL ) {
    LOG_ERROR( "CPF_reload_libs(): Cannot allocate memory for cpf_tmp!" )
    exit( EXIT_FAILURE );
  }

  // Set cpf_tmp fields:
  if ( snprintf( cpf_tmp->path,
                 sizeof( cpf_tmp->path ),
                 "%s",
                 (*cpf)->path ) < 0 ) {
    LOG_ERROR( "snprintf() error!" )
    exit( EXIT_FAILURE );
  }
  cpf_tmp->num_plugins = num_plugins;
  // cpf_tmp will receive only (R), (U) and (N) plugins, as calculated in num_plugins
  cpf_tmp->plugin = ( plugin_t * )calloc( num_plugins, sizeof( plugin_t ) );
  if ( cpf_tmp->plugin == NULL ) {
    LOG_ERROR( "Cannot allocate memory for tmp plugins!" )
    exit( EXIT_FAILURE );
  }

  // memcpy the (R), (U) and (N) plugins to cpf_tmp
  for( c = 0 ; c < num_plugins ; /* void */ ) {
    for ( l = 0 ; l < (*cpf)->num_plugins ; l++ ) {
      if ( status_l[l] != 'D' ) { // only 'R' and 'U' flags
        memcpy( &cpf_tmp->plugin[c], &(*cpf)->plugin[l], sizeof( plugin_t ) );
        // Protect cpf_tmp against CPF_free( cpf ) below
        (*cpf)->plugin[l].dlhandle = NULL;
        (*cpf)->plugin[l].lib_func = NULL;
        c++;
      }
    }
    for ( r = 0 ; r < cpf_reloaded->num_plugins ; r++ ) {
      if ( status_r[r] == 'N' ) {
        memcpy( &cpf_tmp->plugin[c], &cpf_reloaded->plugin[r], sizeof( plugin_t ) );
        // Protect cpf_tmp against CPF_free( &cpf_reloaded ) below
        cpf_reloaded->plugin[r].dlhandle = NULL;
        cpf_reloaded->plugin[r].lib_func = NULL;
        c++;
      }
    }
  }

  FREE( status_r )
  FREE( status_l )
  CPF_free( &cpf_reloaded );
  CPF_free( cpf );
  sort_plugins( cpf_tmp );
  check_and_set_dep( cpf_tmp );
  *cpf = cpf_tmp;
  return EXIT_SUCCESS;
}


void
CPF_unload_libs( cpf_t * cpf )
{
  if ( cpf->num_plugins == 0 ) {
    LOG_INFO( "CPF_unload_libs(): There is no plugins loaded in memory!" )
    return;
  }
  CPF_call_dtor( cpf );
  CPF_free_plugins( cpf );
}