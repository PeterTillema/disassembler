	section .text
	public _int_to_digits
_int_to_digits:
	ld	iy, 0
	add	iy, sp
	ld	de, (iy + 3)
	ld	hl, (iy + 6)

	ld	bc, -10000000
	call	.num1
	ld	bc, -1000000
	call	.num1
	ld	bc, -100000
	call	.num1
	ld	bc, -10000
	call	.num1
	ld	bc, -1000
	call	.num1
	ld	bc, -100
	call	.num1
	ld	bc, -10
	call	.num1
	ld	bc, -1
	call	.num1
	xor	a, a
	ld	(de), a
	ret
.num1:
	xor	a, a
.num2:
	inc	a
	add	hl, bc
	jr	c, .num2
	sbc	hl, bc
	dec	a
	ret	z			; Digit is zero, don't print
	add	a, '0'
	ld	(de), a
	inc	de
	ret


	public _byte_to_hex
_byte_to_hex:
	ld	iy, 0
	add	iy, sp
	ld	hl, (iy + 3)
	jr	__byte_hex

	public _short_to_hex
_short_to_hex:
	ld	iy, 0
	add	iy, sp
	ld	hl, (iy + 3)
	jr	__short_hex

	public _int_to_hex
_int_to_hex:
	ld	iy, 0
	add	iy, sp
	ld	hl, (iy + 3)
	ld	a, (iy + 8)
	call	_digit_to_hex
__short_hex:
	ld	a, (iy + 7)
	call	_digit_to_hex
__byte_hex:
	ld	a, (iy + 6)

_digit_to_hex:
	ld	b, a
	rra
	rra
	rra
	rra
	and	a, 0x0F
	or	a, 0xF0
	daa
	add	a, 0xA0
	adc	a, 0x40
	ld	(hl), a
	inc	hl
	ld	a, b
	or	a, 0xF0
	daa
	add	a, 0xA0
	adc	a, 0x40
	ld	(hl), a
	inc	hl
	ld	(hl), 0
	ret

