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

        if (*p == '+' || *p == '-') {
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

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    user_input = argv[1];
    token = tokenize();

    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    printf("    mov rax, %ld\n", expect_number());
    while (!at_eof()) {
        if (consume('+')) {
            printf("    add rax, %d\n", expect_number());
        } else if (consume('-')) {
            printf("    sub rax, %d\n", expect_number());
        } else {
            error("unexpected token while compiling");
        }
    }

    printf("    ret\n");
    return 0;
}
