# Context change
    .text
    .globl CTX_switch
# Function arguments :
#   %esp + 4 : old context container address
#   %esp + 8 : new context container address
CTX_switch:
    # saving old context
    movl 4(%esp), %eax
    movl %ebx, (%eax)
    movl %esp, 4(%eax)
    movl %ebp, 8(%eax)
    movl %esi, 12(%eax)
    movl %edi, 16(%eax)
    # restoring new context
    movl 8(%esp), %eax
    movl (%eax), %ebx
    movl 4(%eax), %esp
    movl 8(%eax), %ebp
    movl 12(%eax), %esi
    movl 16(%eax), %edi
    # switch to new process
    ret

# Process end on return
    .globl PROC_end
PROC_end:
# read retval
    pushl %eax
# call C function dealing with exit
    call exit
# exit never returns

# Switch to usermode
    .globl JMP_usermode
JMP_usermode:
# set segments to USER_DS
    movl $0x43, %eax
	movl %eax, %ds
	movl %eax, %es 
	movl %eax, %fs 
	movl %eax, %gs
    iret
