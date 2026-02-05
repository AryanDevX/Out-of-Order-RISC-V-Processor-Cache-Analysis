# .text: stores executable instructions of the program.
# .data: for defining global variables and stored in static memory area.

.data

    prompt1: .string "\nEnter integer n or -1 to exit: " #String is used to convert character into ascii values and also add \0 at end that tell the end of string.
    prompt2: .string "\nClosed Form ans: "
    prompt3: .string "\nIterative Ans: "
    prompt4: .string "\nRecursive Ans: "
    prompt5: .string "\nMemoization Ans: "
    buffer: .zero 255 #Reserve 255 bytes of memory and fill them with 0.
    prompt_end: .string "\nProgram is succesffully ended!"
    memo: .zero 262140 #We need 65535*4 (As 1 int takes 4 bytes),the memo array.
    prompt6: .string "\nSorry, our program doesn't support an integer equal to or greater than 65535."
    prompt7: .string "\nNegative integers except -1(i.e for EXIT) are not allowed."


.text 
main:
    li a7, 4 #Loading 4 into register a7. 4 is used to tell OS that we want to print the string.
    la a0, prompt1 #Loading the address of promp1
    ecall # The system look at a7 and according to the value in it, it do that operation. Ecall = Environmental call.

    call readInt #Calling our function for reading integer.
    mv t0,a0  #Storing result temporarily
    #Don't use t0 in functions as it hold n.

    #Exit
    li t1, -1
    beq t0, t1, exit_program

    #Negative Num:
    li t1, 0
    blt t0, t1, neg_num
    
    #Too large Num:
    li t1, 65536
    bge t0, t1, greater_num

    #Closed Formula:
    li a7, 4
    la a0, prompt2 
    ecall 
    mv a0, t0
    call closed_formula
    li a7, 1 #Print integer.
    ecall #ecall treat as signed no. so using unsigned doesn't matter. Therefore the max no. for which we can get triangular no. is 65534 here (For 32 bit register)
    
    #Iterative:
    li a7, 4
    la a0, prompt3 
    ecall 
    mv a0, t0 
    call iterative
    li a7, 1
    ecall
    mv a0, t0    #not necessary ig cause a0 eventualy overwritten as prompt4 in line 61, same like this on line 66
    #Recursive:
    li a7, 4
    la a0, prompt4 
    ecall 
    mv a0, t0 
    call recursive
    li a7, 1
    ecall
    mv a0, t0  
    

    #Memoized Recursive 
    li a7, 4
    la a0, prompt5
    ecall
    mv a0, t0
    call memoized_recursive
    li a7, 1
    ecall
    
    li a7, 11
    li a0, 10 # ascii newline
    ecall
    j main




#Reading from console:
readInt:
    li a7, 63          # syscall: read
    li a0, 0           
    la a1, buffer
    li a2, 255
    ecall
    la t1, buffer      # pointer to buffer
    li a0, 0           # result
    li t2, 10          # base 10
    li t5, 1           # sign = +1

parse_loop:
    lbu t3, 0(t1)
    li t4, 10          # newline '\n'
    beq t3, t4, parse_end
    beqz t3, parse_end
    li t6, 45          # ASCII '-'
    beq t3, t6, set_negative
    addi t3, t3, -48   # ASCII to int
    mul a0, a0, t2
    add a0, a0, t3
    addi t1, t1, 1
    j parse_loop

set_negative:
    li t5, -1          # sign = -1
    addi t1, t1, 1
    j parse_loop

parse_end:
    mul a0, a0, t5     # apply sign
    ret


closed_formula:
    addi t1, a0, 1 
    andi t2, a0, 1
    li t3, 2
    bnez t2, is_odd

is_even:
    div a0, a0, t3 
    mul a0, a0, t1 
    ret 

is_odd:
    div t1, t1, t3
    mul a0, a0, t1 
    ret



iterative: 
    li t1, 1
    li t2, 0
    blez a0, end_loop 

loop_start:
    add t2, t2, t1 
    beq t1, a0, end_loop 
    addi t1, t1, 1
    j loop_start 

end_loop:
    mv a0, t2 
    ret 



recursive: 
    beq a0, zero,  return_zero
    li t1, 1
    beq a0, t1, base_case
    addi sp, sp, -8 
    sw ra, 4(sp)
    sw a0, 0(sp)
    addi a0, a0, -1 
    jal ra, recursive
    lw t2, 0(sp)
    lw ra, 4(sp)
    addi sp, sp, 8
    add a0, a0, t2 
    ret 

base_case:
    li a0, 1
    ret 

return_zero:
    li a0, 0
    ret



memoized_recursive:
    beq a0, zero, memo_zero
    li t1, 1
    beq a0, t1, memo_one
    slli t2, a0, 2       
    la t3, memo
    add t3, t3, t2      
    lw t4, 0(t3)         
    bnez t4, memo_end    
    addi sp, sp, -8
    sw ra, 4(sp)
    sw a0, 0(sp)
    addi a0, a0, -1
    jal ra, memoized_recursive
    lw t5, 0(sp)        
    lw ra, 4(sp)
    addi sp, sp, 8
    add a0, a0, t5       
    sw a0, 0(t3)         
    ret
    
memo_zero:
    li a0, 0
    la t3, memo
    sw a0, 0(t3)     
    ret

memo_one:
    li a0, 1
    la t3, memo
    sw a0, 4(t3)    
    ret
    
memo_end:
    mv a0, t4
    ret


#Negative Number:
neg_num:
    li a7, 4
    la a0, prompt7
    ecall 
    j main 



#Greater Number:
greater_num:
    li a7, 4
    la a0, prompt6 
    ecall 
    j main



#Exit
exit_program:
    li a7, 4
    la a0, prompt_end
    ecall

    li a7, 10
    ecall