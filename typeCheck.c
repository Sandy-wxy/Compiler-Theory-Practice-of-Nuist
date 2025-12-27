#include "typeCheck.h"

TypeInfo *typeCheckBinaryOp(TypeInfo *left, TypeInfo *right, char *op)
{
    if (!isTypeCompatible(left, right))
    {
        yyerror("Type mismatch in binary operation");
        return NULL;
    }

    // 指针运算特殊处理
    if (left->pointer_level > 0 || right->pointer_level > 0)
    {
        return handlePointerArithmetic(left, right, op);
    }

    return left; // 返回提升后的类型
}

bool isTypeCompatible(TypeInfo *t1, TypeInfo *t2)
{
    if (t1->base_type != t2->base_type)
        return false;
    if (t1->pointer_level != t2->pointer_level)
        return false;
    return true;
}