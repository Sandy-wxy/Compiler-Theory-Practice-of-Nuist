import sys

# 存储四元式的列表
four = []

def get_quadruples():
    try:
        # 读取由编译器生成的中间代码文件
        with open('Innercode', 'r') as file:
            stmts = file.readlines()
        
        for stmt in stmts:
            # 去除行号前缀 (例如 "1: ") 并分割指令
            line = stmt.strip()
            if not line: continue
            if ':' in line:
                line = line.split(':', 1)[1].strip()
            
            ops = line.split()
            # 尝试将数字标识符转换为 int 类型以便后续处理
            for i in range(len(ops)):
                try:
                    ops[i] = int(ops[i])
                except:
                    pass
            
            # 解析不同类型的中间代码指令 [cite: 122]
            if len(ops) < 2: continue
            
            if ops[0] == "if":
                # 格式: if t1 goto 6 -> (j==, t1, 1, 6)
                four.append(("j==", ops[1], 1, ops[3]))
            elif ops[0] == "goto":
                four.append(('j', '_', '_', ops[1]))
            elif ops[0] == "arg":
                four.append(('arg', '_', '_', ops[1]))
            elif ops[0] == "call":
                four.append(('call', '_', '_', ops[1]))
            elif ops[0] == "return":
                four.append(('ret', '_', '_', ops[1]))
            elif len(ops) > 2 and ops[1] == '=':
                if len(ops) == 3: # 赋值: a = 10
                    four.append(('=', ops[2], '_', ops[0]))
                else: # 运算: t1 = a > b
                    four.append((ops[3], ops[2], ops[4], ops[0]))
    except FileNotFoundError:
        print("错误: 找不到 Innercode 文件，请先运行编译器生成中间代码。")
        sys.exit(1)

# 获取四元式
get_quadruples()

# 汇编指令映射表 [cite: 112, 113, 114]
rule = {
    "j==": ["判断", "je"],
    "=": ["赋值", "mov"],
    "+": ["运算", "add"],
    "-": ["运算", "sub"],
    "*": ["运算", "imul"],
    "/": ["运算", "idiv"],
    "%": ["运算", "idiv"],
    ">": ["关系", "jg"],
    "<": ["关系", "jl"],
    ">=": ["关系", "jge"],
    "<=": ["关系", "jle"],
    "==": ["关系", "je"],
    "!=": ["关系", "jne"],
    "j": ["转移", "jmp"],
    "ret": ["返回", "ret"],
    "arg": ["函数", "arg"],
    "call": ["函数", "call"],
    "arg": ["函数", "push"],   
    "address_of": ["运算", "lea"],
    "deref": ["运算", "mov"],
    "FLOAT_CONST": ["赋值", "movss"], 
}

# 自动生成跳转标签
t = []
for n, line in enumerate(four):
    if rule.get(line[0], [""])[0] in ["判断", "转移"]:
        t.append(line[3] - 1) # 记录跳转目标的索引

t = sorted(list(set(t)))
four_in = {idx: f"CODE{i+1}" for i, idx in enumerate(t)}

# 汇编生成逻辑
result = []
tab = "        "
data_map = {} # 变量到栈偏移的映射
stack_ptr = 4

def generate_asm_for_array(op_tuple):
    target = get_addr(op_tuple[3])
    base = get_addr(op_tuple[1])
    offset = get_addr(op_tuple[2])
    
    # 数组元素加载示例：mov eax, [base + offset] 
    result.append(f"{tab}mov rsi, {offset}")
    result.append(f"{tab}mov rdi, {base}")
    result.append(f"{tab}mov eax, [rdi + rsi]")
    result.append(f"{tab}mov {target}, eax")


def get_addr(var):
    global stack_ptr
    if isinstance(var, int):
        return str(var)
    if var not in data_map:
        data_map[var] = stack_ptr
        stack_ptr += 4
    return f"[rbp-{data_map[var]}]"

# 初始化汇编头部
result.append("extern printf, scanf")
result.append("global main")
result.append("section .text")
result.append("main:")
result.append(f"{tab}push rbp")
result.append(f"{tab}mov rbp, rsp")
result.append(f"{tab}sub rsp, 96") # 预留栈空间



