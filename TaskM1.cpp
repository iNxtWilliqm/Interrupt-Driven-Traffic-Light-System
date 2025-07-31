volatile byte r, y, g = 0;
volatile bool triggered = false;

void setup() {
  // Output pins
  DDRB |= 0b00000111;
  
  // Enables PCIE2 (Port D interrupts)
  PCICR = 0b00000100;
  
  // Enables interrupts on PD5, PD6, PD7 (Arduino pins 5,6,7)
  PCMSK2 = 0b11100000;
  
  // Clear the timer register
  TCCR1A = 0; 
  
  TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC mode, Prescaler: 1024 (16MHz / 1024 = 15,625Hz)
  // TCCR1B = 0b00001011; // CTC mode, Prescaler: 1024
  
  OCR1A = 1 * 16000000 / 1024 - 1; // Set Compare Match value for approximately n-second interval
  
  TIMSK1 = (1 << OCIE1A); // Enable Timer1 output compare A match interrupt
  // TIMSK1 = 0b00000010;  // Enable Timer1 output compare A match interrupt
  
  Serial.begin(9600);
  
  // Start with red
  r = 1;
  // Initialize LEDs
  updateLED();
}

void loop() {
	// Nothing here
}

void updateLED() {
  digitalWrite(8, g);
  digitalWrite(9, y);
  digitalWrite(10, r);
}
volatile int pciCount = 0;

// PCI ISR (Event-Based)
ISR(PCINT2_vect) {
  r = PIND & 0b10000000; // D7
  y = PIND & 0b01000000; // D6
  g = PIND & 0b00100000; // D5
  
  triggered = true; // Signal Timer ISR to sync cycle
  
  Serial.println("PCI Triggered");
  
  updateLED();
}

// Timer1 ISR (Time-Based)
ISR(TIMER1_COMPA_vect) {
  static byte cycle = 0;
  
  if (triggered) {
    // Sync cycle with current LED state
    if (r)       cycle = 0;
    else if (y)  cycle = (g) ? 3 : 1; // If coming from green, next is Y (state 3)
    else if (g)  cycle = 2;
    triggered = false; // Resume normal cycle
  } else {
    // Cycle: 0 (Red) -> 1 (Yellow) -> 2 (Green) -> 3 (Yellow) -> 0 (Red)...
    cycle = (cycle + 1) % 4;
    r = (cycle == 0);
    y = (cycle == 1 || cycle == 3);
    g = (cycle == 2);
  }

  Serial.print("Time cycle=");
  Serial.println(cycle);
  
  updateLED();
}