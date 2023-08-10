#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

typedef enum {
    TK_RESERVED,
    TK_NUM,
    TK_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
    TokenKind kind;
    Token *next;
    int val; // represents value of a number when kind is TK_NUM)
    char *str; // string of the entire token
    int len; // length of the token
};

// Input program
char *user_input;

Token *token; // The current active token

// Reports an error location and exit.
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token-> len || memcmp(token->str, op, token->len)) return false;
    token = token->next;
    return true;
}

void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token-> len || memcmp(token->str, op, token->len))
        error_at(token->str, "expected '%c'", op);
    token = token->next;
}

int expect_number() {
    if (token->kind != TK_NUM) error_at(token->str, "expected a number");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

bool startswith(char *p, char *q) {
    return memcmp(p, q, strlen(q)) == 0;
}

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '<' || *p == '>') {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        error_at(p, "expected a number in main tokenizer loop");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_EQ, // ==
    ND_NEQ, // !=
    ND_LT, // <
    ND_LEQ, // <=
    ND_NUM, // integer
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val; // Used only when kind is ND_NUM
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node *expr();
Node *mul();
Node *unary();
Node *primary();
Node *relational();
Node *add();

Node *primary() {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }
    return new_node_num(expect_number());
}

Node *unary() {
    if (consume("+")) return primary();
    if (consume("-")) return new_node(ND_SUB, new_node_num(0), unary());
    return primary();
}

Node *mul() {
    Node *node = unary();

    for(;;) {
        if (consume("*")) node = new_node(ND_MUL, node, unary());
        else if (consume("/")) node = new_node(ND_DIV, node, unary());
        else return node;
    }
}

Node *expr() {
    Node *node = relational();

    for(;;) {
        if (consume("==")) node = new_node(ND_EQ, node, relational());
        else if (consume("!=")) node = new_node(ND_NEQ, node, relational());
        else return node;
    }
}

Node *relational() {
    Node *node = add();

    for(;;) {
        if (consume("<")) node = new_node(ND_LT, node, add());
        else if (consume(">")) node = new_node(ND_LT, add(), node);
        else if (consume("<=")) node = new_node(ND_LEQ, node, add());
        else if (consume(">=")) node = new_node(ND_LEQ, add(), node);
        else return node;
    }
}

Node *add() {
    Node *node = mul();
    for(;;) {
        if (consume("+")) node = new_node(ND_ADD, node, mul());
        else if (consume("-")) node = new_node(ND_SUB, node, mul());
        else return node;
    }
}

// Put the calculated result of node in top of the stack
void gen(Node *node) {
    if (node->kind == ND_NUM) {
        printf("    push %d\n", node->val);
        return;
    }

    // Put the result of another node: node->lhs in top of the stack
    gen(node->lhs);
    // Put the result of another node: node->rhs in top of the stack
    gen(node->rhs);

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

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Arguments count must be 2.");
        return 1;
    }

    user_input = argv[1];
    token = tokenize(user_input);
    Node *node = expr();

    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    gen(node);

    printf("    pop rax\n");
    printf("    ret\n");
    return 0;
}
