#include "9cc.h"

// Put the calculated result of node in top of the stack
void codegen(Node *node) {
    if (node->kind == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }

    // Put the result of another node: node->lhs in top of the stack
    codegen(node->lhs);
    // Put the result of another node: node->rhs in top of the stack
    codegen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            printf("    add rax, rdi\n");
            break;
        case ND_SUB:
            printf("    sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("    imul rax, rdi\n");
            break;
        case ND_EQ:
            printf("    cmp rax, rdi\n");
            printf("    sete al\n"); // move the result of cmp (recorded in flag register) to al register (smallest 8 bits in rax)
            printf("    movzb rax, al\n"); // clear value of rax except for al to zero
            break;
        case ND_NEQ:
            printf("    cmp rax, rdi\n");
            printf("    setne al\n"); // move the result of cmp (recorded in flag register) to al register (smallest 8 bits in rax)
            printf("    movzb rax, al\n"); // clear value of rax except for al to zero
            break;
        case ND_LT:
            printf("    cmp rax, rdi\n");
            printf("    setl al\n"); // move the result of cmp (recorded in flag register) to al register (smallest 8 bits in rax)
            printf("    movzb rax, al\n"); // clear value of rax except for al to zero
            break;
        case ND_LEQ:
            printf("    cmp rax, rdi\n");
            printf("    setle al\n"); // move the result of cmp (recorded in flag register) to al register (smallest 8 bits in rax)
            printf("    movzb rax, al\n"); // clear value of rax except for al to zero
            break;
        case ND_DIV:
            // Set value stored at `rax` to `rdx` + `rax` by extending 64bit to 128bit
            printf("    cqo\n");
            // Divide `rdx` + `rax` by `rdi` and set the quotient to `rax` and remainder to `rdx`
            printf("    idiv rdi\n");
            break;
    }

    printf("    push rax\n");
}