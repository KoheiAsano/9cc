#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum{
  TK_NUM = 256, // Number
  TK_EOF, // End Of File
};

typedef struct {
  int ty; // Type of Token
  int val; // value when ty == TK_NUM
  char *input; // Token string
} Token;
//Token sequence limited 100 tokens
Token tokens[100];

// split string pointed by p, save on tokens
void tokenize(char *p){
  int i=0;
  while(*p){
    //skip space
    if(isspace(*p)){
      p++;
      continue;
    }
    if(*p == '+' || *p == '-'){
      tokens[i].ty = *p;
      tokens[i].input = p;
      i++;
      p++;
      continue;
    }
    if(isdigit(*p)){
      tokens[i].ty = TK_NUM;
      tokens[i].input = p;
      tokens[i].val = strtol(p,&p,10);
      i++;
      continue;
    }

    fprintf(stderr, "can't tokenize: %s\n", p);
    exit(1);
  }
  tokens[i].ty = TK_EOF;
  tokens[i].input = p;
}

// func to report error
void error(int i){
  fprintf(stderr, "unknown string: '%s'\n",tokens[i].input);
  exit(1);
}
int main(int argc, char **argv){
  if (argc != 2){
    fprintf(stderr, "wrong argument's number\n");
    return 1;
  }
  tokenize(argv[1]);
  //output initial part of asm
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  //check if first token is number
  if (tokens[0].ty != TK_NUM)error(0);
  printf("  mov rax, %d\n", tokens[0].val);

  // process `+ <NUM>` or `- <NUM>`, print asm
  int i = 1;
  while(tokens[i].ty != TK_EOF){
    if(tokens[i].ty == '+'){
      i++;
      if(tokens[i].ty != TK_NUM)error(i);
      printf("  add rax, %d\n", tokens[i].val);
      i++;
      continue;
    }
    if(tokens[i].ty == '-'){
      i++;
      if(tokens[i].ty != TK_NUM)error(i);
      printf("  sub rax, %d\n", tokens[i].val);
      i++;
      continue;
    }
    error(i);
  }
  printf("  ret\n");
  return 0;
}
