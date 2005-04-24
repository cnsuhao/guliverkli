
	.686
	.model flat
	.mmx
	.xmm

	.const

	__uvmin DD 0d01502f9r ; -1e+010
	__uvmax DD 0501502f9r ; +1e+010

	.code
	
;
; memsetd
;

@memsetd@12 proc public

	push	edi
	
	mov		edi, ecx
	mov		eax, edx
	mov		ecx, [esp+4+4]
	cld
	rep		stosd
	
	pop		edi

	ret		4
	
@memsetd@12 endp

;
; ticks
;

_ticks proc public

	rdtsc
	ret

_ticks endp

;
; SaturateColor
;
			
@SaturateColor_sse2@4 proc public

	pxor		xmm0, xmm0
	movaps		xmm1, [ecx]
	packssdw	xmm1, xmm0
	packuswb	xmm1, xmm0
	punpcklbw	xmm1, xmm0
	punpcklwd	xmm1, xmm0
	movaps		[ecx], xmm1

	ret

@SaturateColor_sse2@4 endp

@SaturateColor_asm@4 proc public

	push	esi

	mov		esi, ecx

	xor		eax, eax
	mov		edx, 000000ffh

	mov		ecx, [esi]
	cmp		ecx, eax
	cmovl	ecx, eax
	cmp		ecx, edx
	cmovg	ecx, edx
	mov		[esi], ecx

	mov		ecx, [esi+4]
	cmp		ecx, eax
	cmovl	ecx, eax
	cmp		ecx, edx
	cmovg	ecx, edx
	mov		[esi+4], ecx

	mov		ecx, [esi+8]
	cmp		ecx, eax
	cmovl	ecx, eax
	cmp		ecx, edx
	cmovg	ecx, edx
	mov		[esi+8], ecx

	mov		ecx, [esi+12]
	cmp		ecx, eax
	cmovl	ecx, eax
	cmp		ecx, edx
	cmovg	ecx, edx
	mov		[esi+12], ecx
	
	pop		esi
	
	ret
	
@SaturateColor_asm@4 endp

;
; swizzling
;

punpck macro op, sd0, sd2, s1, s3, d1, d3

	movaps					@CatStr(xmm, %d1),	@CatStr(xmm, %sd0)
	pshufd					@CatStr(xmm, %d3),	@CatStr(xmm, %sd2), 0e4h
	
	@CatStr(punpckl, op)	@CatStr(xmm, %sd0),	@CatStr(xmm, %s1)
	@CatStr(punpckh, op)	@CatStr(xmm, %d1),	@CatStr(xmm, %s1)
	@CatStr(punpckl, op)	@CatStr(xmm, %sd2),	@CatStr(xmm, %s3)
	@CatStr(punpckh, op)	@CatStr(xmm, %d3),	@CatStr(xmm, %s3)

	endm
	
punpcknb macro

	movaps	xmm4, xmm0
	pshufd	xmm5, xmm1, 0e4h

	psllq	xmm1, 4
	psrlq	xmm4, 4

	movaps	xmm6, xmm7
	pand	xmm0, xmm7
	pandn	xmm6, xmm1
	por		xmm0, xmm6

	movaps	xmm6, xmm7
	pand	xmm4, xmm7
	pandn	xmm6, xmm5
	por		xmm4, xmm6

	movaps	xmm1, xmm4

	movaps	xmm4, xmm2
	pshufd	xmm5, xmm3, 0e4h

	psllq	xmm3, 4
	psrlq	xmm4, 4

	movaps	xmm6, xmm7
	pand	xmm2, xmm7
	pandn	xmm6, xmm3
	por		xmm2, xmm6

	movaps	xmm6, xmm7
	pand	xmm4, xmm7
	pandn	xmm6, xmm5
	por		xmm4, xmm6

	movaps	xmm3, xmm4

	punpck	bw, 0, 2, 1, 3, 4, 6

	endm

;
; unSwizzleBlock32
;

@unSwizzleBlock32_sse2@12 proc public

	push		ebx

	mov			ebx, [esp+4+4]
	mov			eax, 4

	align 16
