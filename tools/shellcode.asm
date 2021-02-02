.set SER0_THR, 0x70006000
.set SER0_DLL, 0x70006000
.set SER0_DLM, 0x70006004
.set SER0_FCR, 0x70006008
.set SER0_LCR, 0x7000600c
.set SER0_LSR, 0x70006014

.set fb_base, 0x10885db0

.text
.global _start

_start:

// make the piezo buzzer buzz
	ldr r0, =0x7000A000
	ldr r1, =0x80800800
	str r1, [r0]

// UART init code derived from rockbox/firmware/drivers/serial.c

// divisor latch enable
	ldr r0, =SER0_LCR
	ldr r1, =0x80
	str r1, [r0]

// DLM = 0
	ldr r0, =SER0_DLM
	ldr r1, =0
	str r1, [r0]

// set DLL
	ldr r0, =SER0_DLL
	ldr r1, =((24000000L / 500000 / 16) & 0xFF)
	str r1, [r0]

// divisor latch disable, 8n1
	ldr r0, =SER0_LCR
	ldr r1, =0x03
	str r1, [r0]

// Tx+Rx FIFO reset and FIFO enable
	ldr r0, =SER0_FCR
	ldr r1, =0x07
	str r1, [r0]

start_hello:
	ldr r2, =hello
	
hello_loop:
	ldrb r1, [r2]
	add r2, #1
	cmp r1, #0
	beq start_hello
	
	ldr r0, =SER0_THR
	str r1, [r0]


// wait for UART to be ready to send
	ldr r0, =SER0_LSR
wait_for_ready:
	ldr r1, [r0]
	tst r1, #0x20  // bit 5 will be set if ready to TX
	beq wait_for_ready    // branch if zero (not ready)

	b hello_loop

end:
	b end

hello:
.ascii "Hello, UART world!\r\n\0"
