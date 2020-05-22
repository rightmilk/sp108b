#include <assert.h>
#include "compiler.h"

int  E();
void STMT();
void BLOCK();

int tempIdx = 1, labelIdx = 1, gotolabelIdx = 1;
GOTOLABEL gotoLabel[GOTOLABEL_MAX]; // 存放goto標籤

#define nextTemp() (tempIdx++)
#define nextLabel() (labelIdx++)

int findGotoLabel(char *label) { // 尋找goto標籤
  for(int i=1; i<gotolabelIdx; i++ )
  {
    if(strcmp(gotoLabel[i].label, label) == 0)
    {
      return gotoLabel[i].index;
    }
  }
  return -1;
}

int isNext(char *set) {
  char eset[SMAX], etoken[SMAX];
  sprintf(eset, " %s ", set);
  sprintf(etoken, " %s ", tokens[tokenIdx]);
  return (tokenIdx < tokenTop && strstr(eset, etoken) != NULL);
}

int isNext2(char *set) { //尋找接下來出現的第二個字
  char eset[SMAX], etoken[SMAX];
  sprintf(eset, " %s ", set);
  sprintf(etoken, " %s ", tokens[tokenIdx+1]);
  return (tokenIdx+1 < tokenTop && strstr(eset, etoken) != NULL);
}

int isNextType(TokenType type) {
  return (types[tokenIdx] == type);
}

int isEnd() {
  return tokenIdx >= tokenTop;
}

char *next() {
  // printf("%02d:token = %-10s type = %-10s\n", tokenIdx, tokens[tokenIdx], typeName[types[tokenIdx]]);
  return tokens[tokenIdx++];
}

char *skip(char *set) {
  if (isNext(set)) {
    return next();
  } else {
    error("skip(%s) got %s fail!\n", set, next());
  }
}

char *skipType(TokenType type) {
  if (isNextType(type)) {
    return next();
  } else {
    error("skipType(%s) got %s fail!\n", typeName[type], typeName[types[tokenIdx]]);
  }
}

// CALL(id) = (E*)
int CALL(char *id) {
  assert(isNext("("));
  skip("(");
  int e[100], ei = 0;
  while (!isNext(")")) {
    e[ei++] = E();
    if (!isNext(")")) skip(",");
  }
  for (int i=0; i<ei; i++) {
    irEmitArg(e[i]);
  }
  skip(")");
  irEmitCall(id, ei);
}

// F = (E) | Number | Id | CALL
int F() {
  int f;
  if (isNext("(")) { // '(' E ')'
    next(); // (
    f = E();
    next(); // )
  } else { // Number | Id | CALL
    f = nextTemp();
    char *item = next();
    irEmitAssignTs(f, item);
    // emit("t%d = %s\n", f, item);
  }
  return f;
}

// E = F (op E)*
int E() {
  int i1 = F();
  while (isNext("+ - * / & | < > = <= >= == != && ||")) {
    char *op = next();
    int i2 = E();
    int i = nextTemp();
    irEmitOp2(i, i1, op, i2);
    // emit("t%d = t%d %s t%d\n", i, i1, op, i2);
    i1 = i;
  }
  return i1;
}

int EXP() {
  tempIdx = 1; // 讓 temp 重新開始，才不會 temp 太多！
  return E();
}

// ASSIGN = id '=' E
void ASSIGN(char *id) {
  // char *id = next();
  skip("=");
  int e = EXP();
  irEmitAssignSt(id, e);
  // emit("%s = t%d\n", id, e);
}

// while (E) STMT
void WHILE() {
  int whileBegin = nextLabel();
  int whileEnd = nextLabel();
  irEmitLabel(whileBegin);
  // emit("(L%d)\n", whileBegin);
  skip("while");
  skip("(");
  int e = E();
  irEmitIfNotGoto(e, whileEnd);
  // emit("goif T%d L%d\n", whileEnd, e);
  skip(")");
  STMT();
  irEmitGoto(whileBegin);
  // emit("goto L%d\n", whileBegin);
  irEmitLabel(whileEnd);
  // emit("(L%d)\n", whileEnd);
}

// IF = if (E) STMT (else STMT)?
void IF() {
  int ifEnd = nextLabel();
  int elseEnd = nextLabel();
  skip("if");
  skip("(");
  int e = E();
  irEmitIfNotGoto(e, ifEnd);
  // emit("if not T%d goto L%d\n", e, ifEnd);
  skip(")");
  STMT();
  irEmitGoto(elseEnd);
  // emit("goto L%d\n", elseEnd);
  irEmitLabel(ifEnd);
  // emit("(L%d)\n", ifEnd);
  if (isNext("else")) {
    skip("else");
    STMT();
  }
  irEmitLabel(elseEnd);
  // emit("(L%d)\n", elseEnd);
}

void FOR() {
  int forBegin = nextLabel();
  int forEnd = nextLabel();
  skip("for");
  skip("(");
  STMT();
  irEmitLabel(forBegin);
  int e = E();
  irEmitIfNotGoto(e, forEnd);
  skip(";");
  char *id = next();
  ASSIGN(id);
  skip(")");
  STMT();
  irEmitGoto(forBegin);
  irEmitLabel(forEnd);
}

int newGotoLabel(char *label) { // 建立新標籤
  int index = nextLabel();
  gotoLabel[gotolabelIdx].index = index;
  gotoLabel[gotolabelIdx].label = label;
  gotolabelIdx++;
  return index;
}

void GOTO() { // 跳到標籤位置
  skip("goto");
  char *label=next();
  
  int index = findGotoLabel(label);
  if (index == -1)
    index = newGotoLabel(label);
  irEmitGoto(index);
  skip(";");
}

void GotoLabel() { // 擺放跳躍標籤
  char *label=next();
  int index = findGotoLabel(label);
  if (index == -1)
    index = newGotoLabel(label);
  irEmitLabel(index);
  skip(":");
}

void STMT() {
  if (isNext("while"))
    WHILE();
  else if (isNext("if"))
    IF();
  else if (isNext("for"))
    FOR();
  else if (isNext("goto"))
    GOTO();
  else if (isNext2(":")) // 暫時不考慮switch case
    GotoLabel();
  else if (isNext("{"))
    BLOCK();
  else {
    char *id = next();
    /*
    if (eq(id, "int")) {
      skip("int");
      while (!isNext(";")) {
        char *var = skipType(Id);
        mapAdd(symMap, var, &symList[symTop++]);
      }
    }
    */
    if (isNext("(")) {
      CALL(id);
    } else {
      ASSIGN(id);
    }
    skip(";");
  }
}

void STMTS() {
  while (!isEnd() && !isNext("}")) {
    STMT();
  }
}

// { STMT* }
void BLOCK() {
  skip("{");
  STMTS();
  skip("}");
}

void PROG() {
  STMTS();
}

void parse() {
  // printf("============ parse =============\n");
  tokenIdx = 0;
  PROG();
}