@@:
	movaps		xmm0, [ecx+16*0]
	movaps		xmm1, [ecx+16*1]
	movaps		xmm2, [ecx+16*2]
	movaps		xmm3, [ecx+16*3]

	punpck		qdq, 0, 2, 1, 3, 4, 6

	movaps		[edx], xmm0
	movaps		[edx+16], xmm2
	movaps		[edx+ebx], xmm4
	movaps		[edx+ebx+16], xmm6

	add			ecx, 64
	lea			edx, [edx+ebx*2]

	dec			eax
	jnz			@B

	pop			ebx

	ret			4

@unSwizzleBlock32_sse2@12 endp

;
; unSwizzleBlock16
;

@unSwizzleBlock16_sse2@12 proc public

	push		ebx

	mov			ebx, [esp+4+4]
	mov			eax, 4
	
	align 16
@@:
	movaps		xmm0, [ecx+16*0]
	movaps		xmm1, [ecx+16*1]
	movaps		xmm2, [ecx+16*2]
	movaps		xmm3, [ecx+16*3]

	punpck		wd, 0, 2, 1, 3, 4, 6
	punpck		dq, 0, 4, 2, 6, 1, 3
	punpck		wd, 0, 4, 1, 3, 2, 6

	movaps		[edx], xmm0
	movaps		[edx+16], xmm2
	movaps		[edx+ebx], xmm4
	movaps		[edx+ebx+16], xmm6

	add			ecx, 64
	lea			edx, [edx+ebx*2]

	dec			eax
	jnz			@B
	
	pop			ebx
	
	ret			4
	
@unSwizzleBlock16_sse2@12 endp

;
; unSwizzleBlock8
;

@unSwizzleBlock8_sse2@12 proc public

	push		ebx

	mov			ebx, [esp+4+4]
	mov			eax, 2

	align 16
@@:
	; col 0, 2

	movaps		xmm0, [ecx+16*0]
	movaps		xmm1, [ecx+16*1]
	movaps		xmm4, [ecx+16*2]
	movaps		xmm5, [ecx+16*3]

	punpck		bw,  0, 4, 1, 5, 2, 6
	punpck		wd,  0, 2, 4, 6, 1, 3
	punpck		bw,  0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 2, 4, 6, 1, 3

	pshufd		xmm1, xmm1, 0b1h
	pshufd		xmm3, xmm3, 0b1h

	movaps		[edx], xmm0
	movaps		[edx+ebx], xmm2
	lea			edx, [edx+ebx*2]

	movaps		[edx], xmm1
	movaps		[edx+ebx], xmm3
	lea			edx, [edx+ebx*2]

	; col 1, 3

	movaps		xmm0, [ecx+16*4]
	movaps		xmm1, [ecx+16*5]
	movaps		xmm4, [ecx+16*6]
	movaps		xmm5, [ecx+16*7]

	punpck		bw,  0, 4, 1, 5, 2, 6
	punpck		wd,  0, 2, 4, 6, 1, 3
	punpck		bw,  0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 2, 4, 6, 1, 3

	pshufd		xmm0, xmm0, 0b1h
	pshufd		xmm2, xmm2, 0b1h

	movaps		[edx], xmm0
	movaps		[edx+ebx], xmm2
	lea			edx, [edx+ebx*2]

	movaps		[edx], xmm1
	movaps		[edx+ebx], xmm3
	lea			edx, [edx+ebx*2]

	add			ecx, 128

	dec			eax
	jnz			@B

	pop			ebx
	
	ret			4

@unSwizzleBlock8_sse2@12 endp

;
; unSwizzleBlock4
;

@unSwizzleBlock4_sse2@12 proc public

	push		ebx

	mov         eax, 0f0f0f0fh
	movd        xmm7, eax 
	pshufd      xmm7, xmm7, 0

	mov			ebx, [esp+4+4]
	mov			eax, 2

	align 16
