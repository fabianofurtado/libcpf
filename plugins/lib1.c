#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void * concat_char_int(char * c, int i) {
  void * ret;

  if ( ( ret = malloc( strlen( c ) + 100 ) ) == NULL ) {
    printf("concat_char_int() from lib1: cannot malloc!\n");
    exit( EXIT_FAILURE );
  }

  sprintf( (char *)ret, "concat_char_int() from lib1: %s %d", c, i );

  return ret;
}

char * get_lib_name() {
  return "Msg from lib1: I add 1 to ints";
}

int do_operation(int i) {
  return i+1;
}