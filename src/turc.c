#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

typedef char Value;

// https://stackoverflow.com/a/46013943
char *strndup(const char *s, size_t n) {
    size_t i;
    for (i = 0; i < n && s[i] != '\0'; i++)
        ;
    
    char *copy = malloc(n + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, s, i);
    copy[i] = '\0';
    return copy;
}

char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read < file_size) {
        fclose(file);
        free(buffer);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    fclose(file);
    return buffer;
}

typedef enum {
    LEFT,
    RIGHT,
} Direction;

typedef struct {
    char* state;
    char* new_state;
    Value value;
    Value new_value;
    Direction direction;
} Transition;

typedef struct {
    char* initial_state;
    Value default_value;
    Transition* transitions;
    size_t transition_count;
} Machine;

typedef struct {
    Value* values;
    size_t capacity;
} Tape;

void free_transition(Transition* transition) {
    free(transition->state);
    free(transition->new_state);
}

void free_machine(Machine* machine) {
    free(machine->initial_state);
    
    for (size_t i = 0; i < machine->transition_count; i++) {
        free_transition(&machine->transitions[i]);
    }

    free(machine->transitions);
    free(machine);
}

Machine* parse_machine(const char* source) {
    if (source == NULL) {
        return NULL;
    }

    Machine* machine = calloc(sizeof(Machine), 1);

    // read initial state
    const char* start = source;
    while (*source != '\n') {
        source++;
    }
    machine->initial_state = strndup(start, source - start);

    source++; // skip newline

    // read default value
    machine->default_value = *(source++);

    source++; // skip newline

    // read transitions
    while (*source != '\0') {
        machine->transitions = realloc(machine->transitions, (++machine->transition_count) * sizeof(Transition));

        Transition* transition = &machine->transitions[machine->transition_count - 1];

        // read state
        start = source;
        while (*source != ' ') {
            source++;
        }
        transition->state = strndup(start, source - start);

        source++; // skip space

        // read value
        transition->value = *(source++);

        source++; // skip space

        // read new value
        transition->new_value = *(source++);

        source++; // skip space

        // read direction
        if (strncmp(source, "<-", 2) == 0) {
            transition->direction = LEFT;
            source += 2;
        } else if (strncmp(source, "->", 2) == 0) {
            transition->direction = RIGHT;
            source += 2;
        } else {
            free_machine(machine);
            return NULL;
        }

        source++; // skip space

        // read new state
        start = source;
        while (*source != '\n' && *source != '\0') {
            source++;
        }
        transition->new_state = strndup(start, source - start);

        source++; // skip newline
    }

    return machine;
}

void run_machine(Machine* machine, Tape* tape) {
    char* state = machine->initial_state;
    size_t head = 0;

    while (1) {
        // find transition
        Transition* transition = NULL;
        for (size_t i = 0; i < machine->transition_count; i++) {
            Transition* t = &machine->transitions[i];
            if (strcmp(t->state, state) == 0 && t->value == tape->values[head]) {
                transition = t;
                break;
            }
        }

        if (transition == NULL) {
            printf("info: transition '%s' not found, halting\n", state);
            break;
        }

        // update state
        state = transition->new_state;

        // write to tape
        tape->values[head] = transition->new_value;

        // move head
        if (transition->direction == LEFT) {
            if (head == 0) {
                fprintf(stderr, "error: head moved out of bounds\n");
                break;
            }

            head--;
        } else {
            head++;
        }

        if (head >= tape->capacity) {
            tape->capacity *= 2;
            tape->values = realloc(tape->values, tape->capacity * sizeof(Value));
            memset(tape->values + head, machine->default_value, tape->capacity - head);
        }
    }
}

Tape* create_tape(Machine* machine) {
    Tape* tape = calloc(sizeof(Tape), 1);

    tape->capacity = 1;
    tape->values = malloc(sizeof(Value) * tape->capacity);
    memset(tape->values, machine->default_value, tape->capacity);
    
    return tape;
}

void free_tape(Tape* tape) {
    (void)tape;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <machine.turc> <input.txt>\n", argv[0]);
        return 1;
    }

    // skip program name
    argv++;
    argc--;

    // read machine path
    char* source = read_file(argv[0]);
    if (source == NULL) {
        fprintf(stderr, "error: failed to read file '%s'\n", argv[0]);
        return 1;
    }
    argv++;
    argc--;

    Machine* machine = parse_machine(source);
    free(source);
    if (machine == NULL) {
        return 1;
    }

    Tape* tape = create_tape(machine);
    if (tape == NULL) {
        free_machine(machine);
        return 1;
    }

    // read input
    source = read_file(argv[0]);
    if (source == NULL) {
        fprintf(stderr, "error: failed to read file '%s'\n", argv[0]);
        free_tape(tape);
        free_machine(machine);
        return 1;
    }

    tape->values = realloc(tape->values, strlen(source) * sizeof(Value));
    tape->capacity = strlen(source);
    for (size_t i = 0; i < strlen(source); i++) {
        tape->values[i] = source[i];
    }

    run_machine(machine, tape);

    printf("tape: ");
    for (size_t i = 0; i < tape->capacity; i++) {
        printf("%c ", tape->values[i]);
    }
    printf("\n");

    free_tape(tape);
    free_machine(machine);
    return 0;
}
