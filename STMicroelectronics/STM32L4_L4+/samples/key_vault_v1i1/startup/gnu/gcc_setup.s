
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


    /* Copy initialised sections into RAM if required. */
  
    ldr     r0, =__data_load_start__
    sub     r0,r0,r3
    add     r0,r0,r5
    ldr     r1, =__data_start__
    sub     r1,r1, r4
    add     r1,r1,r9
    ldr     r2, =__data_end__
    sub     r2,r2,r4
    add     r2,r2,r9
    bl      crt0_memory_copy

    /* Relocate addressess in the RAM section. */

    ldr     r0, =__reldyn_load_start__
    sub     r0,r0,r3
    add     r0,r0,r5
    ldr     r1, =__reldyn_end__
    sub     r1,r1,r3
    add     r1,r1,r5
relocation_loop:
    cmp     r0, r1                    // Loop over the rel.dyn table.
    beq     relocation_done
    ldr     r2, [r0, #4]              // Table is composed by [offiset, info].
    cmp     r2, #23                   // Look for info = R_ARM_RELATIVE (23)
    bne     relocation_not_rel        // Ignore all non-R_ARM_RELATIVE.
    ldr     r2, [r0]                  // Is the memory to relocate in the code or data area?
    cmp     r2, r4                    // If less than, it is a code address
    blt     relocation_not_rel        // GOT will take care of all code addressess
    sub     r2,r2,r4
    add     r2,r2,r9
    ldr     r6, [r2]                  // Get the offset in the address.
    cmp     r6, r4                    // Is it in the code or data area?
    blt     relocation_flash_area     // If less than, it is a code address
    sub     r6, r6, r4                // Compute offset of data area
    add     r6, r6, r9                // Build address based on the loaded data address
    b       relocation_address_built  // Finished building address
relocation_flash_area:
    sub     r6, r6, r3                // Compute offset of code area
    add     r6, r6, r5                // Build address based on the loaded code address
relocation_address_built:
    str     r6, [r2]                  // Replace the offset by an absolute address
relocation_not_rel:
    add     r0, r0, #8                // Go to the next node in the rel.dyn table.
    b       relocation_loop
relocation_done:

    /* Zero bss. */
    
    ldr     r0, =__bss_start__
    sub     r0,r0,r4
    add     r0,r0,r9
    ldr     r1, =__bss_end__
    sub     r1,r1,r4
    add     r1,r1,r9
    mov     r2, #0
    bl      crt0_memory_set


    /* Setup heap - not recommended for Threadx but here for compatibility reasons */

    ldr     r0, =__heap_start__
    sub     r0,r0,r4
    add     r0,r0,r9
    ldr     r1, =__heap_end__
    sub     r1,r1,r4
    add     r1,r1,r9
    sub     r1,r1,r0
    mov     r2, #0
    str     r2, [r0]
    add     r0, r0, #4
    str     r1, [r0]
    
    LDMIA   sp!, {r3, r4, r5, r6, r7, lr}       // Store other preserved registers
    bx      lr                                  // Return to caller
  
    .align 4

  /* Startup helper functions. */

    .thumb_func
crt0_memory_copy:

    cmp     r0, r1
    beq     memory_copy_done
    cmp     r2, r1
    beq     memory_copy_done
    sub     r2, r2, r1
memory_copy_loop:
    ldrb    r6, [r0]
    add     r0, r0, #1
    strb    r6, [r1]
    add     r1, r1, #1
    sub     r2, r2, #1
    cmp     r2, #0
    bne     memory_copy_loop
memory_copy_done:
    bx      lr

    .thumb_func
crt0_memory_set:
    cmp     r0, r1
    beq     memory_set_done
    strb    r2, [r0]
    add     r0, r0, #1
    b       crt0_memory_set
memory_set_done:
    bx      lr

    /* Setup attibutes of heap section so it doesn't take up room in the elf file */
    .section .heap, "wa", %nobits
  