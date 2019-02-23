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

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *new_vector() {
  Vector *vec = malloc(sizeof(Vector));
  vec->data = malloc(sizeof(void *) * 16);
  vec->capacity = 16;
  vec->len = 0;
  return vec;
}

void vec_push(Vector *vec, void *elem){
  if(vec->capacity == vec->len){
    vec->capacity *= 2;
    vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
  }
  vec->data[vec->len++] = elem;
}

Vector *tokens;
int pos = 0;

Token *add_token(Vector *v, int ty, char *input){
  Token *t = malloc(sizeof(Token));
  t->ty = ty;
  t->input = input;
  vec_push(v,t);
  return t;
}

// split string pointed by p, save on tokens
Vector *tokenize(char *p){
  Vector *v = new_vector();
  int i=0;
  while(*p){
    //skip space
    if(isspace(*p)){
      p++;
      continue;
    }
    if(*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')'){
      add_token(v,*p,p);
      i++;
      p++;
      continue;
    }
    if(isdigit(*p)){
      Token *t = add_token(v,TK_NUM,p);
      t->val = strtol(p,&p,10);
      i++;
      continue;
    }

    fprintf(stderr, "can't tokenize: %s\n", p);
    exit(1);
  }
  add_token(v,TK_EOF,p);
  return v;
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


int consume(int ty){
  if (((Token *)tokens->data[pos])->ty != ty)
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
  Token *t = tokens->data[pos];
  if (consume('(')){
    Node *node = add();
    if(!consume(')'))
      fprintf(stderr,"close parenthesis is not found: %s", t->input);
    return node;
  }
  if(t->ty == TK_NUM)
    return new_node_num(((Token *)tokens->data[pos++])->val);

  fprintf(stderr,"unknown token : %s", t->input);
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
  fprintf(stderr, "unknown string: '%s'\n",((Token *)tokens->data[i])->input);
  exit(1);
}


int main(int argc, char **argv){
  if (argc != 2){
    fprintf(stderr, "wrong argument's number\n");
    return 1;
  }
  tokens = tokenize(argv[1]);
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
