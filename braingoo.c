#include "int_stack.c"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define TRANSPILED_FILENAME ".transpiled.c"
#define DEFAULT_TAPE_SIZE 30000
#define DEFAULT_TAPE_SIZE_STR "30000"
#define TAPE "tape"
#define TAPE_HEAD "head"
#define FINAL_COMPILATION_C_FLAGS "-O3 -Wno-unused-result"

//#define INCLUDE_TRANSPILATION_ASSERTS 
#ifdef INCLUDE_TRANSPILATION_ASSERTS
    #define TRANS_ASSERT(file, err) fprintf(file, "\tassert(%s);\n", err)
#else
    #define TRANS_ASSERT(file, err) 
#endif

#define WRAP_CHECK(file) fprintf(file, "if(%s < 0){ %s = %d; }else if(%s >= %d){ %s = 0; }\n", TAPE_HEAD, TAPE_HEAD, DEFAULT_TAPE_SIZE-1, TAPE_HEAD, DEFAULT_TAPE_SIZE, TAPE_HEAD);

// returns number of parsed arguments
int get_cl_args(int argc, char** argv, int num_possible_args, ...){
    va_list args;
    va_start(args, num_possible_args);
    
    int i = 1;
    char** arg;
    for(i = 1; i < argc; i++){
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

void print_header(FILE* file){
    // headers
    fprintf(file, "#include <stdio.h>\n#include <stdint.h>\n#include <stdlib.h>\n#include <assert.h>\n");
    // macro to check wrapping of read/write head
    fprintf(file, "#define WRAP_CHECK if(%s < 0){ %s = %d; }else if(%s >= %d){ %s = 0; };\n", TAPE_HEAD, TAPE_HEAD, DEFAULT_TAPE_SIZE-1, TAPE_HEAD, DEFAULT_TAPE_SIZE, TAPE_HEAD);
    // declare main and tape + read/write head variables
    fprintf(file, "\
int main(void){\n\t\
long int %s = 0;\n\t\
uint8_t* %s = malloc(%d);\n\t\
for(int i = 0; i < %d; i++) %s[i] = 0;\n", TAPE_HEAD, TAPE, DEFAULT_TAPE_SIZE, DEFAULT_TAPE_SIZE, TAPE);
}

void print_footer(FILE* file){
    fprintf(file, "\n\tfree(%s);\n\treturn 0;\n}", TAPE);
}

int compile_c(const char* out_filename, bool save_transpilation){
    char cmd[1000] = "gcc " FINAL_COMPILATION_C_FLAGS " -o ";
    if(out_filename){
        strncat(cmd, out_filename, 100);
    }else{
        strncat(cmd, "out", 100);
    }
    strncat(cmd, " " TRANSPILED_FILENAME, 100);
    int status = system(cmd);
    if(!status){
        if(!save_transpilation && system("rm " TRANSPILED_FILENAME)){
            printf("Warning: couldn't delete " TRANSPILED_FILENAME);
        }
    }
    return status;
}

void write_instruction(FILE* file, char byte, IntStack* label_stack, int* label_count){
    switch(byte){
        case '+': 
            fprintf(file, "\t%s[%s]++; WRAP_CHECK\n", TAPE, TAPE_HEAD);
            break;
        case '-': 
            fprintf(file, "\t%s[%s]--; WRAP_CHECK\n", TAPE, TAPE_HEAD);
            break;
        case '<': 
            fprintf(file, "\t%s--; WRAP_CHECK\n", TAPE_HEAD);
            TRANS_ASSERT(file, TAPE_HEAD" >= 0 && "TAPE_HEAD" < "DEFAULT_TAPE_SIZE_STR);
            break;
        case '>': 
            fprintf(file, "\t%s++; WRAP_CHECK\n", TAPE_HEAD);
            TRANS_ASSERT(file, TAPE_HEAD" >= 0 && "TAPE_HEAD" < "DEFAULT_TAPE_SIZE_STR);
            break;
        case '.': 
            fprintf(file, "\tputc(%s[%s], stdout); WRAP_CHECK\n", TAPE, TAPE_HEAD);
            break;
        case ',': 
            fprintf(file, "\t(void)scanf(\"%%c\", &%s[%s]); WRAP_CHECK\n", TAPE, TAPE_HEAD);
            break;
        case '[': 
            fprintf(file, "\tif(!%s[%s]) goto label_close%d; WRAP_CHECK\n", TAPE, TAPE_HEAD, *label_count);
            fprintf(file, "\tlabel_open%d:\n", *label_count);
            int_stack_push(label_stack, *label_count);
            (*label_count)++;
            break;
        case ']': 
            fprintf(file, "\tif(%s[%s]) goto label_open%d; WRAP_CHECK\n", TAPE, TAPE_HEAD, int_stack_peek(*label_stack));
            fprintf(file, "\tlabel_close%d:\n", int_stack_peek(*label_stack));
            int_stack_pop(label_stack);
            break;
        default: break; // comment characters
    }
}

void print_usage(){
    printf("Usage: ./braingoo <output filename> <input filename> <-k (keep transpilation file)>\n");
}

int main(int argc, char** argv){
    /* command line parsing */
    char* src_filename = "";
    char* out_filename = "";
    char* keep_transpilation_arg = "";
    int num_args = get_cl_args(argc, argv, 3, &out_filename, &src_filename, &keep_transpilation_arg); 
    if(num_args == 0 || num_args == 1){
        printf("Error: no input file provided\n");
        print_usage();
        return 1;
    }else if(num_args > 3){
        printf("Warning: encountered more arguments than expected... ignoring all but the first 3 arguments\n");
        print_usage();
    }

    FILE* src = open_src(src_filename); // failure + exit possible
    const bool keep_transpilation = keep_transpilation_arg ? !strncmp(keep_transpilation_arg, "-k", 2) : false;
    
    /* open file for transpilation to c. binary is generating by passing this file to gcc */
    FILE* transpilation = fopen(TRANSPILED_FILENAME, "w");
    if(!transpilation){
        printf("Error: couldn't open/create .transpilation.c");
        fclose(src);
        return 1;
    }

    /* the main transiplation loop */
    int label_count = 0;
    IntStack label_stack = int_stack_new(1000);

    print_header(transpilation);
    uint8_t byte = fgetc(src);
    while((int8_t)byte != EOF){
        write_instruction(transpilation, byte, &label_stack, &label_count);
        byte = fgetc(src);
    }
    print_footer(transpilation);

    /* clean up after yourself! */
    fclose(src);
    fclose(transpilation);
    int_stack_free(&label_stack);

    /* gcc compilaton */
    if(compile_c(out_filename, keep_transpilation)){
        printf("Error: gcc returned non-zero exit code, did braingoo output a bad transpilation?\n");
        return 1;
    }

    return 0;
}
