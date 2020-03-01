/*
 * stm: stack machine
 */
#define DEB 0
#define UNIX 0
#define WEB 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if UNIX
#include <signal.h>
#include <unistd.h>
#endif
#define true 1
#define false 0
#define N 128 /* max read line size */
#define STACKSIZE 1000
#define PCODESIZE 1000
#define MAXEXLOOP 1000
char Copyright[] =
 "@(#) 2014 Isamu Shioya.";
int stack[STACKSIZE];
struct spcode {
  int inst;
  int op;
};
struct spcode pcode[PCODESIZE];
int sp = 0; /* stack pointer */
int bp = 0; /* frame or base pointer */
int pca = 0; /* program counter */
int max_pca = 0; /* max p_code */
unsigned char flag = 'E'; /* for comparison */
int err=false;

#define NINST 20
char *instcode[] = {"NOP", "LL", "LA", "L", "A","M","S","D","ST","H","CALL","RT",
                    "CPA","JNZ","JZ","JPZ","JP","JN","JUC", "TRO"};
#define NOP 0
#define LL 1
#define LA 2
#define L 3
#define A 4
#define M 5
#define S 6
#define D 7
#define ST 8
#define H 9
#define CALL 10
#define RT 11
#define CPA 12
#define JNZ 13
#define JZ 14
#define JPZ 15
#define JP 16
#define JN 17
#define JUC 18
#define TRO 19
#if UNIX
#define TOUPPER(CH) \
  (((CH) >= 'a' && (CH) <= 'z') ? ((CH) - 'a' + 'A') : (CH))

int stricmp (const char *s1, const char *s2)
{
  while (*s2 != 0 && TOUPPER (*s1) == TOUPPER (*s2))
    s1++, s2++;
  return (int) (TOUPPER (*s1) - TOUPPER (*s2));
}
#endif
/*
 * is_valid_nnint
 */
int is_valid_nnint(char *str) { /* non negative*/
  if (!*str) return false; /* empty str */
  while (*str) {
    if (!isdigit(*str)) return false;
    else ++str;
  }
  return true;
}
/*
 * is_valid_int
 */
int is_valid_int(char *str) { /* allow "-" */
  if (!*str) return false; /* empty str */
  if (*str == '-') ++str;
  return is_valid_nnint(str);
}
/*
 * init
 */
void init(void) {
  int i;
  for(i=0;i<STACKSIZE;i++) {
    stack[i] = 0;
  }
  for(i=0;i<PCODESIZE;i++) {
    pcode[i].inst = H;
    pcode[i].op = 0;
  }
}
/*
 * instf
 */
int instf(char * s) {
  int i;
  for(i=0;i<NINST;i++) {
    if(!stricmp(s,instcode[i])) return i;
  }
  return -1;
}
void disp_pcode(void) {
  int i;
  for(i=0;i<pca;i++) {
    /* if (!pcode[i].inst) continue; */ /* NOP */
    printf("pc: %d %s %d\n",i,instcode[pcode[i].inst],pcode[i].op);
  }
}
void disp_stack(void) {
  int i;
  printf("bp %d, sp %d, pc %d, flag %c.\n",bp,sp,pca,flag);
  for(i=0;i<sp;i++) printf("stack %d: %d\n",i, stack[i]);
}
/*
 * execute
 */
