/*
 * ================================================================
 *  MODULE 6 : MINI COMPILER — FULL INTEGRATION
 *  Integrates Module 1 (Lexer) + Module 2&3 (Parser) +
 *             Module 4 (TAC) + Module 5 (Optimizer) +
 *             Module 6 (Assembly Generator)
 *
 *  Language  : C  (C99)
 *  Compile   : gcc module_6_mini_compiler.c -o mini_compiler
 *  Run       : ./mini_compiler
 * ================================================================
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* ================================================================
   -------- MODULE 1 : LEXER  (unchanged from Module 1) ----------
   ================================================================ */

char *keywords[] = {"int","if","while","return","main","for","print"};
int keywordCount = 7;

int isKeyword(char word[]) {
    int i;
    for(i=0;i<keywordCount;i++)
        if(strcmp(word,keywords[i])==0)
            return 1;
    return 0;
}

int isOperatorChar(char ch) {
    return ch=='+' || ch=='-' || ch=='*' || ch=='/' ||
           ch=='=' || ch=='<' || ch=='>' ||
           ch==';' || ch=='(' || ch==')' ||
           ch=='{' || ch=='}';
}

/* Lexer runs on a string and prints tokens.
   Returns 1 so it can be called inline.            */
void lexer(char code[]) {
    int i=0,j;
    char token[100];

    printf("\n--- LEXER OUTPUT (Module 1) ---\n");

    while(code[i] != '\0') {

        if(isspace(code[i])) { i++; continue; }

        /* Handle // comments */
        if(code[i]=='/' && code[i+1]=='/') {
            while(code[i]!='\n' && code[i]!='\0') i++;
            continue;
        }

        /* Identifier or Keyword */
        if(isalpha(code[i]) || code[i]=='_') {
            j=0;
            while(isalnum(code[i]) || code[i]=='_')
                token[j++]=code[i++];
            token[j]='\0';

            if(isKeyword(token))
                printf("Token: %-15s -> KEYWORD\n",token);
            else
                printf("Token: %-15s -> IDENTIFIER\n",token);
        }

        /* Number (including negative) */
        else if(isdigit(code[i]) ||
               (code[i]=='-' && isdigit(code[i+1]))) {
            j=0;
            if(code[i]=='-') token[j++]=code[i++];
            while(isdigit(code[i])) token[j++]=code[i++];
            token[j]='\0';
            printf("Token: %-15s -> NUMBER\n",token);
        }

        /* Multi-character operators */
        else if((code[i]=='=' && code[i+1]=='=') ||
                (code[i]=='!' && code[i+1]=='=') ||
                (code[i]=='<' && code[i+1]=='=') ||
                (code[i]=='>' && code[i+1]=='=')) {
            printf("Token: %c%c             -> OPERATOR\n",
                   code[i],code[i+1]);
            i+=2;
        }

        /* Single operator */
        else if(isOperatorChar(code[i])) {
            printf("Token: %c              -> OPERATOR\n",code[i]);
            i++;
        }

        else {
            printf("Unknown: %c\n",code[i]);
            i++;
        }
    }
}

/* ================================================================
   -------- MODULE 2 & 3 : PARSER  (unchanged from Module 2&3) ---
   ================================================================ */

char input[200];
int  pos = 0;

int bossFunction(), multiplyFunction(), numberPicker();

int numberPicker() {
    int value;
    if(input[pos] == '(') {
        printf(" Found bracket ...\n");
        pos++;
        value = bossFunction();
        pos++;                   /* consume ')' */
    } else {
        value = input[pos] - '0';
        printf("Picked number: %d\n", value);
        pos++;
    }
    return value;
}

int multiplyFunction() {
    int value = numberPicker();
    while(input[pos] == '*') {
        printf(" Multiplication detected!\n");
        pos++;
        value = value * multiplyFunction();
    }
    return value;
}

int bossFunction() {
    int value = multiplyFunction();
    while(input[pos] == '+') {
        printf(" Addition detected\n");
        pos++;
        value = value + bossFunction();
    }
    return value;
}

/* ================================================================
   -------- MODULE 4 : THREE-ADDRESS CODE GENERATOR --------------
   ================================================================ */

