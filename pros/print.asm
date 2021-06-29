global _start   ; _start是一个符号(.symboil)，链接器会把其作为entry point

; 数据段
section .data
    buffer db 'Hello, system call.', 10, 0   ; buffer，10是换行符，0是字符串结束
    length db 40                            ; buffer的长度

; 代码段
section .text
    _start:
        mov eax, 4          ; 4，write系统调用
        mov ebx, 1         ; fd（文件描述符），1为标准输出
        mov ecx, buffer     ; buffer的地址
        mov edx, [length]   ; 根据地址从数据段获取buffer的长度
        int 0x80            ; system call

        mov eax, 1          ; 1，exit系统调用
        mov ebx, 0          ; exit code
        int 0x80            ; system call
