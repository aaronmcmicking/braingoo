#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifndef INT_STACK_IMPLEMENTATION
    #define INT_STACK_IMPLEMENTATION
#endif
#include "int_stack.h"

//#define INCLUDE_TRANSPILATION_ASSERTS 
#ifdef INCLUDE_TRANSPILATION_ASSERTS
    #define TRANS_ASSERT(file, err) fprintf(file, "assert(%s);", err)
#else
    #define TRANS_ASSERT(file, err) 
#endif

#define TRANSPILED_FILENAME ".transpiled.c"
#define DEFAULT_TAPE_SIZE 30000
#define DEFAULT_TAPE_SIZE_STR "30000"
#define TAPE "tape"
#define TAPE_HEAD "head"
#define FINAL_COMPILATION_C_FLAGS "-O3 -Wno-unused-result"

void print_header(FILE* file){
    // headers
    fprintf(file, "#include <stdio.h>\n#include <stdint.h>\n#include <stdlib.h>\n#include <assert.h>\n\n");
    // macro to check wrapping of read/write head
    fprintf(file, "#define WRAP_CHECK if(%s < 0){ %s = %d; }else if(%s >= %d){ %s = 0; };\n\n", TAPE_HEAD, TAPE_HEAD, DEFAULT_TAPE_SIZE-1, TAPE_HEAD, DEFAULT_TAPE_SIZE, TAPE_HEAD);
    // declare main and tape + read/write head variables
    fprintf(file, "\
int main(void){\n\t\
long int %s = 0;\n\t\
uint8_t* %s = malloc(%d);\n\t\
for(int i = 0; i < %d; i++) %s[i] = 0;\n\n\t", TAPE_HEAD, TAPE, DEFAULT_TAPE_SIZE, DEFAULT_TAPE_SIZE, TAPE);
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
    bool newline = true;
    switch(byte){
        case '+': 
            fprintf(file, "%s[%s]++; WRAP_CHECK ", TAPE, TAPE_HEAD);
            break;
        case '-': 
            fprintf(file, "%s[%s]--; WRAP_CHECK ", TAPE, TAPE_HEAD);
            break;
        case '<': 
            fprintf(file, "%s--; WRAP_CHECK ", TAPE_HEAD);
            TRANS_ASSERT(file, TAPE_HEAD" >= 0 && "TAPE_HEAD" < "DEFAULT_TAPE_SIZE_STR);
            break;
        case '>': 
            fprintf(file, "%s++; WRAP_CHECK ", TAPE_HEAD);
            TRANS_ASSERT(file, TAPE_HEAD" >= 0 && "TAPE_HEAD" < "DEFAULT_TAPE_SIZE_STR);
            break;
        case '.': 
            fprintf(file, "putc(%s[%s], stdout); WRAP_CHECK ", TAPE, TAPE_HEAD);
            break;
        case ',': 
            fprintf(file, "(void)scanf(\"%%c\", &%s[%s]); WRAP_CHECK ", TAPE, TAPE_HEAD);
            break;
        case '[': 
            fprintf(file, "if(!%s[%s]) goto label_close%d; WRAP_CHECK \n\t", TAPE, TAPE_HEAD, *label_count);
            fprintf(file, "label_open%d:", *label_count);
            int_stack_push(label_stack, *label_count);
            (*label_count)++;
            break;
        case ']': 
            fprintf(file, "if(%s[%s]) goto label_open%d; WRAP_CHECK \n\t", TAPE, TAPE_HEAD, int_stack_peek(*label_stack));
            fprintf(file, "label_close%d:", int_stack_peek(*label_stack));
            int_stack_pop(label_stack);
            break;
        default: newline = false; break; // comment characters
    }
    if(newline){ fprintf(file, "\n\t"); }
}

int compile(FILE* src, char* out_filename, bool keep_transpilation){
    /* open file for transpilation to c. binary is generating by passing this file to gcc */
    FILE* transpilation = fopen(TRANSPILED_FILENAME, "w");
    if(!transpilation){
        printf("Error: couldn't open/create .transpilation.c");
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
    fclose(transpilation);
    int_stack_free(&label_stack);

    /* gcc compilaton */
    if(compile_c(out_filename, keep_transpilation)){
        printf("Error: gcc returned non-zero exit code, did braingoo output a bad transpilation?\n");
        return 1;
    }
    
    return 0;
}
