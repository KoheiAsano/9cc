#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




enum{
  TK_NUM = 256, // Number
  TK_IDENT,
  TK_EOF, // End Of File
};

typedef struct {
  int ty; // Type of Token
  int val; // value when ty == TK_NUM
  char name; //value when ty == TK_IDENT
  char *input;
} Token;

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *tokens;
int pos = 0;
// func to report error
void error(int i){
  fprintf(stderr, "unknown string: '%s'\n",((Token *)tokens->data[i])->input);
  exit(1);
}
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
    if(*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == '=' || *p == ';'){
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
    if('a' <= *p && *p <= 'z'){
      Token *t = add_token(v,TK_IDENT,p);
      t->name = *p;
      i++;
      p++;
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
  ND_IDENT,
};

typedef struct Node {
  int ty;//type
  struct Node *lhs; //Left Hand Side
  struct Node *rhs; // Right Hand Side
  int val; // used when ty == ND_NUM
  char name; // used when ty == ND_IDENT
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

Node *new_node_id(char name){
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->name = name;
  return node;
}


int consume(int ty){
  if (((Token *)tokens->data[pos])->ty != ty)
    return 0;
  pos++;
  return 1;
}
Node *stmt();
Node *assign();
Node *add();
Node *mul();
Node *term();

Node *code[100];
void program() {
  int i=0;
  while(((Token *)tokens->data[pos])->ty != TK_EOF){
    code[i++] = stmt();
  }
  code[i] = NULL;
}

Node *stmt() {
  Node *node = assign();
  if (!consume(';'))error(((Token *)tokens->data[pos])->input);
  return node;
}

Node *assign(){
  Node *node = add();
  for(;;){
    if(consume('='))
      node = new_node('=', node, assign());
    else
      return node;
  }
}

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
  if(t->ty == TK_IDENT){
    return new_node_id(((Token *)tokens->data[pos++])->name);
  }

  fprintf(stderr,"unknown token : %s", t->input);
}

void gen_lval(Node *node){
  if(node->ty != ND_IDENT)error(node->val);

  int offset = ('z' - node->name + 1) * 8;
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n",offset);
  printf("  push rax\n");
}



void gen(Node *node){
  if(node->ty == ND_NUM){
    printf("  push %d\n", node->val);
    return;
  }
  if(node->ty == ND_IDENT){
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }
  if(node->ty == '='){
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
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





int main(int argc, char **argv){
  if (argc != 2){
    fprintf(stderr, "wrong argument's number\n");
    return 1;
  }

  //tokenize and parse
  // result will be kept in code
  tokens = tokenize(argv[1]);
  program();

  //output initial part of asm
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  //prologue: reserve 26 variants' space
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, 208\n");
  // generate code descending AST
  for (int i=0;code[i];i++){
    gen(code[i]);
    //stacktop is result of equation
    //pop it not to be stackoverflow
    printf("  pop rax\n");
  }

  //epilogue
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
  return 0;
}
