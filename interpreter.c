#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#define DEFAULT_TAPE_SIZE 30000
uint8_t tape[DEFAULT_TAPE_SIZE];
size_t head = 0;

size_t crash_ptr;
uint8_t crash_instr;
void save_crash_point(size_t ptr, uint8_t byte){
    crash_ptr = ptr;
    crash_instr = byte;
}

void halt_and_catch_fire(const char* reason, bool blame_byte){
    printf("Interpreter reported a fatal runtime error! Reason was:\n\t%s\n", reason);
    if(blame_byte){
        printf("Crash occured when executing instruction '%c' (%zuth byte of source code)\n", crash_instr, crash_ptr);
    }
    exit(1);
}

enum EXE_ACTION{
    NEXT_INSTRUCTION = 0,
    JUMP_FORWARD,
    JUMP_BACK,
};

enum EXE_ACTION exe_instruction(uint8_t byte){
    switch(byte){
        case '+': 
            tape[head]++;
            break;
        case '-': 
            tape[head]--;
            break;
        case '<': 
            if(head == 0){ head = DEFAULT_TAPE_SIZE; }
            else { head--; }
            break;
        case '>': 
            if(head == DEFAULT_TAPE_SIZE) { head = 0; }
            else { head++; }
            break;
        case '.': 
            putc(tape[head], stdout);
            break;
        case ',': 
            (void)scanf("%c", &tape[head]);
            break;
        case '[': 
            if(!tape[head]){
                return JUMP_FORWARD;
            }
            return NEXT_INSTRUCTION;
            break;
        case ']': 
            if(tape[head]){
                return JUMP_BACK;
            }
            return NEXT_INSTRUCTION;
            break;
        default: break; // comment characters
    }
    return NEXT_INSTRUCTION;
}

void init_tape(){
    for(int i = 0; i < DEFAULT_TAPE_SIZE; i++){ tape[i] = 0; }
    head = 0;
}

size_t count_bytes(FILE* src){
    size_t count = 0;
    while(fgetc(src) != EOF) count++;
    fseek(src, 0, SEEK_SET);
    return count;
}

size_t load_src_code(FILE* src, uint8_t** dest){
    size_t count = count_bytes(src);    
    uint8_t* data = malloc(sizeof(uint8_t)*count);
    for(size_t i = 0; i < count; i++){
        data[i] = fgetc(src);
    }
    *dest = data; 
    return count;
}

int jump_forward(size_t* instr_ptr, uint8_t* instructions, size_t instructions_size_bytes){
    /*
     * A crash is possible here if the jump can't terminate correctly, 
     * we'll save our starting point if a crash needs to be reported.
     */
    save_crash_point(*instr_ptr, instructions[*instr_ptr]);

    int opened = 0, closed = 0;;
    do{
        (*instr_ptr)++;

        if(instructions[*instr_ptr] == ']'){
            if(opened == closed){
                (*instr_ptr)++; // advance to byte after bracket
                return 0;
            }else{
                closed++;
            }
        }else if(instructions[*instr_ptr] == '['){
            opened++;
        }

    }while(*instr_ptr < instructions_size_bytes);
    /* 
     * Reached the end of instructions without encountering closing bracket,
     * the interpreter should now crash
     */
    return 1; 
}

int jump_back(size_t* instr_ptr, uint8_t* instructions){
    /*
     * A crash is possible here if the jump can't terminate correctly, 
     * we'll save our starting point if a crash needs to be reported.
     */
    save_crash_point(*instr_ptr, instructions[*instr_ptr]);

    int opened = 0, closed = 0;;
    for(;;){ //  returns or crashes
        if(*instr_ptr == 0) break; // crash
        (*instr_ptr)--;

        if(instructions[*instr_ptr] == ']'){
            closed++;
        }else if(instructions[*instr_ptr] == '['){
            if(opened == closed){
                return 0;
                (*instr_ptr)++; // advance to byte after bracket
            }else{
                opened++;
            }
        }
    };
    /* 
     * Reached the end of instructions without encountering closing bracket,
     * the interpreter should now crash
     */
    return 1; 
}

int interpret(FILE* src){
    uint8_t* instructions = NULL;
    size_t src_size_bytes = load_src_code(src, &instructions);
    if(instructions == NULL){
        printf("Error: Interpreter read 0 bytes from source code... was the source empty?\n");
        return 1;
    }

    size_t instr_ptr = 0; 
    init_tape();

    while(instr_ptr <= src_size_bytes){
        enum EXE_ACTION action = exe_instruction(instructions[instr_ptr]);
        int status = 0;
        switch(action){
            case NEXT_INSTRUCTION:
                instr_ptr++;
                break;
            case JUMP_FORWARD:
                status = jump_forward(&instr_ptr, instructions, src_size_bytes);
                if(status){
                    halt_and_catch_fire("Expected corresponding closing bracket, encountered end of instructions instead", true);
                }
                break;
            case JUMP_BACK:
                status = jump_back(&instr_ptr, instructions);
                if(status){
                    halt_and_catch_fire("Expected corresponding opening bracket, encountered start of instructions instead", true);
                }
                break;
            default: break; // comment bytes
        }
    }

    free(instructions);
    return 0;
}