/*
 * Strategy:
 *   We scan the expression string character by character using
 *   a simple precedence-aware recursive approach that mirrors
 *   the bossFunction/multiplyFunction structure of Module 2&3,
 *   but instead of evaluating we EMIT TAC instructions.
 *
 * TAC Instruction format:
 *   tN = operand1 op operand2
 *   var = tN               (copy / assignment)
 *   print var
 */

#define MAX_TAC   200
#define MAX_STR   64

typedef struct {
    char result[MAX_STR];
    char arg1  [MAX_STR];
    char op    [4];
    char arg2  [MAX_STR];
    int  isCopy;    /* 1 => result = arg1  (no op) */
    int  isPrint;   /* 1 => print result           */
    int  dead;      /* 1 => eliminated by DCE      */
} TAC;

TAC  tacList[MAX_TAC];
int  tacCount = 0;
int  tempNum  = 1;   /* next temp index: t1, t2, … */

/* expression position for TAC generation (separate from parser pos) */
char tacExpr[200];
int  tacPos = 0;

static void newTemp(char *buf) {
    sprintf(buf, "t%d", tempNum++);
}

static void emitTAC(const char *res, const char *a1,
                    const char *op, const char *a2,
                    int isCopy, int isPrint) {
    TAC *t = &tacList[tacCount++];
    strncpy(t->result, res, MAX_STR-1);
    strncpy(t->arg1,   a1,  MAX_STR-1);
    strncpy(t->op,     op,  3);
    strncpy(t->arg2,   a2,  MAX_STR-1);
    t->isCopy  = isCopy;
    t->isPrint = isPrint;
    t->dead    = 0;
}

/* Forward declarations for TAC expression generator */
static void tacGenExpr  (char *outVar);
static void tacGenTerm  (char *outVar);
static void tacGenFactor(char *outVar);

static void tacGenFactor(char *outVar) {
    /* skip spaces */
    while(tacExpr[tacPos]==' ') tacPos++;

    if(tacExpr[tacPos]=='(') {
        tacPos++;                       /* consume '(' */
        tacGenExpr(outVar);
        while(tacExpr[tacPos]==' ') tacPos++;
        if(tacExpr[tacPos]==')') tacPos++;  /* consume ')' */
        return;
    }

    /* collect number or identifier */
    int j=0; char buf[MAX_STR];
    if(tacExpr[tacPos]=='-' && isdigit((unsigned char)tacExpr[tacPos+1]))
        buf[j++]=tacExpr[tacPos++];
    while(isalnum((unsigned char)tacExpr[tacPos]) || tacExpr[tacPos]=='_')
        buf[j++]=tacExpr[tacPos++];
    buf[j]='\0';
    strncpy(outVar, buf, MAX_STR-1);
}

static void tacGenTerm(char *outVar) {
    char left[MAX_STR], right[MAX_STR], tmp[MAX_STR];
    tacGenFactor(left);
    while(tacExpr[tacPos]==' ') tacPos++;
    while(tacExpr[tacPos]=='*' || tacExpr[tacPos]=='/') {
        char op[2]; op[0]=tacExpr[tacPos++]; op[1]='\0';
        while(tacExpr[tacPos]==' ') tacPos++;
        tacGenFactor(right);
        newTemp(tmp);
        emitTAC(tmp, left, op, right, 0, 0);
        strncpy(left, tmp, MAX_STR-1);
        while(tacExpr[tacPos]==' ') tacPos++;
    }
    strncpy(outVar, left, MAX_STR-1);
}

static void tacGenExpr(char *outVar) {
    char left[MAX_STR], right[MAX_STR], tmp[MAX_STR];
    tacGenTerm(left);
    while(tacExpr[tacPos]==' ') tacPos++;
    while(tacExpr[tacPos]=='+' || tacExpr[tacPos]=='-') {
        char op[2]; op[0]=tacExpr[tacPos++]; op[1]='\0';
        while(tacExpr[tacPos]==' ') tacPos++;
        tacGenTerm(right);
        newTemp(tmp);
        emitTAC(tmp, left, op, right, 0, 0);
        strncpy(left, tmp, MAX_STR-1);
        while(tacExpr[tacPos]==' ') tacPos++;
    }
    strncpy(outVar, left, MAX_STR-1);
}

