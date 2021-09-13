#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

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
char *user_input; // 入力文字列

// 可変長引数を持つ関数
// printfと同じ引数をとる
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// エラーの場所を報告する
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " "); // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

/* 次のトークンが期待している記号の時は、トークンを1つ進めて
   真を返す。そうでなければ偽 */
bool consume(char *op) {
  if (token->kind != TK_RESERVED ||
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

// 次のトークンが期待している記号の時は、トークンを1つ進める
// それ以外はエラー報告
void expect(char *op) {
  if (token->kind != TK_RESERVED ||
      strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    error_at(token->str, "'%s'ではありません", op);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ進めてその数値を返す
// それ以外はエラー報告
int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいトークンを作成して cur につなげる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

bool strequal(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

// 入力文字列 p をトークナイズして返す
Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while(*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }
    if (strequal(p, "==") || strequal(p, "!=") ||
        strequal(p, "<=") || strequal(p, ">=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }
    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' ||
        *p == '(' || *p == ')' || *p == '<' || *p == '>') {
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
    error_at(p, "無効なトークンです");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}

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

// 左辺と右辺を受け取る2項演算子を表すノードの作成
Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 数値を表すノードの作成
Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

/* 再帰下降構文解析法でパーサを書く時の基本戦略は
 * 非終端記号をそれぞれ関数にマップすること
 * (生成規則がそのまま関数に対応する) */

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// expr = equality
Node *expr() {
  return equality();
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQUAL, node, relational());
    else if (consume("!="))
      node = new_node(ND_NOT_EQUAL, node, relational());
    else
      return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_node(ND_LESS_THAN, node, add());
    else if (consume("<="))
      node = new_node(ND_OR_LESS, node, add());
    else if (consume(">"))
      // 左辺と右辺を入れ替えて、< にしている
      node = new_node(ND_LESS_THAN, add(), node);
    else if (consume(">="))
      // 左辺と右辺を入れ替えて、<= にしている
      node = new_node(ND_OR_LESS, add(), node);
    else
      return node;
  }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

// unary = ("+" | "-")? primary
Node *unary() {
  if (consume("+"))
    return primary();
  if (consume("-"))
    return new_node(ND_SUB, new_node_num(0), primary());
  return primary();
}

// primary = "(" expr ")" | num
Node *primary() {
  // 次のトークンが "("なら、"(" expr ")" のはず
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }
  // そうでなければ数値のはず
  return new_node_num(expect_number());
}

// 抽象構文木を x86-64 におけるスタックマシンのコードに変換
void gen(Node *node) {
  if (node->kind == ND_NUM) {
    printf("\tpush %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("\tpop rdi\n");
  printf("\tpop rax\n");

  switch (node->kind) {
    case ND_ADD:
      printf("\tadd rax, rdi\n");
      break;
    case ND_SUB:
      printf("\tsub rax, rdi\n");
      break;
    case ND_MUL:
      printf("\timul rax, rdi\n");
      break;
    case ND_DIV:
      printf("\tcqo\n");
      printf("\tidiv rdi\n");
      break;
    case ND_EQUAL:
      printf("\tcmp rax, rdi\n");
      printf("\tsete al\n");
      printf("\tmovzb rax, al\n");
      break;
    case ND_NOT_EQUAL:
      printf("\tcmp rax, rdi\n");
      printf("\tsetne al\n");
      printf("\tmovzb rax, al\n");
      break;
    case ND_LESS_THAN:
      printf("\tcmp rax, rdi\n");
      printf("\tsetl al\n");
      printf("\tmovzb rax, al\n");
      break;
    case ND_OR_LESS:
      printf("\tcmp rax, rdi\n");
      printf("\tsetle al\n");
      printf("\tmovzb rax, al\n");
      break;
  }

  printf("\tpush rax\n");
}

int main(int argc, char **argv) {
  if (argc != 2)
    error("引数の個数が正しくありません");

  // トークナイズ & パース
  user_input = argv[1];
  token = tokenize(user_input);
  Node *node = expr();

  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");

  // 抽象構文木を下りながらアセンブリ生成
  gen(node);

  /* スタックトップに式を計算した値があるので
   * それを RAX にロードして関数からの返り値とする */
  printf("\tpop rax\n");
  printf("\tret\n");
  return 0;
}