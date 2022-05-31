#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../libcpf/cpf.h"

#define PLUGIN_VERSION  "v1.0.0"

#define DEP_LIB2        "lib2"
#define DEP_OTHER_LIB4  "other/lib4"

static plugin_ctx_t  p_ctx;

static deps_t  arr_deps[] = { // this array must finish with NULL value
  { DEP_LIB2 },
  { DEP_OTHER_LIB4 },
  { NULL }
};

plugin_ctx_t *
CPF_init_ctx()
{
  strncpy( p_ctx.version, PLUGIN_VERSION, sizeof( p_ctx.version )-1 );
  p_ctx.deps = arr_deps;

  return &p_ctx;
}


void
CPF_constructor( plugin_t * p )
{
  printf( "CPF_constructor() call from \"%s"PLUGIN_EXTENSION"\" %s.\n",
          p->name,
          p_ctx.version );
}

void
CPF_destructor( plugin_t * p )
{
  printf( "CPF_destructor() call from \"%s"PLUGIN_EXTENSION"\" %s.\n",
          p->name,
          p_ctx.version );
}


void *
concat_char_int(char * c, int i)
{
  void * ret;

  if ( ( ret = malloc( strlen( c ) + 100 ) ) == NULL ) {
    printf("concat_char_int() from lib1: cannot malloc!\n");
    exit( EXIT_FAILURE );
  }

  sprintf( (char *)ret, "concat_char_int() from lib1: %s %d", c, i );

  return ret;
}

char *
get_lib_name() {
  return "Msg from lib1!";
}

static int
call_lib2_fcn() {

  int (*lib2_do_oper)(int) = CPF_get_extern_lib_func_by_dep(
                                arr_deps,
                                DEP_LIB2,
                                "do_operation" );
  return lib2_do_oper( 1 );
}

int
do_operation( int i )
{
  return i + call_lib2_fcn();
}