section .text
global main

main:
    mov rax, 0x123456789abcdeff
    jmp 0x123456789
    jmp 0x87654321