/*
 * generateTACForProgram:
 *   Parses a simple program string of the form:
 *     var = expr ; var = expr ; ... print var ;
 *   and emits TAC for each statement.
 */
void generateTACForProgram(char *prog) {
    int i=0, j;
    char varName[MAX_STR], exprBuf[200], resultVar[MAX_STR];

    printf("\n--- THREE ADDRESS CODE (Module 4) ---\n");

    while(prog[i] != '\0') {
        /* skip whitespace */
        while(prog[i]==' '||prog[i]=='\n'||prog[i]=='\t') i++;
        if(prog[i]=='\0') break;

        /* print statement */
        if(strncmp(&prog[i],"print",5)==0 && !isalnum((unsigned char)prog[i+5])) {
            i+=5;
            while(prog[i]==' ') i++;
            j=0;
            while(isalnum((unsigned char)prog[i])||prog[i]=='_')
                varName[j++]=prog[i++];
            varName[j]='\0';
            while(prog[i]!=';'&&prog[i]!='\0') i++;
            if(prog[i]==';') i++;
            emitTAC(varName, varName, "", "", 0, 1);
            continue;
        }

        /* assignment: varName = expr ; */
        j=0;
        while(isalnum((unsigned char)prog[i])||prog[i]=='_')
            varName[j++]=prog[i++];
        varName[j]='\0';
        while(prog[i]==' ') i++;
        if(prog[i]!='=') { i++; continue; }
        i++; /* consume '=' */
        while(prog[i]==' ') i++;

        /* collect expression up to ';' */
        j=0;
        while(prog[i]!=';'&&prog[i]!='\0') exprBuf[j++]=prog[i++];
        exprBuf[j]='\0';
        if(prog[i]==';') i++;

        /* generate TAC for expression */
        strncpy(tacExpr, exprBuf, 199);
        tacPos = 0;
        tacGenExpr(resultVar);

        /* copy result into variable */
        if(strcmp(resultVar, varName)!=0)
            emitTAC(varName, resultVar, "", "", 1, 0);
    }

    /* print raw TAC */
    int k;
    for(k=0; k<tacCount; k++) {
        TAC *t = &tacList[k];
        if(t->isPrint)
            printf("  print %s\n", t->result);
        else if(t->isCopy)
            printf("  %s = %s\n", t->result, t->arg1);
        else
            printf("  %s = %s %s %s\n", t->result, t->arg1, t->op, t->arg2);
    }
}

/* ================================================================
   -------- MODULE 5 : DAG OPTIMIZER  ----------------------------
   ================================================================
   Applies three optimisations in order:
     1. Constant Folding  — evaluate numeric-only ops at compile time
     2. CSE               — reuse already-computed expression temps
     3. Dead Code Elim    — drop temps whose result is never used
*/

#define MAX_CSE 100

char cseKey [MAX_CSE][MAX_STR*3];
char cseTemp[MAX_CSE][MAX_STR];
int  cseCount = 0;

static int isNum(const char *s) {
    int i=0;
    if(!s||!s[0]) return 0;
    if(s[0]=='-') i=1;
    for(;s[i];i++) if(!isdigit((unsigned char)s[i])) return 0;
    return 1;
}

static int evalOp(int a, char op, int b) {
    if(op=='+') return a+b;
    if(op=='-') return a-b;
    if(op=='*') return a*b;
    if(op=='/'&&b!=0) return a/b;
    return 0;
}

