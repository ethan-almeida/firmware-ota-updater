
  .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

.global g_pfnVectors
.global Default_Handler

  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object

g_pfnVectors:
  .word _estack              /* Initial Main Stack Pointer */
  .word Reset_Handler        /* Reset Handler */
  .word NMI_Handler          /* NMI Handler */
  .word HardFault_Handler    /* Hard Fault Handler */
  .word MemManage_Handler    /* MPU Fault Handler */
  .word BusFault_Handler     /* Bus Fault Handler */
  .word UsageFault_Handler   /* Usage Fault Handler */
  .word 0                    /* Reserved */
  .word 0                    /* Reserved */
  .word 0                    /* Reserved */
  .word 0                    /* Reserved */
  .word SVC_Handler          /* SVCall Handler */
  .word DebugMon_Handler     /* Debug Monitor Handler */
  .word 0                    /* Reserved */
  .word PendSV_Handler       /* PendSV Handler */
  .word SysTick_Handler      /* SysTick Handler */
  .size g_pfnVectors, .-g_pfnVectors


  .section .text.Reset_Handler
  .weak Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:
  ldr   sp, =_estack
  ldr   r0, =_sdata
  ldr   r1, =_edata
  ldr   r2, =_sidata
  movs  r3, #0
  b     LoopCopyDataInit

CopyDataInit:
  ldr   r4, [r2, r3]
  str   r4, [r0, r3]
  adds  r3, r3, #4

LoopCopyDataInit:
  adds  r4, r0, r3
  cmp   r4, r1
  bcc   CopyDataInit


  ldr   r2, =_sbss
  ldr   r4, =_ebss
  movs  r3, #0
  b     LoopFillZerobss

FillZerobss:
  str   r3, [r2]
  adds  r2, r2, #4

LoopFillZerobss:
  cmp   r2, r4
  bcc   FillZerobss

  bl    main
  b     .                    

  .size Reset_Handler, .-Reset_Handler


  .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b     Infinite_Loop
  .size Default_Handler, .-Default_Handler


  .weak NMI_Handler
  .thumb_set NMI_Handler, Default_Handler
  .weak HardFault_Handler
  .thumb_set HardFault_Handler, Default_Handler
  .weak MemManage_Handler
  .thumb_set MemManage_Handler, Default_Handler
  .weak BusFault_Handler
  .thumb_set BusFault_Handler, Default_Handler
  .weak UsageFault_Handler
  .thumb_set UsageFault_Handler, Default_Handler
  .weak SVC_Handler
  .thumb_set SVC_Handler, Default_Handler
  .weak DebugMon_Handler
  .thumb_set DebugMon_Handler, Default_Handler
  .weak PendSV_Handler
  .thumb_set PendSV_Handler, Default_Handler
  .weak SysTick_Handler
  .thumb_set SysTick_Handler, Default_Handler
