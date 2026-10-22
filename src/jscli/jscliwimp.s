        @
        @ JSCLIWimp - run * commands for the Archimedes Live frontend in the
        @ desktop
        @
        @ A Wimp task that asks the emulator for a request on every null
        @ event, and runs commands in TaskWindows. Asking rather than checking
        @ the pending word lets the emulator tell that the task is still
        @ polling, i.e. the desktop isn't suspended. A TaskWindow sends what
        @ the command prints back as Message_TaskWindow_Output, which is
        @ passed on to the emulator, and Message_TaskWindow_Morio when it has
        @ finished.
        @
        @ Each TaskWindow has a slot. Commands run one at a time, but
        @ sessions (TaskWindows that stay open and are sent input) run
        @ alongside them. A request's id is its TaskWindow's -txt handle, so
        @ Message_TaskWindow_Ego tells us which slot a new TaskWindow is for.
        @
        @ The task starts with the desktop. If the module is loaded after
        @ that, start it with *WimpTask Desktop_JSCLIWimp
        @

        .include "jscli-defs.s"

        XWimp_Initialise  = 0x600c0
        XWimp_CloseDown   = 0x600dd
        XWimp_StartTask   = 0x600de
        XWimp_PollIdle    = 0x600e1
        XWimp_SendMessage = 0x600e7

        Service_StartWimp   = 0x49
        Service_StartedWimp = 0x4a

        Message_Quit              = 0
        Message_TaskWindow_Input  = 0x808c0
        Message_TaskWindow_Output = 0x808c1
        Message_TaskWindow_Ego    = 0x808c2
        Message_TaskWindow_Morio  = 0x808c3
        Message_TaskWindow_Morite = 0x808c4

        Wimp_Null                 = 0
        Wimp_UserMessage          = 17
        Wimp_UserMessageRecorded  = 18
        Wimp_UserMessageAcknowledge = 19

        WIMP_VERSION   = 300
        WIMP_POLL_MASK = 0x00000030     @ no Pointer Entering or Leaving events
        POLL_INTERVAL  = 5              @ centiseconds between null events
        START_TIMEOUT  = 300            @ centiseconds for a TaskWindow to say hello

        START_BUF_SIZE = 256            @ OS_CLI's limit
        WIMP_BLOCK_SIZE = 256
        STACK_SIZE = 256

        @ A TaskWindow slot
        SLOT_ID    = 0                  @ request id, or 0 if the slot is free
        SLOT_KIND  = 4                  @ JSCLI_KIND_CLI or JSCLI_KIND_OPEN
        SLOT_CHILD = 8                  @ TaskWindow's task handle, once known
        SLOT_START = 12                 @ when the TaskWindow was started
        SLOT_SIZE  = 16
        MAX_SLOTS  = 8                  @ JSCLI_MAX_RUNNING in jscli.c

        @ Flags for the slots' state, in null_event
        SLOTS_FREE     = 1
        SLOTS_COMMAND  = 2              @ a command is running
        SLOTS_STARTING = 4              @ a TaskWindow hasn't said hello yet

        MSG_HEADER = 20                 @ Wimp message header size
        MAX_INPUT  = 232                @ JSCLI_MAX_INPUT in jscli.c

        WS_PENDING   = 0                @ written by the emulator, not used
        WS_TASK      = 4                @ 0 inactive, -1 starting, else task handle
        WS_REQ_ID    = 8                @ the request from JSCLI_GetCommand
        WS_REQ_KIND  = 12
        WS_REQ_TARGET = 16
        WS_REQ_LEN   = 20
        WS_SLOTS     = 24
        @ The request is read into a message block, so input can be sent
        @ from where it is: the data of Message_TaskWindow_Input is at +24
        WS_MSG       = WS_SLOTS + SLOT_SIZE * MAX_SLOTS
        WS_CMD_BUF   = WS_MSG + MSG_HEADER + 4
        WS_START_BUF = WS_CMD_BUF + CMD_BUF_SIZE
        WS_WIMP_BLOCK = WS_START_BUF + START_BUF_SIZE * 2
        WS_STACK_TOP = WS_WIMP_BLOCK + WIMP_BLOCK_SIZE + STACK_SIZE
        WORKSPACE_SIZE = WS_STACK_TOP



        .global _start

_start:

