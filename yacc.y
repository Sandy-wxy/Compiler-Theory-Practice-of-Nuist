%{
    #include "tree.h"
    #include "hashMap.h"
    #include "inner.h"
    #include "optimizer.h"
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>

    void output();
    int yylex();
    void yyerror(const char *s); // 错误分析
    
    extern int yylineno;
    extern FILE* yyout;
    FILE *out, *outInner;
    
    HashMap* hashMap = NULL;    // 符号表
    int scope = 0;
    struct Declator* declator;
    int type;
    int preType;

    Tree* root;
    // 关键修复：使用 extern 避免与 inner.c 中的定义冲突
    extern struct Node *head;    
    
    int line_count = 1;

    // 函数原型声明
    void exportDot(struct Tree* node, FILE* fp);
%}

%union {
    struct Tree* tree;
}

/* 注册 Token */
%token <tree> FLOAT FLOAT_CONST
%token <tree> INT8 INT10 INT16
%token <tree> ID
%token <tree> INT
%token <tree> VOID MAIN RET CONST STATIC AUTO IF ELSE WHILE DO BREAK CONTINUE SWITCH CASE STRUCT
%token <tree> DEFAULT SIZEOF TYPEDEF VOLATILE GOTO INPUT OUTPUT
%token <tree> LE GE AND OR ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN 
%token <tree> INC DEC EQUAL NE PTR FOR STR
%token <tree> '(' ')' '[' ']' '{' '}' '!' '^' '&' '*' '-' '+' '=' '/' '%' ',' ';' '<' '>'

%type <tree> type
%type <tree> constant primary_expression postfix_expression unary_expression cast_expression
%type <tree> multiplicative_expression additive_expression relational_expression equality_expression
%type <tree> logical_and_expression logical_or_expression assignment_expression operate_expression declare_expression
%type <tree> nullable_expression while_expression for_expression funcion_expression if_expression if_identifier return_expression null unary_operator
%type <tree> main_function sentence statement assignment_operator single_expression
%type <tree> argument_list
%type <tree> external_declaration program
%precedence ')'
%precedence ELSE

%%

/* 规则部分 */

project:
    program
    {       
        root = createTree("Project", 1, $1);
    }
;

program
    : external_declaration
    | program external_declaration
    {
        $$ = createTree("program", 2, $1, $2);
    }
;

external_declaration
    : main_function
    | single_expression 
    {
        $$ = $1; 
    }
;

main_function
    : VOID MAIN '(' ')' '{' sentence '}'
    {
        $$ = createTree("Main Func", 7, $1, $2, $3, $4, $5, $6, $7);
    }
    | type MAIN '(' ')' '{' sentence '}'
    {
        $$ = createTree("Main Func", 7, $1, $2, $3, $4, $5, $6, $7);
    }
;

sentence
    : statement sentence
    {
        $$ = createTree("sentence", 2, $1, $2);
    }
    | statement
;

constant
    : INT10       { $$ = $1; $$->type = 262; }
    | FLOAT_CONST { $$ = $1; $$->type = 300; }
    | INT8        { $$ = $1; $$->type = 262; }
    | INT16       { $$ = $1; $$->type = 262; }
;

primary_expression
    : ID
    {/* ============ 修改开始 ============ */
        /* 1. 构造一个假的数据包去查表 */
        Data* queryData = toData(0, $1->content, NULL, scope);
        
        /* 2. 在符号表(hashMap)里搜索这个变量 */
        HashNode* hashNode = get(hashMap, queryData);
// === 修改开始：详细的调试打印 ===
        if(hashNode == NULL){
            //printf("[DEBUG_YACC] Lookup failed for '%s' at Scope %d.\n", $1->content, scope);
            $1->type = 0; 
        } else {
           // printf("[DEBUG_YACC] Lookup SUCCESS for '%s'. Found Type: %d\n", $1->content, hashNode->data->type);
            $1->type = hashNode->data->type;
        }
        // === 修改结束 ===
        $$ = $1;
    }
    | constant
    {
        $$ = $1;
    }
    | '(' operate_expression ')'
    {
        $$ = createTree("primary_expression", 3, $1, $2, $3);
        /* 类型透传：(i+f) 的类型等同于内部表达式的类型 */
        if($2) {
            $$->type = $2->type;
        }
    }
;

