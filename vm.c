#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "vec.h"
#define CARR_SV_IMPLEMENTATION
#include "sv.h"
#define CARR_TERM_IMPLEMENTATION
#include "term.h"

#define FPS 1.0f

#define STACK_CAPACITY 10

static bool RUN = true;

typedef enum {
    POP,
    PUSH,
    DUPE,
    SWAP,
    ROT,
    ADD,
    SUB,
    MULT,
    DIV,
    SHIFTL,
    SHIFTR,
    PRINTS,
    PRINTC,
    PRINT,
    JMPZ,
    JMPL,
    JMPG,
    HALT,
} OpCode;

typedef struct {
    OpCode code;
    int    operand;
} Op;

typedef struct {
    Op*    items;
    size_t len;
    size_t cap;
} Ops;

typedef struct {
    int ip;
    int sp;
    int stack[STACK_CAPACITY];
} VM;


void handle_sigint(int _)
{
    (void)_;
    RUN = false;
}

#define VM_BINOP(vm, op)   \
    int a = vm_pop((vm));  \
    int b = vm_pop((vm));  \
    int c = a op b;        \
    vm_push(vm, c);        \


VM vm_new()
{
    return (VM) {
        .ip    = 0,
        .sp    = 0,
        .stack = {0},
    };
}

int vm_pop(VM* vm)
{
    if (vm->sp == 0) {
        printf("ERROR: stack underflow\n");
        exit(1);
    }
    return vm->stack[--vm->sp];
}

void vm_push(VM* vm, int item)
{
    if (vm->sp == STACK_CAPACITY - 1) {
        printf("ERROR: stack overflow\n");
        exit(1);
    }
    vm->stack[vm->sp++] = item;
}

OpCode op_code_from_sv(StringView op_str)
{
    if (0 == strncmp("PUSH",    op_str.data, op_str.len)) { return PUSH;    }
    if (0 == strncmp("POP",     op_str.data, op_str.len)) { return POP;     }
    if (0 == strncmp("DUPE",    op_str.data, op_str.len)) { return DUPE;    }
    if (0 == strncmp("SWAP",    op_str.data, op_str.len)) { return SWAP;    }
    if (0 == strncmp("ROT",     op_str.data, op_str.len)) { return ROT;    }
    if (0 == strncmp("SHIFTL",  op_str.data, op_str.len)) { return SHIFTL;  }
    if (0 == strncmp("SHIFTR",  op_str.data, op_str.len)) { return SHIFTR;  }
    if (0 == strncmp("ADD",     op_str.data, op_str.len)) { return ADD;     }
    if (0 == strncmp("SUB",     op_str.data, op_str.len)) { return SUB;     }
    if (0 == strncmp("MULT",    op_str.data, op_str.len)) { return MULT;    }
    if (0 == strncmp("DIV",     op_str.data, op_str.len)) { return DIV;     }
    if (0 == strncmp("PRINT",   op_str.data, op_str.len)) { return PRINT;   }
    if (0 == strncmp("PRINTS",  op_str.data, op_str.len)) { return PRINTS;  }
    if (0 == strncmp("PRINTC",  op_str.data, op_str.len)) { return PRINTC;  }
    if (0 == strncmp("JMPZ",    op_str.data, op_str.len)) { return JMPZ;   }
    if (0 == strncmp("JMPL",    op_str.data, op_str.len)) { return JMPL;   }
    if (0 == strncmp("JMPG",    op_str.data, op_str.len)) { return JMPG;   }
    if (0 == strncmp("HALT",    op_str.data, op_str.len)) { return HALT;   }

    printf("ERROR: invalid op_str: %.*s\n", (int)op_str.len, op_str.data);
    exit(1);
    return 0;
}

void op_print(Op inst)
{
    switch (inst.code) {
        case PUSH:     printf("PUSH %d", inst.operand);  break;
        case DUPE:     printf("DUPE %d", inst.operand);  break;
        case SWAP:     printf("SWAP");   break;
        case ROT:      printf("ROT");    break;
        case POP:      printf("POP");    break;
        case SHIFTL:   printf("SHIFTL"); break;
        case SHIFTR:   printf("SHIFTR"); break;
        case ADD:      printf("ADD");    break;
        case SUB:      printf("SUB");    break;
        case MULT:     printf("MULT");   break;
        case DIV:      printf("DIV");    break;
        case PRINTS:   printf("PRINTS");  break;
        case PRINTC:   printf("PRINTC");  break;
        case PRINT:    printf("PRINT");  break;
        case JMPZ:     printf("JMPZ %d", inst.operand);  break;
        case JMPL:     printf("JMPL %d", inst.operand);  break;
        case JMPG:     printf("JMPG %d", inst.operand);  break;
        case HALT:     printf("HALT %d", inst.operand);  break;
    }
}

