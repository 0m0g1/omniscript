const DDRB = 0x24.asPointer<Uint8>(); // Data Direction Register B (sets pins as input/output)
const PORTB = 0x25.asPointer<Uint8>(); // Port B Output Register

// Configure all pins as outputs
DDRB.value = 0xFF; // 0b11111111 (all pins output)

// Toggle all pins (blink LEDs)
while (true) {
  PORTB.value ^= 0xFF; // XOR to flip bits
  console.log(PORTB.value);
  Time.sleep(500);
}