void execute(int exec_add) {
  int c, ct, op;
  bp=sp=0;pca=exec_add;
  printf("exec: sp %d, pc %d.\n",sp,pca);
  for(ct=0;;ct++) {
    c = pcode[pca].inst;
    op = pcode[pca].op;
    printf("--\nex %d, pc: %d, inst: %s, op: %d, sp: %d, bp: %d.\n",ct,pca,instcode[c],op,sp,bp);
    switch(c) {
      case NOP:
        break;
      case H:
        break;
      case LL:
        stack[sp] = op; sp++;
        break;
      case LA:
        stack[sp] = bp + op; sp++;
        break;
      case ST:
        if (stack[sp-2] < sp-2 && stack[sp-2] >= 0) { /* -2 */
          stack[stack[sp-2]] = stack[sp-1]; sp=sp-2;
        }
        else {
          err = true;
          printf("beyond the stack pointer %d(%d) at %d.\n",stack[sp-2], sp-2,pca);
        }
        break;
      case L:
        if (bp+op < sp && bp+op >= 0) {
          stack[sp] = stack[bp+op]; sp++;
        }
        else {
          err = true;
          printf("beyond the stack pointer %d.\n",bp+op);
        }
        break;
      case CALL:
#if DEB
        printf("CALL: %d %d\n",c,op);
#endif
        if(op<max_pca && op >=0) {
          stack[sp++] = pca+1; /* set return address */
          stack[sp] = bp; /* set old base pointer */
          bp=++sp; /* set new base pointer */
          pca = op; /* jump to new address */
          disp_stack();
          continue;
        }
        else err=true;
        break;
      case RT:
        if((bp-op-2<sp && bp-op-2 >=0) && (bp-1<sp && bp-1>=0)) {
          pca = stack[bp-2]; stack[bp-op-2] = stack[sp-1];
          sp = bp-op-1; bp = stack[bp-1];
          disp_stack();
          continue;
        }
        else err=true;
        break;
      case A:
        stack[sp-2] = stack[sp-2]+stack[sp-1]; sp--;
        break;
      case S:
        stack[sp-2] = stack[sp-2]-stack[sp-1]; sp--;
        break;
      case M:
        stack[sp-2] = stack[sp-2]*stack[sp-1]; sp--;
        break;
      case D:
        if (stack[sp-1] == 0) {printf("zero divide at %d\n",pca);err = true;}
        else {stack[sp-2] = stack[sp-2]/stack[sp-1]; sp--;}
        break;
      case CPA:
        if (sp > 0) {
          if (stack[sp-1] > 0) flag = 'P';
          else if (stack[sp-1] == 0) flag = 'Z';
          else flag = 'N';
          sp--;
        }
        else err=true;
        break;
      case JNZ:
        if(op<max_pca && op >=0) {
          if (flag == 'N' || flag == 'Z') {
            pca = op;
            disp_stack();
            continue;
          }
        }
        else err=true;
        break;
      case JZ:
        if(op<max_pca && op >=0) {
          if (flag == 'Z') {
            pca = op;
            disp_stack();
            continue;
          }
        }
        else err=true;
        break;
      case JPZ:
        if(op<max_pca && op >=0) {
          if (flag == 'P' || flag == 'Z') {
            pca = op;
            disp_stack();
            continue;
          }
        }
        else err=true;
        break;
      case JP:
        if(op<max_pca && op >=0) {
          if (flag == 'P') {
            pca = op;
            disp_stack();
            continue;
          }
        }
        else err=true;
        break;
      case JN:
        if(op<max_pca && op >=0) {
          if (flag == 'N') {
            pca = op;
            disp_stack();
            continue;
          }
        }
        else err=true;
        break;
      case JUC: /* jump unconditional */
#        if(op<max_pca && op >=0) {
          pca = op;
          disp_stack();
          continue;
        }
        else err=true;
        break;
      case TRO:
        printf("TRO: %d %d, not implemented.\n",c,op);
        break;
      default:
        err = true;
    }
    if (!err) disp_stack();
    if(c==H || err == true) {
      if (err) printf("err ");
      printf("halt.\n");
      break;
    }
    if(ct >MAXEXLOOP) {
      printf("halt, max loop %d.\n",ct);
      break;
    }
    pca++;
  }
}
/*
 * readopcode
 */
int readopcode(void) {
  char readline[N]; /* readline */
  char pads[N], insts[N], ops[N];
  int instr; int oldpca, i;
  int exec_add = 0;
  while (fgets(readline,N,stdin) != NULL ) {
    /* printf("%s", readline); */
    i = sscanf(readline,"%s %s %s",pads,insts,ops);
    if (i==0 || i==EOF) continue;
    if (pads[0]=='%') {
      if(stricmp(insts,"START")==0 && is_valid_nnint(ops)) {
        exec_add = atoi(ops); /* execution address */
      }
    }
    else if (pca >= PCODESIZE) { err=true;printf("*pc is overflow.\n");}
    else if(is_valid_nnint(pads)) {
      oldpca = pca; /* keep old pca */
      pca = atoi(pads);
      if(oldpca > pca) printf("*warning: Overwrite p-code at %d.\n",pca);
      for(i=oldpca;i<pca;i++) {
        pcode[i].inst = NOP; /* insert NOP automaticaly if it is interleaved */
        pcode[i].op = 0;
      }
      if (pca >= PCODESIZE) {
        err=true;printf("*p-code area is overflow at %d.\n",pca);
      }
      else if((instr = instf(insts)) < 0) {
        err=true;printf("*error inst \"%s\" at %d.\n",insts,pca);
      }
      else if(!is_valid_int(ops)) {
        err=true;printf("*error op \"%s\" at %d.\n",ops,pca);
      }
      else {
        pcode[pca].inst = instr;
        pcode[pca].op = atoi(ops);
        pca++;
      }
    }
    else {
      if((instr = instf(pads)) < 0) {
        err=true;printf("*error inst \"%s\" at %d.\n",pads,pca);
      }
      else if(!is_valid_int(insts)) {
        err=true;printf("*error op \"%s\" at %d.\n",insts,pca);
      }
      else {
        pcode[pca].inst = instr;
        pcode[pca].op = atoi(insts);
        pca++;
      }
    }
    pads[0]=insts[0]=ops[0]='\0';
  }
  disp_pcode();
  max_pca = pca;
  return exec_add;
}
#if WEB
/*
 * timeout
 */
#define TTIME 1
int time = 0;
int timeout() {
  time +=TTIME;
  printf("time out: %d\n",time);
  exit(0);
}
#endif
/*
 * main
 */
void main(void) {
  int i; int exec_add;
#if WEB
  int timeout();
  signal(SIGALRM, (void *)timeout);
  alarm(TTIME);
#endif
  printf("start stack machine v1.1 by Shioya.\n");
  printf("max-stack:%d, max-pc:%d, max-ex:%d.\n",STACKSIZE,PCODESIZE,MAXEXLOOP);
  for(i=0;i<NINST;i++) {
    if(NINST-1 == i) printf("%s\n",instcode[i]);
    else printf("%s ",instcode[i]);
  }
  init();
  exec_add = readopcode();
  if(err) {
    printf("*error in readcode.\n"); exit(1);
  }
  execute(exec_add);
}
