/******************************************************************************
    Copyright � 2012-2014 Martin Karsten

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
# GCC: arguments in %rdi, %rsi, %rdx, %rcx , %r8, %r9
# see http://en.wikipedia.org/wiki/X86_calling_conventions

# store/restore caller-owned registers
# see http://x86-64.org/documentation/abi.pdf, Sec 3.2.1

.text

.macro REGSAVE
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %rbx
	pushq %rbp
.endm

.macro REGRESTORE
	popq %rbp
	popq %rbx
	popq %r12
	popq %r13
	popq %r14
	popq %r15
.endm

#.globl stackStart
#.align 8
#stackStart:         # (nextSP)
#	movq %rdi, %rsp    # load next stack
#	REGRESTORE
#	retq

.globl stackSwitch
.align 8
stackSwitch:					# (uThread* nextuThread, void* args, &currSP, nextSP, post-fun)
	REGSAVE
	movq %rsp, (%rdx)  # save current stack
	movq %rcx, %rsp    # load next stack
	REGRESTORE
	jmpq *%r8          # post-func in %r8, arg in %rdi and %rsi

.globl stackInit
.align 8
stackInit:                     # stack, func, arg1, arg2, arg3, arg4 -> new stack
	movq $0,   -64(%rdi)       # indirectly set rbp to 0
	movq %rsi, -56(%rdi)       # store 'func' for stub function (via rbx)
	movq %rdx, -48(%rdi)       # store 'arg1' for stub function (via r12)
	movq %rcx, -40(%rdi)       # store 'arg2' for stub function (via r13)
	movq %r8,  -32(%rdi)       # store 'arg3' for stub function (via r14)
	movq %r9,  -24(%rdi)       # store 'arg4' for stub function (via r15)
	lea stackStub(%rip), %rax  # load the address of stackStub relative to IP
	mov %rax, -16(%rdi)	       # push stub function as return address
	leaq -64(%rdi), %rax       # return stack address, size 64: cf. REGSAVE
	retq

.align 8
stackStub:                   # stub needed to pass arguments to 'func'
	movq %r12, %rdi            # move 'arg1' into first argument
	movq %r13, %rsi            # move 'arg2' into second argument
	movq %r14, %rdx            # move 'arg3' into third argument
	movq %r15, %rcx            # move 'arg4' into fourth argument
	pushq %rbp                 # previous %rip = 0 (fake stack frame)
	pushq %rbp                 # previous %rbp = 0
	movq %rsp, %rbp
	jmp  *%rbx                 # jump to 'func' function
