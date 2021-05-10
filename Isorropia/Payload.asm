_DATA SEGMENT
PUBLIC payloadResult
	payloadResult dq 0
	thou real4 1000.0
_DATA ENDS

_TEXT    SEGMENT
PUBLIC Payload
Payload PROC
	lea rax,[r14+38h]
	mov [payloadResult],rax
	movss xmm7,[thou]
	mov r12d,4
	ret
Payload ENDP
_TEXT    ENDS
END