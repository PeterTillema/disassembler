	section .text
	public __Z13int_to_digitsPcj
__Z13int_to_digitsPcj:
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


	public __Z13digits_to_intPc
__Z13digits_to_intPc:
	ld	iy, 0
	add	iy, sp
	sbc	hl, hl
	ld	de, (iy + 3)
.loop:
	ld	a, (de)
	inc	de
	or	a, a
	ret	z
	cp	a, 'Z' + 1
	jr	nc, .null
	cp	a, 'A'
	jr	c, .digit
	sub	a, 'A' - '9' - 1
.digit:
	sub	a, '0'
	add	hl, hl
	add	hl, hl
	add	hl, hl
	add	hl, hl
	or	a, l
	ld	l, a
	jr	.loop
.null:
	or	a, a
	sbc	hl, hl
	ret


	public __Z11byte_to_hexPch
__Z11byte_to_hexPch:
	ld	iy, 0
	add	iy, sp
	ld	hl, (iy + 3)
	jr	__byte_hex

	public __Z12short_to_hexPcj
__Z12short_to_hexPcj:
	ld	iy, 0
	add	iy, sp
	ld	hl, (iy + 3)
	jr	__short_hex

	public __Z10int_to_hexPcj
__Z10int_to_hexPcj:
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