# 遍历四元式生成指令
for i, op_tuple in enumerate(four):
    op = op_tuple[0]
    
    # 插入标签
    if i in four_in:
        result.append(f"{four_in[i]}:")
    
    op_type = rule.get(op, ["未知", ""])[0]
    
    if op_type == "赋值":
        if op == "FLOAT_CONST": # 识别浮点常量
            result.append(f"{tab}movss xmm0, {get_addr(op_tuple[1])}")
            result.append(f"{tab}movss {get_addr(op_tuple[3])}, xmm0")
        else:
            result.append(f"{tab}mov eax, {get_addr(op_tuple[1])}")
            result.append(f"{tab}mov {get_addr(op_tuple[3])}, eax")
        
    elif op_type == "运算":
        result.append(f"{tab}mov eax, {get_addr(op_tuple[1])}")
        if op == "address_of":
            # 对应 p = &a -> lea rax, [rbp-offset_a]
            result.append(f"{tab}lea rax, {get_addr(op_tuple[1])}")
            result.append(f"{tab}mov {get_addr(op_tuple[3])}, rax")
        elif op == "deref":
            # 对应 val = *p -> mov rax, [p]; mov eax, [rax]
            result.append(f"{tab}mov rax, {get_addr(op_tuple[1])}")
            result.append(f"{tab}mov eax, [rax]")
            result.append(f"{tab}mov {get_addr(op_tuple[3])}, eax")
        else:
            # 普通算术运算 (包含数组下标计算 t1 = i * 4)
            result.append(f"{tab}mov eax, {get_addr(op_tuple[1])}")
        if op in ["/", "%"]:
            result.append(f"{tab}cdq") # 符号扩展 eax 到 edx:eax
            result.append(f"{tab}idiv dword {get_addr(op_tuple[2])}")
            target = "edx" if op == "%" else "eax"
            result.append(f"{tab}mov {get_addr(op_tuple[3])}, {target}")
        else:
            instr = rule[op][1]
            result.append(f"{tab}{instr} eax, {get_addr(op_tuple[2])}")
            result.append(f"{tab}mov {get_addr(op_tuple[3])}, eax")
            
    elif op_type == "关系":
        # 关系运算通常紧跟 if 跳转
        result.append(f"{tab}mov eax, {get_addr(op_tuple[1])}")
        result.append(f"{tab}cmp eax, {get_addr(op_tuple[2])}")
        # 这里仅作记录，实际跳转由随后的 j== 处理
        
    elif op_type == "判断":
        # 如果前一条是关系运算，则根据关系生成跳转
        prev_op = four[i-1][0] if i > 0 else ""
        jmp_instr = rule.get(prev_op, ["", "je"])[1] if rule.get(prev_op, [""])[0] == "关系" else "jne"
        target_label = four_in.get(op_tuple[3]-1, "END")
        result.append(f"{tab}{jmp_instr} {target_label}")

    elif op_type == "转移":
        target_label = four_in.get(op_tuple[3]-1, "END")
        result.append(f"{tab}jmp {target_label}")

    elif op_type == "函数":
        if op == "arg":
            # 简化版：仅支持一个参数通过 rsi 传递给 printf
            result.append(f"{tab}mov esi, {get_addr(op_tuple[3])}")
        elif op == "call":
            if op_tuple[3] in ["output", "output_int"]:
                result.append(f"{tab}mov rdi, out_format")
                result.append(f"{tab}xor eax, eax")
                result.append(f"{tab}call printf")

    elif op_type == "返回":
        result.append(f"{tab}mov eax, {get_addr(op_tuple[3])}")
        result.append(f"{tab}leave")
        result.append(f"{tab}ret")

# 结尾处理
result.append("END:")
result.append(f"{tab}leave")
result.append(f"{tab}ret")

# 数据段
result.append("\nsection .data")
result.append(f'{tab}out_format: db "%d", 10, 0')
result.append(f'{tab}in_format: db "%d", 0')

result.append("\nsection .bss")
result.append(f'{tab}number: resb 4')

# 写入文件
with open('assembly.asm', 'w') as f:
    for line in result:
        f.write(line + "\n")

print("汇编代码已成功生成至 assembly.asm")