@@:
	; col 0, 2

	movaps		xmm0, [ecx+16*0]
	movaps		xmm1, [ecx+16*1]
	movaps		xmm4, [ecx+16*2]
	movaps		xmm3, [ecx+16*3]

	punpck		dq, 0, 4, 1, 3, 2, 6
	punpck		dq, 0, 2, 4, 6, 1, 3
	punpcknb
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		wd, 0, 2, 1, 3, 4, 6

	pshufd		xmm0, xmm0, 0d8h
	pshufd		xmm2, xmm2, 0d8h
	pshufd		xmm4, xmm4, 0d8h
	pshufd		xmm6, xmm6, 0d8h

	punpck		qdq, 0, 2, 4, 6, 1, 3

	pshuflw		xmm1, xmm1, 0b1h
	pshuflw		xmm3, xmm3, 0b1h
	pshufhw		xmm1, xmm1, 0b1h
	pshufhw		xmm3, xmm3, 0b1h

	movaps		[edx], xmm0
	movaps		[edx+ebx], xmm2
	lea			edx, [edx+ebx*2]

	movaps		[edx], xmm1
	movaps		[edx+ebx], xmm3
	lea			edx, [edx+ebx*2]

	; col 1, 3

	movaps		xmm0, [ecx+16*4]
	movaps		xmm1, [ecx+16*5]
	movaps		xmm4, [ecx+16*6]
	movaps		xmm3, [ecx+16*7]

	punpck		dq, 0, 4, 1, 3, 2, 6
	punpck		dq, 0, 2, 4, 6, 1, 3
	punpcknb
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		wd, 0, 2, 1, 3, 4, 6

	pshufd		xmm0, xmm0, 0d8h
	pshufd		xmm2, xmm2, 0d8h
	pshufd		xmm4, xmm4, 0d8h
	pshufd		xmm6, xmm6, 0d8h

	punpck		qdq, 0, 2, 4, 6, 1, 3

	pshuflw		xmm0, xmm0, 0b1h
	pshuflw		xmm2, xmm2, 0b1h
	pshufhw		xmm0, xmm0, 0b1h
	pshufhw		xmm2, xmm2, 0b1h

	movaps		[edx], xmm0
	movaps		[edx+ebx], xmm2
	lea			edx, [edx+ebx*2]

	movaps		[edx], xmm1
	movaps		[edx+ebx], xmm3
	lea			edx, [edx+ebx*2]

	add			ecx, 128

	dec			eax
	jnz			@B

	pop			ebx
	
	ret			4

@unSwizzleBlock4_sse2@12 endp

;
; unSwizzleBlock8HP
;

@unSwizzleBlock8HP_sse2@12 proc public

	push		ebx

	mov			ebx, [esp+4+4]
	mov			eax, 4

	align 16
@@:
	movaps		xmm0, [ecx+16*0]
	movaps		xmm1, [ecx+16*1]
	movaps		xmm2, [ecx+16*2]
	movaps		xmm3, [ecx+16*3]

	punpck		qdq, 0, 2, 1, 3, 4, 6
	
	pslld		xmm0, 24
	pslld		xmm2, 24
	pslld		xmm4, 24
	pslld		xmm6, 24
	
	packssdw	xmm0, xmm2
	packssdw	xmm4, xmm6
	packuswb	xmm0, xmm4

	movlps		qword ptr [edx], xmm0
	movhps		qword ptr [edx+ebx], xmm0

	add			ecx, 64
	lea			edx, [edx+ebx*2]

	dec			eax
	jnz			@B

	pop			ebx

	ret			4

@unSwizzleBlock8HP_sse2@12 endp

;
; unSwizzleBlock4HLP
;

@unSwizzleBlock4HLP_sse2@12 proc public

	push		ebx

	mov			ebx, [esp+4+4]
	mov			eax, 4
	
	align 16
@@:
	movaps		xmm0, [ecx+16*0]
	movaps		xmm1, [ecx+16*1]
	movaps		xmm2, [ecx+16*2]
	movaps		xmm3, [ecx+16*3]

	punpck		qdq, 0, 2, 1, 3, 4, 6
	
	psrld		xmm0, 4
	psrld		xmm2, 4
	psrld		xmm4, 4
	psrld		xmm6, 4
	
	pslld		xmm0, 24
	pslld		xmm2, 24
	pslld		xmm4, 24
	pslld		xmm6, 24
	
	packssdw	xmm0, xmm2
	packssdw	xmm4, xmm6
	packuswb	xmm0, xmm4

	movlps		qword ptr [edx], xmm0
	movhps		qword ptr [edx+ebx], xmm0

	add			ecx, 64
	lea			edx, [edx+ebx*2]

	dec			eax
	jnz			@B

	pop			ebx

	ret			4

@unSwizzleBlock4HLP_sse2@12 endp

;
; unSwizzleBlock4HHP
;

