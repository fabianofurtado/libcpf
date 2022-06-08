# libcpf
CPF means C Plugin Framework, and it's an Open source Plugin Framework software written in C, to be compiled and used into your Linux projects.

## Dependencies
- openssl: uses SHA1 hash function

## Learn by FAQs
### Please, define "plugins".
Plugins are a piece of software used to extend or add functionality to your program, without the need to recompile the project. Libcpf uses _.so_ (shared object) file type as plugin resource, and the power of dynamic linking to load it into memory. "Module" is a synonym for "plugin", but we are going to use the name "plugin" here.

### Is libcpf ready to use?
**Yes** and **no**! Libcpf is based on C function pointers concept and there's a need to create functions prototype to be able to use the plugins' functions. Fortunately, you'll need to modify only two files, _fp_prototype.c_ and _fp_prototype.h_, and there're macros defined to help you to do that.

### Is libcpf a library or a framework?
Well, it isn't an easy question to answer. As a library, libcpf provides several functions to manage the plugins. However, libcpf is based on function prototypes and you must define these prototypes to call the plugins functions.

Both the Library and Framework plays a vital role in software development. A library performs a specific or well-defined operation, whereas a framework provides a skeleton where programmers define the application content of the operation.

A library provides a set of helper functions/objects/modules which your application code calls for specific functionality.

Framework, on the other hand, has defined open or unimplemented functions or objects which the user writes to create a custom application.

### What is a function prototype?
A function prototype is a function declaration that specifies the data types of its arguments in the parameter list, and the function's return type. Example:

    int sum( int a, int b );

In this case, the function _sum_ needs two integer parameters ("a" and "b") and returns an integer.

### How can I create new function prototypes in libcpf?
As I said, you'll only need to modify two files, _fp_prototype.c_ and _fp_prototype.h_.

There's a commentary inside _fp_prototype.h_ file explaining this process. Check it out.

### How can I start using libcpf?
Shortly, you can include the libcpf into your program ...

    #include "libcpf/cpf.h"

... and compile it with:

    gcc -ldl -lcrypto <my_program.c> -o <my_program>

Check the example program out to see how it works.

### What functions libcpf provides?
All the function prototypes are defined in _cpf.h_ header file and they're self-explanatory. Check the example program out to see how the functions work. The example program also has 2 libs (_plugins/lib1.so_ and _plugins/lib2.so_) with some _boilerplate code_ to configure the lib dependencies. It's very straightforward.

### What do you mean by "CPF_call_func_by_offset()"?
A pointer is a kind of variable that holds the address of another variable. The same concept applies to function pointers, but instead of pointing to variables, they point to functions. Under the hood, when you call a function by name, there's a mechanism to translate this name into a memory address. So, there's no need to use the function's name to call it. Every function is referenced by a base memory address plus an offset. If you know the offset, you can call the function by this "number", without the base memory address.

