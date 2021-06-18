#define SERV_EXC_TASK(id) \
	.text; \
trap##id: \
	pushl	$(0); \
	pushl	$(id); \
	cld;	/* for gcc */ \
	call	trap_handler; \
	addl	$(8),%esp; \
	iret; \
	jmp	trap##id; \
	.data; \
	.long	trap##id

#define SERV_ERR_TASK(id) \
	.text; \
trap##id: \
	pushl	$(id); \
	cld;	/* for gcc */ \
	call	trap_handler; \
	addl	$(8),%esp; \
	iret; \
	jmp	trap##id; \
	.data; \
	.long	trap##id

	.data
.globl exception_handler_tasks
exception_handler_tasks:
SERV_EXC_TASK(0)
SERV_EXC_TASK(1)
SERV_EXC_TASK(2)
SERV_EXC_TASK(3)
SERV_EXC_TASK(4)
SERV_EXC_TASK(5)
SERV_EXC_TASK(6)
SERV_EXC_TASK(7)
SERV_ERR_TASK(8)
SERV_EXC_TASK(9)
SERV_ERR_TASK(10)
SERV_ERR_TASK(11)
SERV_ERR_TASK(12)
SERV_ERR_TASK(13)
SERV_ERR_TASK(14)
SERV_EXC_TASK(15)
SERV_EXC_TASK(16)
SERV_ERR_TASK(17)
SERV_EXC_TASK(18)
SERV_EXC_TASK(19)
SERV_EXC_TASK(20)
SERV_EXC_TASK(21)
SERV_EXC_TASK(22)
SERV_EXC_TASK(23)
SERV_EXC_TASK(24)
SERV_EXC_TASK(25)
SERV_EXC_TASK(26)
SERV_EXC_TASK(27)
SERV_EXC_TASK(28)
SERV_EXC_TASK(29)
SERV_EXC_TASK(30)
SERV_EXC_TASK(31)

# Interrupt handlers
# Handles programable clock interrupts
    .globl IT_PIT_handler
IT_PIT_handler:
# save important registers
	pushl %eax
    pushl %edx
    pushl %ecx
# call C function dealing with interrupt
    call tic_PIT
# restore important registers
	popl %ecx
    popl %edx
    popl %eax
# end interrupt handler
    iret

# Handles keyboard interrupts
    .globl IT_KEYBOARD_handler
IT_KEYBOARD_handler:
# save important registers
	pushl %esi
	pushl %edi
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
# call C function dealing with interrupt
    call keyboard_IT
# restore important registers
	popl %eax
	popl %ebx
	popl %ecx
	popl %edx
	popl %edi
	popl %esi
# end interrupt handler
    iret

# Handles user system calls
    .globl IT_USR_handler
IT_USR_handler:
# call C function dealing with interrupt
	pushl %esi
	pushl %edi
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl %eax
# save data segments
	movl $0x18, %eax
	movl %eax, %ds
	movl %eax, %es 
	movl %eax, %fs 
	movl %eax, %gs
	call user_IT
# return code in eax
# restore data segments
	pushl %eax
	movl $0x43, %eax
	movl %eax, %ds
	movl %eax, %es 
	movl %eax, %fs 
	movl %eax, %gs
	popl %eax
	addl $24, %esp
# end interrupt handler
    iret
