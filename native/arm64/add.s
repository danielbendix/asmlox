	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0	sdk_version 14, 0
	.globl	_op_add                         ; -- Begin function op_add
	.p2align	2
_op_add:                                ; @op_add
	.cfi_startproc
; %bb.0:
    ldp x0, x1, [sp, #16]
    ldp x2, x3, [sp, #0]

    and w4, w0, w2
	;cmp	w0, w2
	;b.ne	LBB0_7
; %bb.1:
	cmp	w4, #2
	b.ne	LBB0_3
; %bb.2:
	fmov	d0, x1
	fmov	d1, x3
	fadd	d0, d0, d1
	fmov	x1, d0
	;mov	w0, #2
    stp x0, x1, [sp, #16]! ; store result
	ret
LBB0_3:

    ; TODO: Only do these if string or failure
	stp	x22, x21, [sp, #-48]!           ; 16-byte Folded Spill
	.cfi_def_cfa_offset 48
	stp	x20, x19, [sp, #16]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #32]             ; 16-byte Folded Spill
	add	x29, sp, #32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48

	cmp	w0, #3
	b.ne	LBB0_7
; %bb.4:
	ldr	w8, [x20]
	and	x9, x2, #0xffffffff
	cmp	w8, #6
	ccmp	x9, #3, #0, eq
	b.ne	LBB0_7
; %bb.5:
	ldr	w8, [x19]
	cmp	w8, #6
	b.ne	LBB0_7
; %bb.6:
	ldrsw	x8, [x20, #16]
	ldrsw	x9, [x19, #16]
	add	x21, x9, x8
	add	x2, x21, #1
	mov	x0, #0
	mov	x1, #0
	bl	_reallocate
	mov	x22, x0
	ldr	x1, [x20, #24]
	ldrsw	x2, [x20, #16]
	bl	_memcpy
	ldrsw	x8, [x20, #16]
	add	x0, x22, x8
	ldr	x1, [x19, #24]
	ldrsw	x2, [x19, #16]
	bl	_memcpy
	strb	wzr, [x22, x21]
	mov	x0, x22
	mov	x1, x21
	bl	_takeString
	mov	x1, x0
	mov	w0, #3
	ldp	x29, x30, [sp, #32]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #16]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp], #48             ; 16-byte Folded Reload
    stp x0, x1, [sp, #-16]! ; store result
	ret
LBB0_7:
Lloh0:
	adrp	x0, l_.str@PAGE
Lloh1:
	add	x0, x0, l_.str@PAGEOFF
	bl	_runtimeError
	.loh AdrpAdd	Lloh0, Lloh1
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"Operands must be two numbers or two strings."

.subsections_via_symbols
