#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TK_RESERVED, // 記号
    TK_NUM, // 整数
    TK_EOF, // 入力終わり
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
    TokenKind kind;
    Token *next;
    int val; // kindがTK_NUMの時、その数値
    char *str; // トークン文字列
};

// Input program
char *user_input;

Token *token;

void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, ap, fmt);
    fprintf(stderr, "\n");
    exit(1);
}

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

// 次のトークンが期待されてる記号なら、ひとつ読み進めてtrueを返す。
// それ以外ならfalse
bool consume(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op) return false;
    token = token->next;
    return true;
}

// 次のトークンが期待されてる記号なら、トークンを一つ進める。
// それ以外ならエラー
void expect(char op) {
    if (token->kind != TK_RESERVED || token->str[0] != op) error_at(token->str, "expected '%c'", op);
    token = token->next;
}

// 次が数値なら、トークンをひとつ進めてその数値を返す。
// それ以外ならエラー
int expect_number() {
    if (token->kind != TK_NUM) error_at(token->str, "expected a number");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    cur->next = tok;
    return tok;
}

Token *tokenize() {
    char *p = user_input;
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')') {
            cur = new_token(TK_RESERVED, cur, p++);
            continue;
        } else if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p);
            cur->val = strtol(p, &p, 10);
        } else {
            error_at(p, "unexpected char while tokenization");
        }
    }

    new_token(TK_EOF, cur, p);
    return head.next;
}

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_NUM, // integer
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの型
    Node *lhs; // 左辺
    Node *rhs; // 右辺
    int val; // kindがND_NUMの場合のみ使う
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
Node *primary();
Node *unary();

Node *expr() {
    Node *node = mul();

    for (;;) {
        if (consume('+')) node = new_node(ND_ADD, node, mul());
        else if (consume('-')) node = new_node(ND_SUB, node, mul());
        else return node;
    }
}

Node *mul() {
    Node *node = unary();

    for (;;) {
        if (consume('*')) node = new_node(ND_MUL, node, unary());
        else if (consume('/')) node = new_node(ND_DIV, node, unary());
        else return node;
    }
}

Node *unary() {
    if (consume('+')) return primary();
    if (consume('-')) return new_node(ND_SUB, new_node_num(0), primary());
    return primary();
}

Node *primary() {
    if (consume('(')) {
        Node *node = expr();
        expect(')');
        return node;
    }

    return new_node_num(expect_number());
}

// 引数のnode以降の（つまり、node->next以降を含む）計算結果をスタックの最下段に配置するようなアセンブリを出力する
void gen(Node *node) {
    if (node->kind == ND_NUM) {
        printf("     push %d\n", node->val);
        return;
    }

    gen(node->lhs);
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
    case ND_DIV:
        printf("    cqo\n");
        printf("    idiv rdi\n");
        break;
    }

    printf("    push rax\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
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
