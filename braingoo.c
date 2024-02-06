#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include "compiler.c"
#include "interpreter.c"

// returns number of parsed arguments
int get_cl_args(int argc, char** argv, int num_possible_args, ...){
    va_list args;
    va_start(args, num_possible_args);

    int max_loop = argc < num_possible_args+1 ? argc : num_possible_args+1;
    
    int i = 1;
    char** arg;
    for(i = 1; i < max_loop; i++){
        arg = va_arg(args, char**);
        *arg = argv[i];
    }

    va_end(args);
    return i-1;
}

FILE* open_src(char* src_filename){
    FILE* src_file = fopen(src_filename, "r");
    if(src_file == NULL){
        printf("Error: failed to open source file '%s'\n", src_filename);
        exit(1);
    }
    return src_file;
}

void print_usage(){
    printf("Compiler usage: ./braingoo -compile <output filename> <input filename> <-k (keep transpilation file)>\n");
    printf("Interpreter usage: ./braingoo -interp <input filename>\n");
}

int main(int argc, char** argv){
    /* command line parsing */
    char* compiling_str = "";
    get_cl_args(argc, argv, 1, &compiling_str); 
    if(!compiling_str){
        printf("Error: no arguments provided\n");
        print_usage();
        return 1;
    }
    argv++;
    argc--;

    char* src_filename = "";
    if(!strncmp(compiling_str, "-compile", 9)){
        /* command line parsing */
        char* out_filename = "";
        char* keep_transpilation_arg = "";
        int num_args = get_cl_args(argc, argv, 3, &out_filename, &src_filename, &keep_transpilation_arg); 
        if(num_args == 0 || num_args == 1){
            printf("Error: no input file provided\n");
            print_usage();
            return 1;
        }else if(num_args > 3){
            printf("Warning: encountered more arguments than expected... ignoring all but the first 4 arguments\n");
            print_usage();
        }

        /* compile */
        FILE* src = open_src(src_filename); // failure + exit possible
        const bool keep_transpilation = keep_transpilation_arg ? !strncmp(keep_transpilation_arg, "-k", 2) : false;
        compile(src, out_filename, keep_transpilation);
        fclose(src);
    }else if(!strncmp(compiling_str, "-interp", 8)){
        /* command line parsing */
        int num_args = get_cl_args(argc, argv, 1, &src_filename); 
        if(num_args > 1){
            printf("Warning: encountered more arguments than expected... ignoring all but the first argument\n");
        }else if(!num_args){
            printf("Error: no input file provided\n");
            print_usage();
            return 1;
        }

        /* interpret */
        FILE* src = open_src(src_filename); // failure + exit possible
        interpret(src);
        fclose(src);
    }else{
        printf("Error: did not invoke either compiler or interpreter\n");
        print_usage();
        return 1;
    }

    return 0;
}
