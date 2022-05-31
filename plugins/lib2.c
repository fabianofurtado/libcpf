#include <stdio.h>
#include "../libcpf/cpf.h"


#define DEP_LIB1        "lib1"

static plugin_ctx_t  p_ctx;

static deps_t  arr_deps[] = { // this array must finish with NULL value
  { DEP_LIB1 },
  { NULL }
};

plugin_ctx_t *
CPF_init_ctx()
{
  p_ctx.version[0] = '\0';
  p_ctx.deps = arr_deps;

  return &p_ctx;
}

void
CPF_destructor( plugin_t * plugin )
{
  puts( "CPF_destructor() call from \"lib2.so\"." );
}

char *
get_lib_name()
{
  return "Msg from lib2!";
}

int
do_operation(int i)
{
  return i+2;
}