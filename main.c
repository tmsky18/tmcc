#include "tmcc.h"

// 可変長引数を持つ関数
// printfと同じ引数をとる
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
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