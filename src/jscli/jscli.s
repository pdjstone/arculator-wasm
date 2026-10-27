        @
        @ JSCLI - run * commands for the Archimedes Live frontend
        @
        @ A TickerV handler checks the pending word the emulator sets when a
        @ request is queued, and adds a transient callback. The callback runs
        @ a command with OS_CLI while WrchV is claimed to capture what it
        @ prints (which then doesn't reach the screen), then reports the
        @ output and any error to the emulator.
        @
        @ A request can instead be keys, put in the keyboard buffer with
        @ OS_Byte 153 as if typed. When the keyboard buffer is full, the rest
        @ of the keys wait for later callbacks.
        @
        @ The callback runs in whatever context RISC OS was about to return
        @ to, which in the desktop can be any task.
        @
        @ 26-bit only: the TickerV handler switches mode with TEQP.
        @

        .include "jscli-defs.s"

        OUT_BUF_SIZE = 256

        STATE_IDLE    = 0
        STATE_QUEUED  = 1               @ callback added
        STATE_RUNNING = 2               @ callback running a command
        STATE_TYPING  = 3               @ keys left over from a full buffer

        WS_PENDING = 0                  @ written by the emulator
        WS_STATE   = 4
        WS_CMD_ID  = 8
        WS_OUT_LEN = 12
        WS_KEY_POS = 16                 @ next key to put in the buffer
        WS_KEY_LEN = 20                 @ keys in WS_CMD_BUF, or 0
        WS_CMD_BUF = 32
        WS_OUT_BUF = WS_CMD_BUF + CMD_BUF_SIZE
        WORKSPACE_SIZE = WS_OUT_BUF + OUT_BUF_SIZE



        .global _start

_start:

module_start:

        .int    0               @ Start
        .int    init            @ Initialisation
        .int    final           @ Finalisation
        .int    service         @ Service Call
        .int    modtitle        @ Title String
        .int    help            @ Help String
        .int    0               @ Help and Command keyword table
        .int    0               @ SWI Chunk base
        .int    0               @ SWI handler code
        .int    0               @ SWI decoding table
        .int    0               @ SWI decoding code

modtitle:
        .string "JSCLI"

help:
        .string "JSCLI\t\t0.01 (20 Oct 2026) Runs commands for Archimedes Live"
        .align

err_not_emulator:
        .int    0x1
        .string "JSCLI needs the Archimedes Live emulator"
        .align

err_busy:
        .int    0x2
        .string "JSCLI is running a command"
        .align



        @ r0-r6 may be corrupted, r11 holds the private word pointer
init:
        stmfd   sp!, {r7-r11, lr}
        mov     r11, r12

        @ See if we need to claim some workspace
        ldr     r2, [r11]
        teq     r2, #0
        bne     1f

        mov     r0, #Module_Claim
        ldr     r3, =WORKSPACE_SIZE
        swi     XOS_Module
        ldmvsfd sp!, {r7-r11, pc}       @ no memory claimed then refuse to initialise
        str     r2, [r11]
1:
        mov     wp, r2
        ldr     r0, =JSCLI_MAGIC
        str     r0, [wp, #WS_PENDING]
        mov     r0, #STATE_IDLE
        str     r0, [wp, #WS_STATE]
        str     r0, [wp, #WS_KEY_LEN]

        bl      register
        bvs     init_failed

        mov     r0, #TickerV
        adr     r1, ticker
        mov     r2, wp
        swi     XOS_Claim
        bvs     init_failed_deregister

        ldmfd   sp!, {r7-r11, pc}

init_failed_deregister:
        mov     r7, r0
        bl      deregister
        mov     r0, r7
init_failed:
        mov     r7, r0
        mov     r0, #Module_Free
        mov     r2, wp
        swi     XOS_Module
        mov     r0, #0
        str     r0, [r11]
        mov     r0, r7
        ldmfd   sp!, {r7-r11, lr}
        orrs    pc, lr, #V_FLAG



final:
        stmfd   sp!, {r7-r11, lr}
        mov     r11, r12
        ldr     wp, [r11]

        @ Don't die under a command we're running (it may be *RMKill JSCLI)
        ldr     r0, [wp, #WS_STATE]
        teq     r0, #STATE_RUNNING
        adreq   r0, err_busy
        ldmeqfd sp!, {r7-r11, lr}
        orreqs  pc, lr, #V_FLAG

        @ Stop the emulator writing to our workspace
        bl      deregister

        mov     r0, #TickerV
        adr     r1, ticker
        mov     r2, wp
        swi     XOS_Release

        ldr     r0, [wp, #WS_STATE]
        teq     r0, #STATE_QUEUED
        adreq   r0, callback
        moveq   r1, wp
        swieq   XOS_RemoveCallBack

        @ Free the workspace and forget it, so that a re-initialisation
        @ (e.g. by RMTidy) claims new workspace rather than reusing this
        mov     r0, #Module_Free
        mov     r2, wp
        swi     XOS_Module
        mov     r0, #0
        str     r0, [r11]

        @ Clear V flag so our module will die
        cmp     pc, #0
        ldmfd   sp!, {r7-r11, pc}



        @ Vectors are reset by a soft reset, and any callback was lost
service:
        teq     r1, #Service_Reset
        movne   pc, lr

        stmfd   sp!, {r0-r2, lr}
        ldr     wp, [r12]
        mov     r0, #STATE_IDLE
        str     r0, [wp, #WS_STATE]
        str     r0, [wp, #WS_KEY_LEN]
        bl      register                @ fails any command we were running
        mov     r0, #TickerV
        adr     r1, ticker
        mov     r2, wp
        swi     XOS_Claim
        ldmfd   sp!, {r0-r2, pc}



        @ Register with the emulator. Exit: V set and r0 = error on failure.
        @ Corrupts r0-r2
register:
        stmfd   sp!, {lr}
        mov     r0, #JSCLI_Register
        mov     r1, #JSCLI_CHANNEL_CALLBACK
        add     r2, wp, #WS_PENDING
        swi     XArcEm_JSCLI            @ not known without the emulator
        ldmvsfd sp!, {pc}
        ldr     r1, =JSCLI_ACK
        teq     r0, r1
        ldmeqfd sp!, {pc}
        adr     r0, err_not_emulator
        ldmfd   sp!, {lr}
        orrs    pc, lr, #V_FLAG

        @ Corrupts r0-r2
deregister:
        mov     r0, #JSCLI_Register
        mov     r1, #JSCLI_CHANNEL_CALLBACK
        mov     r2, #0
        swi     XArcEm_JSCLI
        mov     pc, lr



        @ TickerV: IRQ mode, r12 = workspace. Preserves all registers.
        @ Adds the callback when a request is waiting, or keys are left over
ticker:
        stmfd   sp!, {r0-r2, lr}
        ldr     r0, [wp, #WS_STATE]
        teq     r0, #STATE_TYPING
        beq     1f
        teq     r0, #STATE_IDLE
        bne     ticker_exit
        ldr     r0, [wp, #WS_PENDING]
        tst     r0, #0xff
        beq     ticker_exit
1:

        mov     r0, #STATE_QUEUED
        str     r0, [wp, #WS_STATE]

        @ Switch to SVC mode to call OS_AddCallBack, keeping SVC's lr
        mov     r2, pc
        orr     r0, r2, #3
        teqp    r0, #0
        mov     r0, r0
        stmfd   sp!, {lr}
        adr     r0, callback
        mov     r1, wp
        swi     XOS_AddCallBack
        ldmfd   sp!, {lr}
        teqp    r2, #0
        mov     r0, r0
ticker_exit:
        ldmfd   sp!, {r0-r2, pc}^       @ pass on, with the flags we had



        @ Transient callback: SVC mode, IRQs disabled, r12 = workspace.
        @ Preserves all registers
callback:
        stmfd   sp!, {r0-r6, lr}

        @ Commands can take a while, so let interrupts in
        mov     r0, pc
        bic     r0, r0, #I_FLAG
        teqp    r0, #0

        ldr     r0, [wp, #WS_KEY_LEN]
        teq     r0, #0
        bne     type_keys

        mov     r0, #JSCLI_GetCommand
        mov     r1, #JSCLI_CHANNEL_CALLBACK
        add     r2, wp, #WS_CMD_BUF
        mov     r3, #CMD_BUF_SIZE
        mov     r4, #(1 << JSCLI_KIND_CLI) | (1 << JSCLI_KIND_KEYS)
        swi     XArcEm_JSCLI            @ r0 = id, r1 = kind, r3 = length
        teq     r0, #0
        beq     callback_exit

        str     r0, [wp, #WS_CMD_ID]
        mov     r0, #0
        str     r0, [wp, #WS_OUT_LEN]
        mov     r0, #STATE_RUNNING
        str     r0, [wp, #WS_STATE]

        teq     r1, #JSCLI_KIND_KEYS
        beq     start_keys

        mov     r0, #WrchV
        adr     r1, wrch
        mov     r2, wp
        swi     XOS_Claim
        movvs   r4, r0
        bvs     callback_done

        add     r0, wp, #WS_CMD_BUF
        swi     XOS_CLI
        movvc   r4, #0                  @ r4 = error block or 0
        movvs   r4, r0

        bl      flush_output
        mov     r0, #WrchV
        adr     r1, wrch
        mov     r2, wp
        swi     XOS_Release

        b       callback_done

start_keys:
        str     r3, [wp, #WS_KEY_LEN]
        mov     r0, #0
        str     r0, [wp, #WS_KEY_POS]
        teq     r3, #0
        beq     keys_done

        @ Put keys in the buffer until they run out or it's full. OS_Byte 153
        @ treats the Escape character like the Escape key
type_keys:
        ldr     r4, [wp, #WS_KEY_POS]
        ldr     r5, [wp, #WS_KEY_LEN]
        add     r6, wp, #WS_CMD_BUF
1:
        cmp     r4, r5
        bge     keys_done
        mov     r0, #153
        mov     r1, #0                  @ keyboard buffer
        ldrb    r2, [r6, r4]
        swi     XOS_Byte
        bvs     keys_failed
        bcs     keys_full
        add     r4, r4, #1
        b       1b

        @ Wait for a later callback (see ticker) to try again
keys_full:
        str     r4, [wp, #WS_KEY_POS]
        mov     r0, #STATE_TYPING
        str     r0, [wp, #WS_STATE]
        ldmfd   sp!, {r0-r6, pc}^       @ restores IRQs disabled

keys_failed:
        mov     r4, r0
        b       1f
keys_done:
        mov     r4, #0
1:
        mov     r0, #0
        str     r0, [wp, #WS_KEY_LEN]

callback_done:
        mov     r0, #JSCLI_Done
        ldr     r1, [wp, #WS_CMD_ID]
        mov     r2, r4
        swi     XArcEm_JSCLI

callback_exit:
        mov     r0, #STATE_IDLE
        str     r0, [wp, #WS_STATE]
        ldmfd   sp!, {r0-r6, pc}^       @ restores IRQs disabled



        @ WrchV: r0 = character. Records it and claims the call, so the
        @ output doesn't reach the screen. In the desktop it would be drawn
        @ over the windows, or open a command window for whichever task the
        @ callback interrupted
wrch:
        stmfd   sp!, {r0-r3, lr}
        ldr     r1, [wp, #WS_OUT_LEN]
        add     r2, wp, #WS_OUT_BUF
        strb    r0, [r2, r1]
        add     r1, r1, #1
        str     r1, [wp, #WS_OUT_LEN]
        cmp     r1, #OUT_BUF_SIZE
        bleq    flush_output
        ldmfd   sp!, {r0-r3, lr}
        cmp     r0, r0                  @ clear V: no error
        ldmfd   sp!, {pc}               @ claim: return to OS_WriteC's caller



        @ Send the output buffer to the emulator. Corrupts r0-r3
flush_output:
        ldr     r3, [wp, #WS_OUT_LEN]
        teq     r3, #0
        moveq   pc, lr
        mov     r0, #JSCLI_Output
        ldr     r1, [wp, #WS_CMD_ID]
        add     r2, wp, #WS_OUT_BUF
        swi     XArcEm_JSCLI            @ trapped by the emulator, so SVC's lr survives
        mov     r0, #0
        str     r0, [wp, #WS_OUT_LEN]
        mov     pc, lr

        .ltorg
