	.globl  main

        .text

main:
	# make space on stack for two local integers
	sub     $8, %esp
	
	# call read_int (for details see hello.s)
	push    %ebp
	mov     %esp, %ebp
        call    read_int
	pop     %ebp
	mov     %eax, 0(%esp)           # move function result to stack position 1
	
	# call another read_int
	push    %ebp
	mov     %esp, %ebp
        call    read_int
	pop     %ebp
	mov     %eax, 4(%esp)           # move function result to stack position 2
	
	# add the two integers
	mov     0(%esp), %eax
	add     4(%esp), %eax
	
	# call print_int to print result
	push    %ebp
	mov     %esp, %ebp
	push    %eax
        call    print_int
	add     $4, %esp
	# restore old call frame
	pop     %ebp
	
	# free local stack
	add     $8, %esp
	
        ret