@unSwizzleBlock4HHP_sse2@12 proc public

	push		ebx

	mov			ebx, [esp+4+4]
	mov			eax, 4

	align 16
@@:
	movaps		xmm0, [ecx+16*0]
	movaps		xmm1, [ecx+16*1]
	movaps		xmm2, [ecx+16*2]
	movaps		xmm3, [ecx+16*3]

	punpck		qdq, 0, 2, 1, 3, 4, 6
	
	pslld		xmm0, 28
	pslld		xmm2, 28
	pslld		xmm4, 28
	pslld		xmm6, 28
	
	packssdw	xmm0, xmm2
	packssdw	xmm4, xmm6
	packuswb	xmm0, xmm4

	movlps		qword ptr [edx], xmm0
	movhps		qword ptr [edx+ebx], xmm0

	add			ecx, 64
	lea			edx, [edx+ebx*2]

	dec			eax
	jnz			@B

	pop			ebx

	ret			4

@unSwizzleBlock4HHP_sse2@12 endp

;
; swizzling
;

;
; SwizzleBlock32
;

@SwizzleBlock32_sse2@16 proc public

	push		esi
	push		edi

	mov			edi, ecx
	mov			esi, edx
	mov			edx, [esp+4+8]
	mov			ecx, 4

	mov			eax, [esp+8+8]
	cmp			eax, 0ffffffffh
	jnz			SwizzleBlock32_sse2@WM

	align 16
@@:
	movaps		xmm0, [esi]
	movaps		xmm4, [esi+16]
	movaps		xmm1, [esi+edx]
	movaps		xmm5, [esi+edx+16]

	punpck		qdq, 0, 4, 1, 5, 2, 6

	movaps		[edi+16*0], xmm0
	movaps		[edi+16*1], xmm2
	movaps		[edi+16*2], xmm4
	movaps		[edi+16*3], xmm6

	lea			esi, [esi+edx*2]
	add			edi, 64

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi

	ret			8

SwizzleBlock32_sse2@WM:

	movd		xmm7, eax
	pshufd		xmm7, xmm7, 0
	
	align 16
@@:
	movaps		xmm0, [esi]
	movaps		xmm4, [esi+16]
	movaps		xmm1, [esi+edx]
	movaps		xmm5, [esi+edx+16]

	punpck		qdq, 0, 4, 1, 5, 2, 6

	movaps		xmm3, xmm7
	pshufd		xmm5, xmm7, 0e4h

	pandn		xmm3, [edi+16*0]
	pand		xmm0, xmm7
	por			xmm0, xmm3
	movaps		[edi+16*0], xmm0

	pandn		xmm5, [edi+16*1]
	pand		xmm2, xmm7
	por			xmm2, xmm5
	movaps		[edi+16*1], xmm2

	movaps		xmm3, xmm7
	pshufd		xmm5, xmm7, 0e4h

	pandn		xmm3, [edi+16*2]
	pand		xmm4, xmm7
	por			xmm4, xmm3
	movaps		[edi+16*2], xmm4

	pandn		xmm5, [edi+16*3]
	pand		xmm6, xmm7
	por			xmm6, xmm5
	movaps		[edi+16*3], xmm6

	lea			esi, [esi+edx*2]
	add			edi, 64

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi

	ret			8
	
@SwizzleBlock32_sse2@16 endp

;
; SwizzleBlock16
;

@SwizzleBlock16_sse2@12 proc public

	push		ebx

	mov			ebx, [esp+4+4]
	mov			eax, 4

	align 16
@@:
	movaps		xmm0, [edx]
	movaps		xmm1, [edx+16]
	movaps		xmm2, [edx+ebx]
	movaps		xmm3, [edx+ebx+16]

	punpck		wd, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 5

	movaps		[ecx+16*0], xmm0
	movaps		[ecx+16*1], xmm1
	movaps		[ecx+16*2], xmm4
	movaps		[ecx+16*3], xmm5

	lea			edx, [edx+ebx*2]
	add			ecx, 64

	dec			eax
	jnz			@B

	pop			ebx

	ret			4

@SwizzleBlock16_sse2@12 endp

;
; SwizzleBlock8
;

@SwizzleBlock8_sse2@12 proc public

	push		ebx

	mov			ebx, [esp+4+4]
	mov			eax, 2

	align 16
