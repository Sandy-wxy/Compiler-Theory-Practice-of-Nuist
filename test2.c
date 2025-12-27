int main()
{
    int cube[3][3][3];
    int x, y, z;
    int val;

    x = 1;
    y = 1;
    z = 1;

    /* 验证多维写入 */
    cube[x][y][z] = 99;

    /* 验证多维读取与算术结合 */
    val = cube[x][y][z] + 1;

    output_int(val); // 预期输出 100
    return 0;
}