### How can I discover the offset from a specific function?
- Use libcpf _CPF_print_loaded_libs()_ function to print the loaded libs or use _CPF_get_func_offset()_ directly.
- Use _readelf_ utility if the binary isn't stripped.
- You could use a software reverse engineering tools like _Ghidra_ (https://ghidra-sre.org/) to disassemble the shared object/plugin functions and discover the offsets.

### Can I delete the function's name from plugins?
YES! If you want to "protect" your code and delete the function's name from it, you can hexedit the plugin's file and fill the function's name with NULL bytes (search for _Dynamic Symbol Table_ - _DT_SYMTAB_ and/or _DT_STRTAB_). That said, you won't be able to call a function by name anymore (you'll be able to call the functions only by its offset).
But, instead of filling the function's name with NULL bytes, you could use the reserved word _static_ to define de function. For instance, define these two functions inside _lib_visible.so_.

    void IamVisibleToTheWorld() {...}
    static void IamNOTVisibleToTheWorld() {...}

And then run:

    # readelf -s lib_visible.so | grep -i visible

Can you notice the result?

### Can I secure my plugin from "hackers"?
Shortly, you cannot! :(

Instead of that, you could remove the function's name and strip the binary using strip (https://www.gnu.org/software/binutils/) or sstrip (https://www.muppetlabs.com/~breadbox/software/elfkickers.html) utilities, to make the reverse engineering more difficult.

### Can I call generic constructor and destructor functions inside the plugin?
YES! You can declare the prototypes...

    #include "../libcpf/cpf.h"

    void CPF_constructor( plugin_t * plugin ) { ... }
    void CPF_destructor( plugin_t * plugin ) { ... }

...inside the plugin and it'll be called automatically by _libcpf_. See the _example_.

### How can I define the dependencies between libs?
Let's look at _lib1.c_ code. You must declare a plugin context variable of type _plugin\_ctx\_t_, the array of dependencies _deps\_t_ and a _plugin\_ctx\_t * CPF\_init\_ctx()_ function.
This function will initialize the plugin's context and set the dependencies. It's mandatory the declaration of this function and it cannot be declared as _static_!
The _deps\_t_ array is used to set the dependencies. In this example, we have two dependencies: _lib2_ and _other/lib4_.

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
    ...

## Changing the default names of plugin directory, plugin extension, plugin init context function, constructor and destructor functions
There are five _#defines_ in _cpf.h_ to customize it:

    #define PLUGIN_DIRNAME          "plugins"         // default directory name
    #define PLUGIN_EXTENSION        ".so"             // default plugin extension
    #define PLUGIN_INIT_CTX_FUNC    "CPF_init_ctx"    // default plugin init context func name
    #define PLUGIN_CONSTRUCTOR_FUNC "CPF_constructor" // default plugin constructor func name
    #define PLUGIN_DESTRUCTOR_FUNC  "CPF_destructor"  // default plugin destructor func name

## Using the provided example
There's one example explaining the use of libcpf. To compile it, run:

    $ cd libcpf/
    $ make
    $ cd ../plugin/
    $ make
    $ cd ../plugin2/
    $ make
    $ cd ..
    $ make
    $ cp plugins2/lib4.so plugins/other/

Execute the example program:

    $ ./example

To clean the files generated by _make_:

    $ make clean

## How can I know what libs are loaded in memory?
The libcpf provides the function _void CPF\_print\_loaded\_libs( cpf\_t * cpf );_. Look at the _example_ program. For instance:

    Total of 3 loaded plugins in "/tmp/libcpf/plugins/":
        01) "lib1" (/tmp/libcpf/plugins/lib1.so) version v1.0.0 has:
            * base address: 0x7fa149770000
            * hash id sha1: 55c4562cf11adda3952af997ce9e5306fb93c0c0
            * 2 dependencies: "lib2"  "other/lib4"  
            * 3 functions:
                * "concat_char_int" (offset 0x1200 / address 0x7fa149771200)
                * "get_lib_name" (offset 0x1260 / address 0x7fa149771260)
                * "do_operation" (offset 0x1270 / address 0x7fa149771270)
        02) "lib2" (/tmp/libcpf/plugins/lib2.so) version <NOT DEFINED> has:
            * base address: 0x7fa149775000
            * hash id sha1: 069c632efbf00af8cbded7dfe33ba8bc62123dc1
            * 1 dependency: "lib1"  
            * 2 functions:
                * "get_lib_name" (offset 0x1140 / address 0x7fa149776140)
                * "do_operation" (offset 0x1150 / address 0x7fa149776150)
        03) "other/lib4" (/tmp/libcpf/plugins/other/lib4.so) version v1.0.0 has:
            * base address: 0x7fa14976b000
            * hash id sha1: 32d09a91afb04052f2db0fb9adf0cbc2cd232015
            * 0 dependencies
            * 2 functions:
                * "get_lib_name" (offset 0x1140 / address 0x7fa14976c140)
                * "do_operation" (offset 0x1150 / address 0x7fa14976c150)

## Releases
- v0.0.5 - 2022-06-08 - Bug fix
- v0.0.4 - 2022-05-31 - Simple lib dependencies control
- v0.0.3 - 2022-05-11 - Bug fixes
- v0.0.2 - 2021-07-08 - Constructor and Destructor function calls
- v0.0.1 - 2021-07-01 - Initial release

**TODO**
- remove the _FP_WRAPPER macro_ and create function pointer prototype dynamically. Is it possible?
