#ifndef ERROR_REPORT_H
#define ERROR_REPORT_H

#include <stdio.h>

// 错误类型
typedef enum {
    ERR_LEXICAL,      // 词法错误
    ERR_SYNTAX,       // 语法错误
    ERR_SEMANTIC,     // 语义错误（类型检查、未声明等）
    ERR_TYPE_MISMATCH,// 类型不匹配
    ERR_UNDECLARED,   // 未声明变量
    ERR_REDECLARED    // 重复声明
} ErrorType;

// 错误信息结构
typedef struct {
    ErrorType type;
    int line;
    char* message;
    struct ErrorInfo* next;
} ErrorInfo;

// 错误报告器
typedef struct {
    ErrorInfo* errors;
    int error_count;
    FILE* error_file;
} ErrorReporter;

// 创建错误报告器
ErrorReporter* createErrorReporter(const char* filename);

// 报告错误
void reportError(ErrorReporter* reporter, ErrorType type, int line, const char* format, ...);

// 打印所有错误
void printErrors(ErrorReporter* reporter);

// 释放错误报告器
void freeErrorReporter(ErrorReporter* reporter);

// 获取错误数量
int getErrorCount(ErrorReporter* reporter);

#endif

