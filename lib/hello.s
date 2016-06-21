	.globl  main

        .text

main:
        # make new call frame
	push    %ebp
	mov     %esp, %ebp
	# push call frame
	push    $message
	# call the function
        call    puts
	# remove arguments (1x4 bytes) from frame (stack grows downwards)
	add     $4, %esp
	# restore old call frame
	pop     %ebp
	
        ret

message:
        .asciz "Hello World"            # asciz puts a 0 byte at the end
