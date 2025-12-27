#include "errorReport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

ErrorReporter* createErrorReporter(const char* filename) {
    ErrorReporter* reporter = (ErrorReporter*)malloc(sizeof(ErrorReporter));
    reporter->errors = NULL;
    reporter->error_count = 0;
    if (filename) {
        reporter->error_file = fopen(filename, "w");
    } else {
        reporter->error_file = stderr;
    }
    return reporter;
}

void reportError(ErrorReporter* reporter, ErrorType type, int line, const char* format, ...) {
    if (!reporter) return;
    
    ErrorInfo* error = (ErrorInfo*)malloc(sizeof(ErrorInfo));
    error->type = type;
    error->line = line;
    error->next = NULL;
    
    // 格式化错误消息
    va_list args;
    va_start(args, format);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    error->message = (char*)malloc(strlen(buffer) + 1);
    strcpy(error->message, buffer);
    
    // 添加到链表
    error->next = reporter->errors;
    reporter->errors = error;
    reporter->error_count++;
    
    // 立即输出到文件
    const char* type_str[] = {
        "Lexical", "Syntax", "Semantic", "Type Mismatch", 
        "Undeclared", "Redeclared"
    };
    
    if (reporter->error_file) {
        fprintf(reporter->error_file, "[%s Error] Line %d: %s\n", 
                type_str[type], line, buffer);
        fflush(reporter->error_file);
    }
}

void printErrors(ErrorReporter* reporter) {
    if (!reporter || !reporter->errors) return;
    
    const char* type_str[] = {
        "Lexical", "Syntax", "Semantic", "Type Mismatch", 
        "Undeclared", "Redeclared"
    };
    
    printf("\n=== Error Report ===\n");
    printf("Total errors: %d\n\n", reporter->error_count);
    
    ErrorInfo* current = reporter->errors;
    while (current) {
        printf("[%s] Line %d: %s\n", 
               type_str[current->type], 
               current->line, 
               current->message);
        current = current->next;
    }
    printf("==================\n\n");
}

void freeErrorReporter(ErrorReporter* reporter) {
    if (!reporter) return;
    
    ErrorInfo* current = reporter->errors;
    while (current) {
        ErrorInfo* next = current->next;
        free(current->message);
        free(current);
        current = next;
    }
    
    if (reporter->error_file && reporter->error_file != stderr) {
        fclose(reporter->error_file);
    }
    
    free(reporter);
}

int getErrorCount(ErrorReporter* reporter) {
    return reporter ? reporter->error_count : 0;
}

