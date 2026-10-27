        @
        @ Definitions shared by the JSCLI modules. The ArcEm_JSCLI protocol
        @ is described in arculator-wasm/src/jscli.c
        @

        @ Register naming
        wp .req r12

        @ ARM constants
        V_FLAG = 1 << 28
        I_FLAG = 1 << 27

        @ RISC OS constants
        XOS_CLI               = 0x20005
        XOS_Byte              = 0x20006
        XOS_Exit              = 0x20011
        XOS_Module            = 0x2001e
        XOS_Claim             = 0x2001f
        XOS_Release           = 0x20020
        XOS_ReadMonotonicTime = 0x20042
        XOS_AddCallBack       = 0x20054
        XOS_RemoveCallBack    = 0x2005f
        XOS_ConvertHex8       = 0x200d4

        Module_Enter = 2
        Module_Claim = 6
        Module_Free  = 7
        Module_Lookup = 18

        WrchV   = 0x03
        TickerV = 0x1c

        Service_Reset = 0x27

        @ ArcEm_JSCLI, trapped by the emulator
        XArcEm_JSCLI = 0x76ac5

        JSCLI_Register   = 0
        JSCLI_GetCommand = 1
        JSCLI_Output     = 2
        JSCLI_Done       = 3

        @ Kinds of request, returned by JSCLI_GetCommand in r1
        JSCLI_KIND_CLI  = 0             @ * command, NUL terminated
        JSCLI_KIND_KEYS = 1             @ bytes for the keyboard buffer
        JSCLI_KIND_OPEN  = 2            @ start a session: command, NUL terminated
        JSCLI_KIND_INPUT = 3            @ bytes for session r2's TaskWindow
        JSCLI_KIND_CLOSE = 4            @ kill session (or command) r2's TaskWindow

        JSCLI_CHANNEL_CALLBACK = 0
        JSCLI_CHANNEL_WIMP     = 1

        JSCLI_MAGIC = 0x434c4900        @ top 24 bits of the pending word
        JSCLI_ACK   = 0x4a53434c        @ returned by JSCLI_Register

        CMD_BUF_SIZE = 256
