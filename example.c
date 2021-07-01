#include <stdio.h>
#include <stdlib.h>
#include "libcpf/cpf.h"

typedef int ( *int_int_t ) ( int );
typedef char * ( *charptr_t ) ();

int
main(void)
{
  int_int_t int_int;
  charptr_t charptr;
  int i, choice;
  char * cp;
  uint64_t offset;
  cpf_t * cpf;
  cpf_t * cpf2;

  if ( ( cpf = CPF_init( NULL ) ) == NULL ) {
    LOG_ERROR( "Cannot initialize plugin framework!" )
    return EXIT_FAILURE;
  }

  for(;;) {
    choice = 0;
    printf(
      "----------------------------------------------------------------\n"
      "Welcome to libcpf example!\n"
      "----------------------------------------------------------------\n"
      "Number of plugins loaded in \"%1$s\": %2$d\n\n"
      "1. Print loaded libs\n"
      "2. Reload libs in \"%1$s\"\n"
      "3. Unload all libs in \"%1$s\"\n"
      "4. CPF_call_func_by_name()\n"
      "5. CPF_call_func_by_addr()\n"
      "6. CPF_call_func_by_offset()\n"
      "7. CPF_get_func_addr()\n"
      "8. Init local directory plugin: plugins2\n"
      "9. Init full directory plugin: /tmp/plugins2\n"
      "0. Exit program\n"
      "\nEnter your choice: ",
      cpf->path,
      cpf->number_of_plugins
    );
    scanf( "%d", &choice );

    switch( choice )
    {
      case 1:
        CPF_print_loaded_libs( cpf );
        break;
      case 2:
        if ( CPF_reload_libs( &cpf, true ) == EXIT_FAILURE )
          LOG_ERROR("main(): reload libs error!");
        break;
      case 3:
        CPF_unload_libs( cpf );
        break;
      case 4:
        i = (int)(uint64_t)CPF_call_func_by_name( cpf,
                                                  "lib2",
                                                  "do_operation",
                                                  FP_INT_INT,
                                                  3 );
        printf( "From %s/lib2.so, CPF_call_func_by_name() do_operation: 3 + 2 = %d\n",
                cpf->path,
                i );
        if ( ( cp = (char *)CPF_call_func_by_name( cpf,
                                                  "lib1",
                                                  "get_lib_name",
                                                  FP_CHARPTR ) ) != NULL )
          printf( "From %s/lib1.so, CPF_call_func_by_name() get_lib_name: \"%s\"\n",
                  cpf->path,
                  cp );
        if ( ( cp = (char *)CPF_call_func_by_name( cpf,
                                                  "lib2",
                                                  "get_lib_name",
                                                  FP_CHARPTR ) ) != NULL )
          printf( "From %s/lib2.so, CPF_call_func_by_name() get_lib_name: \"%s\"\n",
                  cpf->path,
                  cp);
        if ( ( cp = (char *)CPF_call_func_by_name( cpf,
                                                  "lib1",
                                                  "concat_char_int",
                                                  FP_VOIDPTR_CHARPTR_INT,
                                                  (char *)"number",
                                                  255 ) ) != NULL ) {
          printf( "From %s/lib1.so, CPF_call_func_by_name() concat_char_int: \"%s\"\n",
                  cpf->path,
                  cp);
          FREE(cp)
        }
        break;
      case 5:
        printf( "From %s/lib1.so, CPF_call_func_by_addr() do_operation: 1 + 5 = %d\n",
                cpf->path,
                (int)(uint64_t)CPF_call_func_by_addr(
                  CPF_get_func_addr( cpf, "lib1", "do_operation" ), FP_INT_INT, 5 )
              );
        break;
      case 6:
        offset = CPF_get_func_offset( cpf, "lib2", "get_lib_name" );
        printf( "From %s/lib2.so, CPF_call_func_by_offset() 0x%lx: \"%s\"\n",
                cpf->path,
                offset,
                (char *)CPF_call_func_by_offset( cpf, "lib2", offset, FP_CHARPTR )
              );
        break;
      case 7:
        if ( ( int_int = CPF_get_func_addr( cpf, "lib2", "do_operation" ) ) != NULL )
          printf( "From %s/lib2.so, CPF_get_func_addr() do_operation: 1 + 2 = %d\n",
                  cpf->path,
                  int_int(1) );
        if ( ( charptr = CPF_get_func_addr( cpf, "lib1", "get_lib_name" ) ) != NULL )
          printf( "From %s/lib1.so, CPF_get_func_addr() get_lib_name: \"%s\"\n",
                  cpf->path,
                  charptr() );
        break;
      case 8:
        puts( "Initializing plugins2 directory..." );

        /*
         * CPF_init( "plugin_directory" ) ==> for relative local directory
         * or CPF_init( "/my/plugin/directory" ) ==> full plugin directory path
         */
        if ( ( cpf2 = CPF_init( "plugins2" ) ) == NULL ) {
          LOG_ERROR( "Cannot initialize plugin framework!" )
          return EXIT_FAILURE;
        }
        offset = CPF_get_func_offset( cpf2, "lib4", "do_operation" );
        printf( "From %s/lib4.so, CPF_call_func_by_offset() 0x%lx: 15 + 4 = %d\n",
                cpf2->path,
                offset,
                (int)(uint64_t)CPF_call_func_by_offset( cpf2,
                                                        "lib4",
                                                        offset,
                                                        FP_INT_INT,
                                                        15 ) );
        CPF_cleanup( &cpf2 );
        break;
      case 9:
        puts( "Initializing /tmp/plugins2 directory..." );

        /*
         * CPF_init( "plugin_directory" ) ==> for relative local directory
         * or CPF_init( "/my/plugin/directory" ) ==> full plugin directory path
         */
        if ( ( cpf2 = CPF_init( "/tmp/plugins2" ) ) == NULL ) {
          LOG_ERROR( "Cannot initialize plugin framework!" )
          return EXIT_FAILURE;
        }
        offset = CPF_get_func_offset( cpf2, "lib1", "do_operation" );
        printf( "From %s/lib1.so, CPF_call_func_by_offset() 0x%lx: 20 + 1 = %d\n",
                cpf2->path,
                offset,
                (int)(uint64_t)CPF_call_func_by_offset( cpf2,
                                                        "lib1",
                                                        offset,
                                                        FP_INT_INT,
                                                        20 ) );
        CPF_cleanup( &cpf2 );
        break;
      default:
        CPF_cleanup( &cpf );
        puts( "Exiting..." );
        return EXIT_SUCCESS;
        break;
    }
  }
  return EXIT_SUCCESS;
}