#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

char *user_input; // 入力文字列

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_NUM,
  TK_EOF,
} TokenKind;

typedef struct Token Token;

struct Token {
  TokenKind kind; // トークンの型
  Token *next;
  int val;        // kind が TK_NUM の場合のその数値
  char *str;
  int len;
};

Token *token; // パーサが読み込むトークン列 (連結リスト)

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_NUM,
  ND_EQUAL,
  ND_NOT_EQUAL,
  ND_LESS_THAN,
  ND_OR_LESS,
} NodeKind;

typedef struct Node Node;

struct Node {
  NodeKind kind; // ノードの型
  Node *lhs;     // 左辺 (left-hand side)
  Node *rhs;     // 右辺
  int val;       // kindが ND_NUMの場合のみ使う
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
Token *tokenize(char *p);
Node *expr();
void gen(Node *node);