module_start:

        .int    start           @ Start
        .int    init            @ Initialisation
        .int    final           @ Finalisation
        .int    service         @ Service Call
        .int    modtitle        @ Title String
        .int    help            @ Help String
        .int    table           @ Help and Command keyword table
        .int    0               @ SWI Chunk base
        .int    0               @ SWI handler code
        .int    0               @ SWI decoding table
        .int    0               @ SWI decoding code

modtitle:
        .string "JSCLIWimp"

help:
        .string "JSCLIWimp\t0.01 (20 Oct 2026) Runs commands for Archimedes Live in TaskWindows"
        .align


        @ Help and Command keyword table
table:
desktop_jscliwimp:
        .string "Desktop_JSCLIWimp"
        .align
        .int    command_desktop_jscliwimp
        .int    0x00000000
        .int    0
        .int    command_desktop_jscliwimp_help

        .byte   0       @ Table terminator

command_desktop_jscliwimp_help:
        .string "Starts the JSCLIWimp task. This is done when the desktop starts; if the module is loaded later, use *WimpTask Desktop_JSCLIWimp"
        .align

err_too_long:
        .int    0x1
        .string "Command too long for a TaskWindow"
        .align

err_no_start:
        .int    0x2
        .string "The TaskWindow didn't start"
        .align

err_no_session:
        .int    0x3
        .string "The session has ended"
        .align

taskwindow_name:
        .string "TaskWindow"
        .align



init:
        stmfd   sp!, {lr}

        @ See if we need to claim some workspace
        ldr     r2, [r12]
        teq     r2, #0
        bne     1f

        mov     r0, #Module_Claim
        ldr     r3, =WORKSPACE_SIZE
        swi     XOS_Module
        ldmvsfd sp!, {pc}       @ no memory claimed then refuse to initialise
        str     r2, [r12]