postfix_expression
    : primary_expression
    | postfix_expression '[' operate_expression ']'
    {
        $$ = addDeclator("Array", $1, $3);
    }
    | postfix_expression INC
    {
        $$ = createTree("postfix_expression", 2, $1, $2);
    }
    | postfix_expression DEC
    {
        $$ = createTree("postfix_expression", 2, $1, $2);
    }
;

unary_operator
    : '+'
    | '-'
    | '!'
;

unary_expression
    : postfix_expression
    | INC postfix_expression { $$ = unaryOpr("unary_expression", $1, $2); }
    | DEC postfix_expression { $$ = unaryOpr("unary_expression", $1, $2); }
    | unary_operator cast_expression
    {
        if(!strcmp($1->content, "*")){
            $$ = addDeclator("Pointer", $2, $1);
        } else {
            $$ = unaryOpr("unary_expression", $1, $2);
        }
    }
    | '*' cast_expression { $$ = addDeclator("Pointer", $2, $1); } 
    | '&' cast_expression { $$ = createTree("address_of", 2, $1, $2); } 
;

cast_expression
    : unary_expression
;

multiplicative_expression
    : cast_expression
    | multiplicative_expression '*' cast_expression { $$ = binaryOpr("multiplicative_expression", $1, $2, $3); }
    | multiplicative_expression '/' cast_expression { $$ = binaryOpr("multiplicative_expression", $1, $2, $3); }
    | multiplicative_expression '%' cast_expression { $$ = binaryOpr("multiplicative_expression", $1, $2, $3); }
    | multiplicative_expression '^' cast_expression { $$ = binaryOpr("multiplicative_expression", $1, $2, $3); }
;

additive_expression
    : multiplicative_expression
    | additive_expression '+' multiplicative_expression { $$ = binaryOpr("additive_expression", $1, $2, $3); }
    | additive_expression '-' multiplicative_expression { $$ = binaryOpr("additive_expression", $1, $2, $3); }
;

relational_expression
    : additive_expression
    | relational_expression '<' additive_expression { $$ = binaryOpr("relational_expression", $1, $2, $3); }
    | relational_expression '>' additive_expression { $$ = binaryOpr("relational_expression", $1, $2, $3); }
    | relational_expression LE additive_expression  { $$ = binaryOpr("relational_expression", $1, $2, $3); }
    | relational_expression GE additive_expression  { $$ = binaryOpr("relational_expression", $1, $2, $3); }
;

equality_expression
    : relational_expression
    | equality_expression EQUAL relational_expression { $$ = binaryOpr("equality_expression", $1, $2, $3); }
    | equality_expression NE relational_expression    { $$ = binaryOpr("equality_expression", $1, $2, $3); }
;

logical_and_expression
    : equality_expression
    | logical_and_expression AND equality_expression { $$ = binaryOpr("logical_and_expression", $1, $2, $3); }
;

logical_or_expression
    : logical_and_expression
    | logical_or_expression OR logical_and_expression { $$ = binaryOpr("logical_or_expression", $1, $2, $3); }
;

assignment_operator
    : '=' | MUL_ASSIGN | DIV_ASSIGN | MOD_ASSIGN | ADD_ASSIGN | SUB_ASSIGN
;

assignment_expression
    : logical_or_expression
    | unary_expression assignment_operator assignment_expression
    {
        // 逻辑全部移交 tree.c 中的 assignOpr，此处只负责调用
        $$ = assignOpr("assignment_expression", $1, $2, $3);
    }
;

operate_expression
    : assignment_expression
    | operate_expression ',' assignment_expression
    {
        $$ = createTree("operate_expression", 3, $1, $2, $3);
    }
;

null: { $$ = createTree("null", 0, yylineno); };

nullable_expression
    : null
    | operate_expression
;

declare_expression
    : type operate_expression
    {
        $$ = createTree("declare_expression", 2, $1, $2);
        if($2){
            putTree(hashMap, $2);
            preType = type = 0;
        }
    }
;

single_expression
    : STRUCT ID '{' sentence '}' ';' 
      { $$ = createTree("struct_declaration", 5, $1, $2, $3, $4, $5); }
    | postfix_expression '.' ID 
      { $$ = createTree("struct_member", 2, $1, $3); }
    | operate_expression ';' { $$ = $1; }
    | declare_expression ';' { $$ = $1; }
    | if_expression          { $$ = $1; }
    | while_expression       { $$ = $1; }
    | for_expression         { $$ = $1; }
    | funcion_expression ';' { $$ = $1; }
    | return_expression ';'  { $$ = $1; }
    | ';'                    { $$ = createTree("null", 0, yylineno); }
