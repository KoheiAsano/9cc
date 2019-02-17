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
  char *input;
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
    if(*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')'){
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


enum{
  ND_NUM = 256,
};

typedef struct Node {
  int ty;
  struct Node *lhs;
  struct Node *rhs;
  int val;
} Node;

Node *new_node(int ty, Node *lhs, Node *rhs){
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val){
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}
int pos = 0;

int consume(int ty){
  if (tokens[pos].ty != ty)
    return 0;
  pos++;
  return 1;
}
Node *add();
Node *mul();
Node *term();

Node *add(){
  Node *node = mul();

  for(;;){
    if(consume('+'))
      node = new_node('+', node, mul());
    else if (consume('-'))
      node = new_node('-', node, mul());
    else
      return node;
  }
}

Node *mul(){
  Node *node = term();

  for(;;){
    if(consume('*'))
      node = new_node('*', node, term());
    else if(consume('/'))
      node = new_node('/', node, term());
    else
      return node;
  }
}

Node *term(){
  if (consume('(')){
    Node *node = add();
    if(!consume(')'))
      fprintf(stderr,"close parenthesis is not found: %s", tokens[pos].input);
    return node;
  }
  if(tokens[pos].ty == TK_NUM)
    return new_node_num(tokens[pos++].val);

  fprintf(stderr,"unknown token : %s", tokens[pos].input);
}


void gen(Node *node){
  if(node->ty == ND_NUM){
    printf("  push %d\n", node->val);
    return;
  }
  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->ty) {
  case '+':
    printf("  add rax, rdi\n");
    break;
  case '-':
    printf("  sub rax, rdi\n");
    break;
  case '*':
    printf("  mul rdi\n");
    break;
  case '/':
    printf("  mov rdx, 0\n");
    printf("  div rdi\n");
  }
  printf("  push rax\n");
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
  Node *node = add();

  //output initial part of asm
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  // generate code descending AST
  gen(node);
  //take value from stacktop
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
