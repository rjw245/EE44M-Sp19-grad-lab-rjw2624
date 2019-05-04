

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
	SVC		#4
	BX		LR

OS_SVC_Suspend
	SVC		#5
	BX		LR

OS_SVC_Wait
	SVC		#6
	BX		LR

OS_SVC_bWait
	SVC		#7
	BX		LR

OS_SVC_Signal
	SVC		#8
	BX		LR

OS_SVC_bSignal
	SVC		#9
	BX		LR


    ALIGN
    END