;

if_expression 
    : if_identifier ')' statement
    {
        int headline = $1->headline;
        int nextline = line_count;
        $$ = ifOpr("if_expression", headline, nextline, $1, $3);
    }
    | if_identifier ')' statement ELSE {$1->nextline = line_count++;} statement
    {
        int headline = $1->headline;
        int next1 = $1->nextline;
        int next2 = line_count;
        $$ = ifelseOpr("if_else_expression", headline, next1, next2, $1, $3, $6);
    }
;

if_identifier
    : IF  '(' operate_expression
    {
        $$ = $3;
        $$->headline = line_count;
        line_count += 2;
    }
;

statement
    : '{' sentence '}' { $$ = createTree("statement", 3, $1, $2, $3); }
    | '{' '}' { $$ = createTree("statement", 2, $1, $2); }
    | single_expression { head = NULL; }
;

for_expression
    : FOR '(' nullable_expression { $3->headline = line_count; } ';' 
      nullable_expression ';' { $6->headline = line_count; } 
      nullable_expression ')' statement
    {
        line_count += 2;
        int head1 = $3->headline;
        int head2 = $6->headline;
        int nextline = line_count++;
        $$ = forOpr("for_expression", head1, head2, nextline, $3, $6, $9, $11);
    }
;

while_expression
    : WHILE { $1->headline = line_count; } '(' operate_expression ')' 
    {
        $4->headline = line_count;
        line_count += 2;
    } statement 
    {
        int head1 = $1->headline;
        int head2 = $4->headline;
        int nextline = line_count++;
        $$ = whileOpr("while_expression", head1, head2, nextline, $4, $7);
    }
;

funcion_expression
    : ID '(' argument_list ')' { $$ = createTree("call", 2, $1, $3); }
    | INPUT '(' operate_expression ')' { $$ = unaryFunc("input", $1, $3); line_count += 2; }
    | OUTPUT '(' operate_expression ')' { $$ = unaryFunc("output", $1, $3); line_count += 2; }
;

argument_list
    : operate_expression { $$ = $1; }
    | argument_list ',' operate_expression { $$ = createTree("arg_list", 3, $1, $2, $3); }
    | /* 空 */ { $$ = createTree("null", 0, yylineno); }
;

return_expression
    : RET { $$ = retNull("return_expression", $1); }
    | RET operate_expression { $$ = retOpr("return_expression", $1, $2); }
;

/* Pro 修复：强制设置 type 为 262 和 300，确保 putTree 能正确写入符号表 */
type
    : INT   { type = 262; $$ = $1; }
    | FLOAT { type = 300; $$ = $1; } 
;

%%

void yyerror(const char* s){
    printf("Error: %s at line %d\n", s, yylineno);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const char* outFile = "Lexical";
    const char* outFile2 = "Grammatical";
    const char* outFile3 = "Innercode";
    extern FILE* yyin, *yyout;

    type = 0;
    line_count = 1;
    hashMap = createHashMap(50); 
    
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror("Error opening input file");
        return 1;
    }

    yyout = fopen(outFile, "w");
    out = fopen(outFile2, "w");
    outInner = fopen(outFile3, "w");

    fprintf(yyout, "%-15s\t%-15s\t%s\n", "Word", "Lexeme", "Attribute");

    if (!yyparse()) {
        printf("Parse successful.\n");
        if (root) {
            /* 1. 执行优化：常量折叠 (1) | 死代码 (4) = 5 */
            root = optimizeTree(root, 5); 

            /* 2. 处理行号替换 */
            int seek = 1;
            int current_line = 1;
            if (root->code) {
                while (seek) {
                    seek = swap(root->code, "#", lineToString(current_line++));
                }
                fprintf(outInner, "%s", root->code);
            }

            /* 3. 打印语法树及导出 DOT 文件 */
            printTree(root);
            FILE* dotFile = fopen("tree.dot", "w");
            if (dotFile) {
                fprintf(dotFile, "digraph AST {\n");
                exportDot(root, dotFile); 
                fprintf(dotFile, "}\n");
                fclose(dotFile);
                printf("AST saved to tree.dot\n");
            }
        }
    } else {
        printf("Parse failed.\n");
    }

    fclose(out);
    fclose(outInner);
    fclose(yyin);
    fclose(yyout);
    destoryHashMap(hashMap);
    return 0;
}