void vm_update(VM* vm, Ops ops)
{
    if (vm->ip >= ops.len) return;
    Op inst = vec_at(&ops, vm->ip);
    vm->ip++;
    switch (inst.code) {
        case POP: {
            vm_pop(vm);
        } break;
        case PUSH: {
            vm_push(vm, inst.operand);
        } break;
        case DUPE: {
            if (inst.operand >= vm->sp || inst.operand < 0) {
                printf("ERROR: stack access out of bound\n");
                exit(1);
            }
            vm_push(vm, vm->stack[inst.operand]);
        } break;
        case SWAP: {
            int a = vm_pop(vm);
            int b = vm_pop(vm);
            vm_push(vm, a);
            vm_push(vm, b);
        } break;
        case ROT: {
            int a = vm_pop(vm);
            int b = vm_pop(vm);
            int c = vm_pop(vm);
            vm_push(vm, a);
            vm_push(vm, c);
            vm_push(vm, b);
        } break;
        case ADD:  {
            VM_BINOP(vm, +);
        } break;
        case SUB:  {
            VM_BINOP(vm, -);
        } break;
        case MULT:  {
            VM_BINOP(vm, *);
        } break;
        case DIV:  {
            VM_BINOP(vm, /);
        } break;
        case SHIFTL:  {
            VM_BINOP(vm, <<);
        } break;
        case SHIFTR:  {
            VM_BINOP(vm, >>);
        } break;
        case PRINTC: {
            printf("%c", vm_pop(vm));
        } break;
        case PRINTS: {
            char ch = vm_pop(vm);
            while (ch != '\0') {
                printf("%c", ch);
                ch = vm_pop(vm);
            }
        } break;
        case PRINT: {
            printf("%d", vm_pop(vm));
        } break;
        case HALT: {
            RUN = false;
        } break;
        case JMPZ: {
            int a = vm_pop(vm);
            if (a == 0) {
                if (inst.operand < 0 || inst.operand >= ops.len) {
                    printf("ERROR: invalid JMPZ operand: %d\n", inst.operand);
                    exit(1);
                }
                vm->ip = inst.operand;
            }
        } break;
        case JMPL: {
            int a = vm_pop(vm);
            if (a < 0) {
                if (inst.operand < 0 || inst.operand >= ops.len) {
                    printf("ERROR: invalid JMPZ operand: %d\n", inst.operand);
                    exit(1);
                }
                vm->ip = inst.operand;
            }
        } break;
        case JMPG: {
            int a = vm_pop(vm);
            if (a > 0) {
                if (inst.operand < 0 || inst.operand >= ops.len) {
                    printf("ERROR: invalid JMPZ operand: %d\n", inst.operand);
                    exit(1);
                }
                vm->ip = inst.operand;
            }
        } break;
    }
}

#define PADDING 3
#define OP_HEIGHT 2

void vm_draw(VM* vm, Ops ops)
{
    // borders
    term_rectangle(0, 0, TERM_COLS - 1, TERM_ROWS - 1);

    // instructions
    for (int i = 0; i < ops.len; ++i) {
        int col = 0 + PADDING;
        int row = 0 + PADDING + i * OP_HEIGHT;
        term_rectangle(col, row, TERM_COLS / 2 - (PADDING*2), OP_HEIGHT);
        term_move(col + (TERM_COLS / 2 - (PADDING*2)) / 2, row + OP_HEIGHT / 2);
        op_print(vec_at(&ops, i));
        if (i == vm->ip) {
            term_move(col + TERM_COLS / 2 - (PADDING*2), row + OP_HEIGHT / 2 );
            printf("<");
        }
    }

    // stack
    int stack_left = TERM_COLS / 2 + PADDING;
    int stack_top  = 0 + PADDING;
    int stack_height = TERM_ROWS - PADDING * 2;
    int stack_width  = TERM_COLS / 2 - 2* PADDING;
    term_rectangle(stack_left, stack_top, stack_width, stack_height);
    int stack_item_height = 2;
    for (int i = 0; i < vm->sp; ++i) {
        int col = stack_left + PADDING;
        int row = stack_top + stack_height - (stack_item_height * (i + 1));
        int w = stack_width - 2*PADDING;
        term_rectangle(col, row, w, stack_item_height);
        term_move(col + w / 2, row + stack_item_height / 2);
        printf("%d", vm->stack[i]);
        term_move(5, TERM_ROWS);
        printf(" ~ ");
    }

}


Ops parse_code(const char* file_path)
{
    Ops ops;
    vec_init(&ops);
    StringBuilder buf = sb_from_file(file_path);
    StringView file = sv_from_sb(buf);

file_loop:
    while (file.len > 0) {
        StringView line = sv_chop_line(&file);
        size_t counter = 0;
        Op op;
        while (line.len > 0) {
            StringView part = sv_chop_by_space(&line);
            sv_strip_space(&part);  

            if (sv_starts_with(part, "//") || part.len == 0) {
                goto file_loop;
            }

            if (counter == 0) {
                op.code = op_code_from_sv(part);
            } else if (counter == 1) {
                op.operand = sv_parse_int(part);
            } else {
                printf("ERROR: invalid number of operands\n");
                exit(1);
            }
            counter++;
        }
        vec_append(&ops, op);
    }

    sb_free(&buf);
    return ops;
}

int main(void)
{
    signal(SIGINT, handle_sigint);

    Ops instructions = parse_code("code.cvm");

    term_init();
    VM vm = vm_new();

    while (RUN) {
        term_clear();

        vm_draw(&vm, instructions);
        // printf("IP: %d\n", vm.ip);
        vm_update(&vm, instructions);

        fflush(stdout);
        usleep(1000000 / FPS);
    }

    term_cleanup();
    vec_free(&instructions);


    return 0;
}
