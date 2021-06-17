# Trigger syscall
    .globl SYS_call
# Function arguments :
#   %esp + 4  : call id (eax)
#   %esp + 8  : param 1 (ebx)
#   %esp + 12 : param 2 (ecx)
#   %esp + 16 : param 3 (edx)
#   %esp + 20 : param 4 (edi)
#   %esp + 24 : param 5 (esi)
SYS_call:
# save registers except eax (return value)
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %edi
    pushl %esi
# esp moved of 20
# write params in registers
    movl 24(%esp), %eax
    movl 28(%esp), %ebx
    movl 32(%esp), %ecx
    movl 36(%esp), %edx
    movl 40(%esp), %edi
    movl 44(%esp), %esi
# interrupt switch to ring0
    int $49
# restore registers
    popl %esi
    popl %edi
    popl %edx
    popl %ecx
    popl %ebx
    ret

