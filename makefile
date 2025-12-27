LEX = flex
YACC = bison
CC = gcc
NASM = nasm
PY = python3

# 可配置变量
SOURCE ?= test
TARGET = $(SOURCE)

# 默认目标
all: $(TARGET)

# 词法分析
lex.yy.c: lex.l
	$(LEX) lex.l

# 语法分析
yacc.tab.c yacc.tab.h: yacc.y hashMap.h tree.h
	$(YACC) -d yacc.y

# 构建编译器
compiler: yacc.tab.c yacc.tab.h lex.yy.c hashMap.h tree.h tree.c stack.c hashMap.c inner.c optimizer.c
	$(CC) -o compiler yacc.tab.c lex.yy.c tree.c stack.c hashMap.c inner.c optimizer.c -lfl -lm

# 生成中间代码
Innercode: compiler $(SOURCE).c
	./compiler $(SOURCE).c

# 生成汇编代码
assembly.asm: assembly.py Innercode
	$(PY) $<

# 汇编
$(SOURCE).o: assembly.asm
	$(NASM) -f elf64 $< -o $@

# 链接
$(TARGET): $(SOURCE).o
	$(CC) -no-pie -o $@ $<

# 清理
clean:
	-rm -rf lex.yy.c yacc.tab.c yacc.tab.h compiler
	-rm -rf Grammatical Lexical Innercode assembly.asm
	-rm -rf $(SOURCE).o $(TARGET)

# 显示帮助
help:
	@echo "用法:"
	@echo "  make SOURCE=filename    # 编译指定的C文件"
	@echo "  make SOURCE=test        # 编译test.c (默认)"
	@echo "  make clean              # 清理生成的文件"
	@echo "  make help               # 显示此帮助信息"
	@echo ""
	@echo "示例:"
	@echo "  make SOURCE=1           # 编译1.c"
	@echo "  make SOURCE=myprogram  # 编译myprogram.c"

.PHONY: all clean help