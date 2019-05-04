

        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8

		EXPORT	OS_SVC_Id
        EXPORT  OS_SVC_Sleep
		EXPORT	OS_SVC_Kill
		EXPORT	OS_SVC_Time
		EXPORT	OS_SVC_AddThread
        EXPORT  OS_SVC_Suspend
        EXPORT  OS_SVC_Wait
        EXPORT  OS_SVC_bWait
        EXPORT  OS_SVC_Signal
        EXPORT  OS_SVC_bSignal
        EXPORT  OS_SVC_Heap_Malloc
        EXPORT  OS_SVC_Heap_Free
        EXPORT  OS_SVC_AddProcess


OS_SVC_Id
	SVC		#0
	BX		LR

OS_SVC_Kill
	SVC		#1
	BX		LR

OS_SVC_Sleep
	SVC		#2
	BX		LR

OS_SVC_Time
	SVC		#3
	BX		LR

OS_SVC_AddThread
    ; Verify pointer is accessible
    PUSH {R0}
    LDR R0, [R0]
    POP {R0}

	SVC		#4
	BX		LR

OS_SVC_Suspend
	SVC		#5
	BX		LR

OS_SVC_Wait
    ; Verify pointer is accessible
    PUSH {R0}
    LDR R0, [R0]
    POP {R0}

	SVC		#6
	BX		LR

OS_SVC_bWait
    ; Verify pointer is accessible
    PUSH {R0}
    LDR R0, [R0]
    POP {R0}

	SVC		#7
	BX		LR

OS_SVC_Signal
    ; Verify pointer is accessible
    PUSH {R0}
    LDR R0, [R0]
    POP {R0}

	SVC		#8
	BX		LR

OS_SVC_bSignal
    ; Verify pointer is accessible
    PUSH {R0}
    LDR R0, [R0]
    POP {R0}

	SVC		#9
	BX		LR

OS_SVC_Heap_Malloc
	SVC		#10
	BX		LR

OS_SVC_Heap_Free
    ; Verify pointer is accessible
    PUSH {R0}
    LDR R0, [R0]
    POP {R0}

	SVC		#11
	BX		LR

OS_SVC_AddProcess
    ; Verify pointers are accessible
    PUSH {R0,R1,R2}
    LDR R0, [R0]
    LDR R1, [R1]
    LDR R2, [R2]
    POP {R0,R1,R2}

	SVC		#12
	BX		LR


    ALIGN
    END
