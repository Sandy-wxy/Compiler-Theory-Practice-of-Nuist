int main()
{
    int a;
    int b;

    /* 常量折叠 (Constant Folding) */
    a = 10 * 5 + 2;

    b = 100;

    b = a + 0;
    b = b * 1;

    return 0;
}