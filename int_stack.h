#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    int* contents;
    size_t count;
    size_t capacity;
}IntStack;

IntStack    int_stack_new       (size_t elements);
void        int_stack_init      (IntStack stack);
void        int_stack_free      (IntStack* stack);

void        int_stack_push      (IntStack* stack, int value);
int         int_stack_pop       (IntStack* stack);
int         int_stack_peek      (IntStack stack);

void        int_stack_resize    (IntStack* stack, size_t new_elements, bool copy_contents);
void        int_stack_print     (IntStack stack);

#ifdef INT_STACK_IMPLEMENTATION

IntStack int_stack_new(size_t elements){
    int* contents = malloc(elements); // size = # bytes for char array
    assert(contents != NULL);
    return (IntStack){contents, 0, elements};
}

void int_stack_free(IntStack* stack){
    if(stack->contents) free(stack->contents);
    stack->count = 0;
    stack->capacity = 0;
}

void int_stack_init(IntStack stack){
    for(size_t i = 0; i < stack.count; i++){
        stack.contents[i] = 0;
    }
}

void int_stack_resize(IntStack* stack, size_t new_elements, bool copy_contents){
    IntStack new_stack = int_stack_new(new_elements);
    if(copy_contents){
        for(size_t i = 0; i < stack->count; i++){
            new_stack.contents[i] = stack->contents[i];
        }
        new_stack.count = stack->count;
    }
    free(stack->contents);
    stack->contents = new_stack.contents;
    stack->capacity = new_stack.capacity;
}

void int_stack_print(IntStack stack){
    for(size_t i = 0; i < stack.count; i++) printf("%d ", stack.contents[i]);
    printf("\n");
}

void int_stack_push(IntStack* stack, int value){
    if(stack->count >= stack->capacity){
        int_stack_resize(stack, stack->capacity*2, true);
    }
    stack->contents[stack->count] = value;
    stack->count++;
}

int int_stack_pop(IntStack* stack){
    if(!stack->count){
        printf("Warning: tried to pop off empty stack");
        return 0;
    }
    stack->count--;
    return stack->contents[stack->count];
}

int int_stack_peek(IntStack stack){
    if(!stack.count){
        printf("Warning: tried to peek empty stack");
        return 0;
    }
    return stack.contents[stack.count-1];
}

#endif // INT_STACK_IMPLEMENTATION








