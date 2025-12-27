extern printf, scanf
global main
section .text
main:
        push rbp
        mov rbp, rsp
        sub rsp, 96
        mov eax, 10
        mov [rbp-4], eax
        mov eax, [rbp-8]
        mov [rbp-12], eax
        mov eax, [rbp-4]
        mov [rbp-16], eax
        mov eax, [rbp-4]
        mov [rbp-12], eax
        mov eax, [rbp-12]
        mov [rbp-4], eax
        mov eax, [rbp-12]
        mov eax, [rbp-12]
        add eax, [rbp-20]
        mov [rbp-24], eax
        mov eax, [rbp-24]
        mov [rbp-4], eax
        mov eax, 0
        leave
        ret
END:
        leave
        ret

section .data
        out_format: db "%d", 10, 0
        in_format: db "%d", 0

section .bss
        number: resb 4
