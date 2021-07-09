#include <stdio.h>
#include "../libcpf/cpf.h"

void CPF_destructor( plugin_t * plugin ) {
  puts( "CPF_destructor() call from \"lib2.so\"." );
}

char * get_lib_name() {
  return "Msg from lib2: I add 2 to ints";
}

int do_operation(int i) {
  return i+2;
}