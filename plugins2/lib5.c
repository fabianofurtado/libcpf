#include <stdio.h>
#include <string.h>
#include "../libcpf/cpf.h"

#define PLUGIN_VERSION "v1.0.0"


static plugin_ctx_t  p_ctx;

static deps_t  arr_deps[] = { // this array must finish with NULL value
  { NULL }
};

plugin_ctx_t *
CPF_init_ctx()
{
  strncpy( p_ctx.version, PLUGIN_VERSION, sizeof( p_ctx.version )-1 );
  p_ctx.deps = arr_deps;

  return &p_ctx;
}


char * get_lib_name() {
  return "Msg from lib5!";
}

int do_operation(int i) {
  return i+5;
}