void optimize() {
    int i,j;
    printf("\n--- OPTIMIZED CODE (Module 5 - DAG Optimizer) ---\n");

    /* ---- Pass 1 : Constant Folding + CSE ---- */
    for(i=0;i<tacCount;i++) {
        TAC *t = &tacList[i];
        if(t->isPrint || t->isCopy) continue;

        /* Constant Folding */
        if(isNum(t->arg1) && isNum(t->arg2)) {
            int val = evalOp(atoi(t->arg1), t->op[0], atoi(t->arg2));
            sprintf(t->arg1, "%d", val);
            t->arg2[0]='\0'; t->op[0]='\0';
            t->isCopy=1;
            continue;
        }

        /* CSE : build key "arg1 op arg2" */
        char key[MAX_STR*3], keyRev[MAX_STR*3];
        sprintf(key,    "%s%s%s", t->arg1, t->op, t->arg2);
        sprintf(keyRev, "%s%s%s", t->arg2, t->op, t->arg1);

        int found=0;
        for(j=0;j<cseCount;j++) {
            if(strcmp(cseKey[j],key)==0 || strcmp(cseKey[j],keyRev)==0) {
                /* reuse existing temp */
                strncpy(t->arg1, cseTemp[j], MAX_STR-1);
                t->arg2[0]='\0'; t->op[0]='\0';
                t->isCopy=1;
                found=1; break;
            }
        }
        if(!found && cseCount<MAX_CSE) {
            strncpy(cseKey [cseCount], key,      MAX_STR*3-1);
            strncpy(cseTemp[cseCount], t->result, MAX_STR-1);
            cseCount++;
        }
    }

    /* ---- Pass 2 : Dead Code Elimination ---- */
    /* collect all operands that are actually used */
    char used[MAX_TAC*2][MAX_STR];
    int  usedCount=0;
    for(i=0;i<tacCount;i++) {
        TAC *t=&tacList[i];
        if(t->arg1[0]) strncpy(used[usedCount++],t->arg1,MAX_STR-1);
        if(t->arg2[0]) strncpy(used[usedCount++],t->arg2,MAX_STR-1);
    }
    /* mark dead: temp whose result is never referenced */
    for(i=0;i<tacCount;i++) {
        TAC *t=&tacList[i];
        if(t->isPrint) continue;
        /* only consider tN temporaries (not user variables) */
        if(t->result[0]=='t' && isdigit((unsigned char)t->result[1])) {
            int found=0;
            for(j=0;j<usedCount;j++)
                if(strcmp(used[j],t->result)==0){found=1;break;}
            if(!found) t->dead=1;
        }
    }

    /* print optimized TAC */
    for(i=0;i<tacCount;i++) {
        TAC *t=&tacList[i];
        if(t->dead) continue;
        if(t->isPrint)
            printf("  print %s\n", t->result);
        else if(t->isCopy)
            printf("  %s = %s\n", t->result, t->arg1);
        else
            printf("  %s = %s %s %s\n", t->result, t->arg1, t->op, t->arg2);
    }
}

/* ================================================================
   -------- MODULE 6 : ASSEMBLY CODE GENERATOR  ------------------
   ================================================================
   Maps each optimized TAC instruction to x86 NASM-style output.
   Variables/temps ? .bss section (resd 1 = 4 bytes each)
   Arithmetic      ? EAX, EBX registers
   Print           ? CALL printf with fmt string
*/

char bssVars[MAX_TAC*2][MAX_STR];
int  bssCount=0;

static void addBssVar(const char *name) {
    int i;
    if(!name||!name[0]) return;
    if(strchr(name,' ')) return;          /* skip "print x" strings */
    if(!isalpha((unsigned char)name[0])&&name[0]!='_') return;
    for(i=0;i<bssCount;i++)
        if(strcmp(bssVars[i],name)==0) return;
    strncpy(bssVars[bssCount++],name,MAX_STR-1);
}

static void emitLoad(const char *reg, const char *operand) {
    if(isNum(operand))
        printf("    MOV %s, %s\n", reg, operand);
    else
        printf("    MOV %s, [%s]\n", reg, operand);
}