@@:
	; col 0, 2

	movaps		xmm0, [edx]
	movaps		xmm2, [edx+ebx]
	lea			edx, [edx+ebx*2]

	pshufd		xmm1, [edx], 0b1h
	pshufd		xmm3, [edx+ebx], 0b1h
	lea			edx, [edx+ebx*2]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 2, 4, 6, 1, 3
	punpck		qdq, 0, 1, 2, 3, 4, 5

	movaps		[ecx+16*0], xmm0
	movaps		[ecx+16*1], xmm4
	movaps		[ecx+16*2], xmm1
	movaps		[ecx+16*3], xmm5

	; col 1, 3

	pshufd		xmm0, [edx], 0b1h
	pshufd		xmm2, [edx+ebx], 0b1h
	lea			edx, [edx+ebx*2]

	movaps		xmm1, [edx]
	movaps		xmm3, [edx+ebx]
	lea			edx, [edx+ebx*2]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 2, 4, 6, 1, 3
	punpck		qdq, 0, 1, 2, 3, 4, 5

	movaps		[ecx+16*4], xmm0
	movaps		[ecx+16*5], xmm4
	movaps		[ecx+16*6], xmm1
	movaps		[ecx+16*7], xmm5

	add			ecx, 128

	dec			eax
	jnz			@B

	pop			ebx

	ret			4
	
@SwizzleBlock8_sse2@12 endp

;
; SwizzleBlock4
;

@SwizzleBlock4_sse2@12 proc public

	push		ebx
	
	mov         eax, 0f0f0f0fh
	movd        xmm7, eax 
	pshufd      xmm7, xmm7, 0

	mov			ebx, [esp+4+4]
	mov			eax, 2

	align 16
@@:
	; col 0, 2

	movaps		xmm0, [edx]
	movaps		xmm2, [edx+ebx]
	lea			edx, [edx+ebx*2]

	movaps		xmm1, [edx]
	movaps		xmm3, [edx+ebx]
	lea			edx, [edx+ebx*2]

	pshuflw		xmm1, xmm1, 0b1h
	pshuflw		xmm3, xmm3, 0b1h
	pshufhw		xmm1, xmm1, 0b1h
	pshufhw		xmm3, xmm3, 0b1h

	punpcknb
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 3

	movaps		[ecx+16*0], xmm0
	movaps		[ecx+16*1], xmm1
	movaps		[ecx+16*2], xmm4
	movaps		[ecx+16*3], xmm3

	; col 1, 3

	movaps		xmm0, [edx]
	movaps		xmm2, [edx+ebx]
	lea			edx, [edx+ebx*2]

	movaps		xmm1, [edx]
	movaps		xmm3, [edx+ebx]
	lea			edx, [edx+ebx*2]

	pshuflw		xmm0, xmm0, 0b1h
	pshuflw		xmm2, xmm2, 0b1h
	pshufhw		xmm0, xmm0, 0b1h
	pshufhw		xmm2, xmm2, 0b1h

	punpcknb
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 3

	movaps		[ecx+16*4], xmm0
	movaps		[ecx+16*5], xmm1
	movaps		[ecx+16*6], xmm4
	movaps		[ecx+16*7], xmm3

	add			ecx, 128

	dec			eax
	jnz			@B

	pop			ebx

	ret			4

@SwizzleBlock4_sse2@12 endp

;
; UVMinMax
;

@UVMinMax_sse2@12 proc public

	push		ebx
	mov			ebx, [esp+4+4]

	add			edx, 16

	movss		xmm6, __uvmax
	pshufd      xmm6, xmm6, 0

	movss		xmm7, __uvmin
	pshufd      xmm7, xmm7, 0

	align 16
@@:
	movaps		xmm0, [edx]
	minps		xmm6, xmm0
	maxps		xmm7, xmm0
	lea			edx, [edx+32]

	dec			ecx
	jnz			@B

	movhlps		xmm6, xmm6
	movss		dword ptr [ebx], xmm6
	pshufd		xmm6, xmm6, 55h
	movss		dword ptr [ebx+4], xmm6

	movhlps		xmm7, xmm7
	movss		dword ptr [ebx+8], xmm7
	pshufd		xmm7, xmm7, 55h
	movss		dword ptr [ebx+12], xmm7
	
	pop			ebx
	
	ret			4
	
@UVMinMax_sse2@12 endp

	end