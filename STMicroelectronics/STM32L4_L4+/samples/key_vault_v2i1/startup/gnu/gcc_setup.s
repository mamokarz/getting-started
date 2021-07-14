
    .text
    .align 4
    .syntax unified

    .global _gcc_setup
    .thumb_func
_gcc_setup:

    STMDB   sp!, {r3, r4, r5, r6, r7, lr}             // Store other preserved registers

    ldr     r3, =__FLASH_segment_start__
    ldr     r4, =__RAM_segment_start__
    mov     r5,r0

    /* Copy GOT table. */
  
    ldr     r0, =__got_load_start__
    sub     r0,r0,r3
    add     r0,r0,r5
    ldr     r1, =__new_got_start__
    sub     r1,r1, r4
    add     r1,r1,r9
    ldr     r2, =__new_got_end__
    sub     r2,r2,r4
    add     r2,r2,r9

new_got_setup:
    cmp     r1, r2          // See if there are more GOT entries
    beq     got_setup_done  // No, done with GOT setup
    ldr     r6, [r0]        // Pickup current GOT entry
    cmp     r6, #0          // Is it 0?
    beq     address_built   // Yes, just skip the adjustment
    cmp     r6, r4          // Is it in the code or data area?
    blt     flash_area      // If less than, it is a code address
    sub     r6, r6, r4      // Compute offset of data area
    add     r6, r6, r9      // Build address based on the loaded data address
    b       address_built   // Finished building address
flash_area:
    sub     r6, r6, r3      // Compute offset of code area
    add     r6, r6, r5      // Build address based on the loaded code address
address_built:
    str     r6, [r1]        // Store in new GOT table
    add     r0, r0, #4      // Move to next entry
    add     r1, r1, #4      // 
    b       new_got_setup   // Continue at the top of the loop
got_setup_done:


    LDMIA   sp!, {r3, r4, r5, r6, r7, lr}       // Store other preserved registers
    bx      lr                                  // Return to caller
  
    .align 4