1:
        mov     wp, r2
        ldr     r0, =JSCLI_MAGIC
        str     r0, [wp, #WS_PENDING]
        mov     r0, #0
        str     r0, [wp, #WS_TASK]
        bl      clear_slots
        ldmfd   sp!, {pc}



final:
        stmfd   sp!, {r11, lr}
        mov     r11, r12
        ldr     wp, [r11]

        bl      deregister

        @ Close Wimp task if active
        ldr     r0, [wp, #WS_TASK]
        cmp     r0, #0
        ldrgt   r1, TASK
        swigt   XWimp_CloseDown

        @ Free the workspace and forget it, so that a re-initialisation
        @ (e.g. by RMTidy) claims new workspace rather than reusing this
        mov     r0, #Module_Free
        mov     r2, wp
        swi     XOS_Module
        mov     r0, #0
        str     r0, [r11]

        @ Clear V flag so our module will die
        cmp     pc, #0
        ldmfd   sp!, {r11, pc}



service:
        teq     r1, #Service_Reset
        teqne   r1, #Service_StartWimp
        teqne   r1, #Service_StartedWimp
        movne   pc, lr

        stmfd   sp!, {lr}
        ldr     wp, [r12]

        teq     r1, #Service_StartWimp
        beq     service_startwimp
        teq     r1, #Service_StartedWimp
        beq     service_startedwimp

        @ Service_Reset: the Wimp, and our task, are gone
        stmfd   sp!, {r0-r2}
        mov     r0, #0
        str     r0, [wp, #WS_TASK]
        bl      clear_slots
        bl      deregister
        ldmfd   sp!, {r0-r2, pc}

service_startwimp:
        ldr     r14, [wp, #WS_TASK]
        teq     r14, #0                 @ Am I already active?
        moveq   r14, #-1                @ No, so set handle to -1
        streq   r14, [wp, #WS_TASK]
        adreq   r0, desktop_jscliwimp   @ r0 points to command to start task
        moveq   r1, #0                  @ claim the service
        ldmfd   sp!, {pc}

service_startedwimp:
        @ Zero the Task Handle if it is still -1
        ldr     r14, [wp, #WS_TASK]
        cmp     r14, #-1
        moveq   r14, #0
        streq   r14, [wp, #WS_TASK]
        ldmfd   sp!, {pc}



command_desktop_jscliwimp:
        stmfd   sp!, {lr}
        ldr     r2, [r12]
        ldr     r2, [r2, #WS_TASK]
        cmp     r2, #0
        ldmgtfd sp!, {pc}               @ already running
        mov     r2, r0
        adr     r1, modtitle
        mov     r0, #Module_Enter
        swi     XOS_Module
        ldmfd   sp!, {pc}



        @ Corrupts r0-r2
clear_slots:
        add     r1, wp, #WS_SLOTS
        mov     r2, #MAX_SLOTS
        mov     r0, #0
1:
        str     r0, [r1], #SLOT_SIZE
        subs    r2, r2, #1
        bne     1b
        mov     pc, lr



        @ Corrupts r0-r2
deregister:
        mov     r0, #JSCLI_Register
        mov     r1, #JSCLI_CHANNEL_WIMP
        mov     r2, #0
        swi     XArcEm_JSCLI
        mov     pc, lr



TASK:
        .ascii  "TASK"

task_name:
        .string "JSCLI"
        .align

message_list:
        .int    Message_TaskWindow_Output
        .int    Message_TaskWindow_Ego
        .int    Message_TaskWindow_Morio
        .int    0

str_taskwindow:
        .string "TaskWindow"
str_quit:
        .string "\" -quit"
str_options:
        .string " -name JSCLI -task &"
str_txt:
        .string " -txt &"
        .align



        @ "Start" entry point
        @ Entered in User Mode, with no stack
        @ Therefore no need to preserve link register before calling SWIs
start:
        ldr     wp, [r12]               @ Get workspace pointer
        add     sp, wp, #WS_STACK_TOP
        ldr     r0, [wp, #WS_TASK]
        cmp     r0, #0                  @ Am I already active?
        ble     1f                      @ No then skip following instructions
        ldr     r1, TASK                @ Yes, so close down first
        swi     XWimp_CloseDown
        mov     r0, #0                  @ Mark as inactive
        str     r0, [wp, #WS_TASK]
1:
        mov     r0, #WIMP_VERSION
        ldr     r1, TASK
        adr     r2, task_name
        adr     r3, message_list
        swi     XWimp_Initialise
        swivs   XOS_Exit                @ Exit if error
        str     r1, [wp, #WS_TASK]

        bl      clear_slots
        ldr     r0, =JSCLI_MAGIC
        str     r0, [wp, #WS_PENDING]
        mov     r0, #JSCLI_Register
        mov     r1, #JSCLI_CHANNEL_WIMP
        add     r2, wp, #WS_PENDING
        swi     XArcEm_JSCLI
        bvs     close_down


        @ Main poll loop
re_poll:
        swi     XOS_ReadMonotonicTime   @ returns time in r0
        add     r2, r0, #POLL_INTERVAL
        mov     r0, #WIMP_POLL_MASK
        add     r1, wp, #WS_WIMP_BLOCK  @ point to Wimp block within workspace
        swi     XWimp_PollIdle
        bvs     close_down

        teq     r0, #Wimp_Null
        beq     null_event
        teq     r0, #Wimp_UserMessage
        teqne   r0, #Wimp_UserMessageRecorded
        beq     user_message
        b       re_poll


null_event:
        @ What can we take? Give up on TaskWindows that haven't started
        mov     r7, #0
        add     r5, wp, #WS_SLOTS
        mov     r6, #MAX_SLOTS
1:
        ldr     r0, [r5, #SLOT_ID]
        teq     r0, #0
        orreq   r7, r7, #SLOTS_FREE
        beq     3f
        ldr     r0, [r5, #SLOT_CHILD]
        teq     r0, #0
        bne     2f
        @ If a TaskWindow fails to start, the Wimp reports the error itself
        @ and we hear nothing, so give up if it hasn't said hello
        swi     XOS_ReadMonotonicTime
        ldr     r1, [r5, #SLOT_START]
        sub     r0, r0, r1
        cmp     r0, #START_TIMEOUT
        orrlt   r7, r7, #SLOTS_STARTING
        blt     2f
        adrl    r2, err_no_start
        bl      slot_done
        orr     r7, r7, #SLOTS_FREE
        b       3f
2:
        ldr     r0, [r5, #SLOT_KIND]
        teq     r0, #JSCLI_KIND_CLI
        orreq   r7, r7, #SLOTS_COMMAND
3:
        add     r5, r5, #SLOT_SIZE
        subs    r6, r6, #1
        bne     1b

        @ Input and close wait until every TaskWindow has started, so the
        @ one they're for has a task handle to send to
        tst     r7, #SLOTS_STARTING
        moveq   r4, #(1 << JSCLI_KIND_INPUT) | (1 << JSCLI_KIND_CLOSE)
        movne   r4, #0
        tst     r7, #SLOTS_FREE
        beq     4f
        orr     r4, r4, #1 << JSCLI_KIND_OPEN
        tst     r7, #SLOTS_COMMAND
        orreq   r4, r4, #1 << JSCLI_KIND_CLI
4:
        @ Ask even if we can't take anything: it's our heartbeat
        mov     r0, #JSCLI_GetCommand
        mov     r1, #JSCLI_CHANNEL_WIMP
        add     r2, wp, #WS_CMD_BUF
        mov     r3, #CMD_BUF_SIZE
        swi     XArcEm_JSCLI
        teq     r0, #0
        beq     re_poll
        add     r5, wp, #WS_REQ_ID
        stmia   r5, {r0-r3}
        teq     r1, #JSCLI_KIND_INPUT
        beq     send_input
        teq     r1, #JSCLI_KIND_CLOSE
        beq     send_close

        @ A command or a session: start a TaskWindow in a free slot
        mov     r0, #0
        mov     r2, #SLOT_ID
        bl      find_slot
        ldr     r0, [wp, #WS_REQ_ID]
        str     r0, [r5, #SLOT_ID]
        str     r1, [r5, #SLOT_KIND]
        mov     r0, #0
        str     r0, [r5, #SLOT_CHILD]
        swi     XOS_ReadMonotonicTime
        str     r0, [r5, #SLOT_START]

        @ RISC OS 2 only has the TaskWindow module if something like !Edit
        @ has loaded it
        mov     r0, #Module_Lookup
        adrl    r1, taskwindow_name
        swi     XOS_Module
        blvc    build_start_command
        swivc   XWimp_StartTask
        movvs   r2, r0
        blvs    slot_done
        b       re_poll


        @ Send the request's data to its target's TaskWindow
send_input:
        bl      find_target
        bne     request_failed
        add     r1, wp, #WS_MSG
        ldr     r3, [wp, #WS_REQ_LEN]
        str     r3, [r1, #MSG_HEADER]
        add     r3, r3, #MSG_HEADER + 4 + 3
        bic     r3, r3, #3
        ldr     r2, =Message_TaskWindow_Input
        b       send_to_target

        @ Kill the request's target's TaskWindow. It replies with Morio,
        @ which finishes the target
send_close:
        bl      find_target
        bne     request_failed
        add     r1, wp, #WS_MSG
        mov     r3, #MSG_HEADER
        ldr     r2, =Message_TaskWindow_Morite

        @ r1 = message block, r2 = action, r3 = size, r5 = target's slot
send_to_target:
        str     r3, [r1, #0]
        mov     r0, #0
        str     r0, [r1, #12]           @ your_ref
        str     r2, [r1, #16]
        mov     r0, #Wimp_UserMessage
        ldr     r2, [r5, #SLOT_CHILD]
        swi     XWimp_SendMessage
        movvc   r0, #0

        @ r0 = error, or 0: report it as the request's result
request_failed:
        mov     r2, r0
        mov     r0, #JSCLI_Done
        ldr     r1, [wp, #WS_REQ_ID]
        swi     XArcEm_JSCLI
        b       re_poll


        @ Find the slot of the request's target
        @ Exit: Z set and r5 = slot if found, else r0 = error.
        @ Corrupts r2, r3, r6
find_target:
        stmfd   sp!, {lr}
        ldr     r0, [wp, #WS_REQ_TARGET]
        mov     r2, #SLOT_ID
        bl      find_slot
        adrnel  r0, err_no_session
        ldmfd   sp!, {pc}


        @ Find the slot whose word at offset r2 is r0
        @ Exit: Z set and r5 = slot if found. Corrupts r3, r6
find_slot:
        add     r5, wp, #WS_SLOTS
        mov     r6, #MAX_SLOTS
1:
        ldr     r3, [r5, r2]
        teq     r3, r0
        moveq   pc, lr
        add     r5, r5, #SLOT_SIZE
        subs    r6, r6, #1
        bne     1b
        teq     r6, #1                  @ clear Z
        mov     pc, lr


        @ The TaskWindow in slot r5 has finished, with error r2 or 0:
        @ report it and free the slot. Corrupts r0, r1
slot_done:
        mov     r0, #JSCLI_Done
        ldr     r1, [r5, #SLOT_ID]
        swi     XArcEm_JSCLI
        mov     r0, #0
        str     r0, [r5, #SLOT_ID]
        str     r0, [r5, #SLOT_CHILD]
        mov     pc, lr


        @ r1 = Wimp block
user_message:
        ldr     r2, [r1, #16]           @ message action
        teq     r2, #Message_Quit
        beq     close_down
        ldr     r3, =Message_TaskWindow_Output
        teq     r2, r3
        beq     taskwindow_output
        add     r3, r3, #1
        teq     r2, r3
        beq     taskwindow_ego
        add     r3, r3, #1
        teq     r2, r3
        beq     taskwindow_morio
        b       re_poll

        @ A TaskWindow has started: its text handle is its request's id
taskwindow_ego:
        mov     r4, r0
        ldr     r0, [r1, #20]
        mov     r2, #SLOT_ID
        bl      find_slot
        ldreq   r2, [r1, #4]
        streq   r2, [r5, #SLOT_CHILD]
        b       acknowledge

taskwindow_output:
        bl      from_child
        bne     acknowledge
        mov     r0, #JSCLI_Output
        add     r2, r1, #24
        ldr     r3, [r1, #20]
        ldr     r1, [r5, #SLOT_ID]
        swi     XArcEm_JSCLI
        add     r1, wp, #WS_WIMP_BLOCK
        b       acknowledge

taskwindow_morio:
        bl      from_child
        bne     acknowledge
        mov     r2, #0
        bl      slot_done
        add     r1, wp, #WS_WIMP_BLOCK
        b       acknowledge

        @ Acknowledge a recorded message, so the sender doesn't get it back.
        @ r4 = event code
acknowledge:
        teq     r4, #Wimp_UserMessageRecorded
        bne     re_poll
        ldr     r2, [r1, #8]            @ my_ref -> your_ref
        str     r2, [r1, #12]
        ldr     r2, [r1, #4]            @ sender
        mov     r0, #Wimp_UserMessageAcknowledge
        swi     XWimp_SendMessage
        b       re_poll


        @ Find the slot of the TaskWindow that sent the message in r1.
        @ Entry: r0 = event code. Exit: Z set and r5 = slot if found,
        @ r4 = event code. Corrupts r0, r2, r3, r6
from_child:
        mov     r4, r0
        ldr     r0, [r1, #4]
        mov     r2, #SLOT_CHILD
        b       find_slot


        @ Build the TaskWindow command for the request in WS_START_BUF.
        @ Quotes in the command are doubled. A session without a command
        @ gets a * prompt.
        @ Exit: r0 = command, or V set and r0 = error
build_start_command:
        stmfd   sp!, {lr}
        add     r1, wp, #WS_START_BUF
        adrl    r0, str_taskwindow
        bl      copy_string
        ldr     r2, [wp, #WS_REQ_KIND]
        teq     r2, #JSCLI_KIND_OPEN
        ldreqb  r2, [wp, #WS_CMD_BUF]
        teqeq   r2, #0
        beq     3f
        mov     r2, #' '
        strb    r2, [r1], #1
        mov     r2, #'"'
        strb    r2, [r1], #1
        add     r0, wp, #WS_CMD_BUF
1:
        ldrb    r2, [r0], #1
        teq     r2, #0
        beq     2f
        teq     r2, #'"'
        streqb  r2, [r1], #1
        strb    r2, [r1], #1
        b       1b
2:
        adrl    r0, str_quit
        bl      copy_string
3:
        adrl    r0, str_options
        bl      copy_string
        ldr     r0, [wp, #WS_TASK]
        mov     r2, #9
        swi     XOS_ConvertHex8         @ r1 -> terminator
        adrl    r0, str_txt
        bl      copy_string
        ldr     r0, [wp, #WS_REQ_ID]
        mov     r2, #9
        swi     XOS_ConvertHex8

        add     r0, wp, #WS_START_BUF
        sub     r1, r1, r0
        cmp     r1, #START_BUF_SIZE
        ldmltfd sp!, {pc}               @ V clear from the CMP
        adrl    r0, err_too_long
        ldmfd   sp!, {lr}
        orrs    pc, lr, #V_FLAG


        @ Copy the string at r0 to r1, leaving r1 at its terminator.
        @ Corrupts r0, r2
copy_string:
        ldrb    r2, [r0], #1
        strb    r2, [r1], #1
        teq     r2, #0
        bne     copy_string
        sub     r1, r1, #1
        mov     pc, lr


close_down:
        bl      deregister
        ldr     r0, [wp, #WS_TASK]
        cmp     r0, #0
        ldrgt   r1, TASK
        swigt   XWimp_CloseDown
        mov     r0, #0
        str     r0, [wp, #WS_TASK]
        swi     XOS_Exit

        .ltorg