void generateAssembly() {
    int i;
    printf("\n--- ASSEMBLY OUTPUT (Module 6) ---\n");

    /* collect variables for .bss */
    for(i=0;i<tacCount;i++) {
        TAC *t=&tacList[i];
        if(t->dead) continue;
        addBssVar(t->result);
        if(!t->isPrint) {
            if(!isNum(t->arg1)) addBssVar(t->arg1);
            if(!isNum(t->arg2)) addBssVar(t->arg2);
        }
    }

    /* sections */
    printf("\nsection .data\n");
    printf("    fmt db \"%%d\", 10, 0\n");

    printf("\nsection .bss\n");
    for(i=0;i<bssCount;i++)
        printf("    %-10s resd 1\n", bssVars[i]);

    printf("\nsection .text\n");
    printf("    global main\n");
    printf("    extern printf\n");
    printf("\nmain:\n");
    printf("    PUSH EBP\n");
    printf("    MOV  EBP, ESP\n\n");

    for(i=0;i<tacCount;i++) {
        TAC *t=&tacList[i];
        if(t->dead) continue;

        /* inline comment showing original TAC */
        if(t->isPrint)
            printf("    ; print %s\n", t->result);
        else if(t->isCopy)
            printf("    ; %s = %s\n", t->result, t->arg1);
        else
            printf("    ; %s = %s %s %s\n", t->result,t->arg1,t->op,t->arg2);

        if(t->isPrint) {
            printf("    MOV  EAX, [%s]\n", t->result);
            printf("    PUSH EAX\n");
            printf("    PUSH fmt\n");
            printf("    CALL printf\n");
            printf("    ADD  ESP, 8\n");
        }
        else if(t->isCopy) {
            emitLoad("EAX", t->arg1);
            printf("    MOV  [%s], EAX\n", t->result);
        }
        else {
            emitLoad("EAX", t->arg1);
            emitLoad("EBX", t->arg2);
            if     (t->op[0]=='+') printf("    ADD  EAX, EBX\n");
            else if(t->op[0]=='-') printf("    SUB  EAX, EBX\n");
            else if(t->op[0]=='*') printf("    IMUL EAX, EBX\n");
            else if(t->op[0]=='/'){printf("    CDQ\n");
                                   printf("    IDIV EBX\n");}
            printf("    MOV  [%s], EAX\n", t->result);
        }
        printf("\n");
    }

    printf("    MOV  EAX, 0\n");
    printf("    POP  EBP\n");
    printf("    RET\n");
}

/* ================================================================
   -------- MAIN : PIPELINE DRIVER  ------------------------------
   ================================================================ */

static void resetState() {
    tacCount=0; tempNum=1; cseCount=0; bssCount=0; pos=0;
    memset(tacList, 0, sizeof(tacList));
    memset(cseKey,  0, sizeof(cseKey));
    memset(cseTemp, 0, sizeof(cseTemp));
    memset(bssVars, 0, sizeof(bssVars));
}

int main() {
    /* ----  5 test programs ---- */
    char *programs[] = {
        "a=2*3+4; print a;",
        "a=2*3; b=2*3; print a; print b;",
        "x=10+5; y=x*2; z=y-3; print z;",
        "a=(2+3)*4; print a;",
        "a=4*5; b=a+3; c=b*2; print c;"
        
    };
    int n = 5, p;

    for(p=0;p<n;p++) {
        printf("\n");
        printf("==============================================\n");
        printf("  TEST PROGRAM %d : %s\n", p+1, programs[p]);
        printf("==============================================\n");

        resetState();

        /* Copy program into input / code buffers */
        strncpy(input, programs[p], 199);

        /* MODULE 1 : Lexer */
        lexer(programs[p]);

        /* MODULE 2 & 3 : Parser (numeric expressions only) */
        /* The full parser runs on pure arithmetic strings.  */
        /* For programs with assignments we show the lexer   */
        /* token stream; the parser evaluates sub-expressions*/
        printf("\n--- PARSER OUTPUT (Module 2 & 3) ---\n");
        /* Extract and evaluate the first arithmetic expression */
        {
            char exprOnly[200]; int i=0,j=0;
            /* find '=' then copy until ';' */
            while(programs[p][i] && programs[p][i]!='=') i++;
            if(programs[p][i]=='=') { i++;
                while(programs[p][i]==' ') i++;
                while(programs[p][i] && programs[p][i]!=';')
                    exprOnly[j++]=programs[p][i++];
            }
            exprOnly[j]='\0';
            strncpy(input, exprOnly, 199);
            pos=0;
            if(j>0) {
                int result = bossFunction();
                printf("Expression '%s' evaluates to: %d\n", exprOnly, result);
            }
        }

        /* MODULE 4 : TAC Generator */
        generateTACForProgram(programs[p]);

        /* MODULE 5 : Optimizer */
        optimize();

        /* MODULE 6 : Assembly Generator */
        generateAssembly();
    }

    return 0;
}
