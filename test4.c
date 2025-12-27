int main()
{
    int a;
    float b;
    int c;

    a = 10;
    b = 3.14;

    /* 测试 1: 正常赋值 */
    c = a; // INT = INT (OK)

    /* 测试 2: 隐式提升 (通常允许，不报警) */
    b = a; // FLOAT = INT (OK)

    /* 测试 3: 精度丢失警告 */
    a = b; // INT = FLOAT (应触发 Warning)

    /* 测试 4: 表达式类型传播与检查 */
    /* b + 1.5 结果是 FLOAT，赋值给 INT 变量 a */
    a = b + 1.5; // INT = FLOAT (应触发 Warning)

    return 0;
}