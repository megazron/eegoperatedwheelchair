/*
 * EEG Signal Processing Arduino Template
 * Based on Spike Recorder code by Backyard Brains
 * Original code by Stanislav Mircic, https://backyardbrains.com/
 * Modified as a template by MegaZroN, portfolio https://megazron.com
 * This template is designed for Muscle SpikerShield and similar products to communicate
 * with Spike Recorder desktop software via USB (virtual serial port).
 * Sample rate depends on number of channels enabled: 10kHz / number of channels.
 * For one channel: 10kHz, two channels: 5kHz, etc.
 */

#define EKG A0 // Analog input pin for EEG signal (modify as needed)
#define BUFFER_SIZE 100 // Sampling buffer size
#define SIZE_OF_COMMAND_BUFFER 30 // Command buffer size
#define LENGTH_OF_MESSAGE_IMPULS 100 // Length of message impulse in ms

// Defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

int buffersize = BUFFER_SIZE;
int head = 0; // Head index for sampling circular buffer
int tail = 0; // Tail index for sampling circular buffer
byte writeByte;
char commandBuffer[SIZE_OF_COMMAND_BUFFER]; // Receiving command buffer
byte reading[BUFFER_SIZE]; // Sampling buffer

int messageImpulsPin = 5; // Pin for message impulse
int messageImpulseTimer = 0;

long debouncing_time = 15; // Debouncing time in milliseconds
volatile unsigned long last_micros;

int redButton = 4; // Red button pin
int greenButton = 7; // Green button pin
int redLED = 13; // Red LED pin
int greenLED = 8; // Green LED pin
int redButtonReady = 1;
int greenButtonReady = 1;

// Serial communication baud rate (options: 9600, 14400, 19200, 28800, 31250, 38400, 57600, 115200)
int baudRate = 230400;
// Interrupt number for timer setup: (16*10^6) / (Fs*8) - 1
// Set to 199 for 10kHz sampling, 1999 for 1kHz, 3999 for 500Hz, 7999 for 250Hz
int interrupt_Number = 199;
int numberOfChannels = 1; // Number of channels to sample
int tempSample = 0;
int commandMode = 0; // Flag for command mode (disable data sending)

void setup() {
  Serial.begin(baudRate); // Initialize serial communication
  delay(300); // Wait for serial initialization
  Serial.println("StartUp!");
  Serial.setTimeout(2);
  pinMode(messageImpulsPin, OUTPUT);

  // Timer setup for precise timed measurements
  cli(); // Stop interrupts

  // Optimize ADC sampling speed by changing prescaler to 16
  sbi(ADCSRA, ADPS2); // 1
  cbi(ADCSRA, ADPS1); // 0
  cbi(ADCSRA, ADPS0); // 0

  // Set timer1 interrupt for desired sampling rate
  TCCR1A = 0; // Clear TCCR1A register
  TCCR1B = 0; // Clear TCCR1B register
  TCNT1 = 0; // Initialize counter value
  OCR1A = interrupt_Number; // Output Compare Registers
  TCCR1B |= (1 << WGM12); // Turn on CTC mode
  TCCR1B |= (1 << CS11); // Set CS11 bit for 8 prescaler
  TIMSK1 |= (1 << OCIE1A); // Enable timer compare interrupt

  sei(); // Allow interrupts
}

ISR(TIMER1_COMPA_vect) {
  // Interrupt service routine for sampling at the set frequency
  if (messageImpulseTimer > 0) {
    messageImpulseTimer--;
    if (messageImpulseTimer == 0) {
      digitalWrite(messageImpulsPin, LOW);
    }
  }

  if (commandMode != 1) {
    // Sample channels and store in buffer (10-bit ADC split into 2 bytes)
    // First byte: 3 MSBs with MSB set to 1 for frame start
    // Second byte: 7 LSBs
    for (int channel = 0; channel < numberOfChannels; channel++) {
      tempSample = analogRead(A0 + channel); // Modify pin as needed
      reading[head] = (tempSample >> 7) | 0x80; // Mark frame start
      head = (head + 1) % BUFFER_SIZE;
      reading[head] = tempSample & 0x7F;
      head = (head + 1) % BUFFER_SIZE;
    }
  }
}

void serialEvent() {
  commandMode = 1; // Enter command mode, disable sampling
  TIMSK1 &= ~(1 << OCIE1A); // Disable timer interrupt

  // Read command until newline
  String inString = Serial.readStringUntil('\n');
  inString.toCharArray(commandBuffer, SIZE_OF_COMMAND_BUFFER);
  commandBuffer[inString.length()] = 0;

  // Parse commands separated by ';'
  char* command = strtok(commandBuffer, ";");
  while (command != 0) {
    char* separator = strchr(command, ':');
    if (separator != 0) {
      *separator = 0; // Split command
      --separator;
      if (*separator == 'c') { // Number of channels command
        separator += 2;
        numberOfChannels = atoi(separator); // Update number of channels
      } else if (*separator == 'p') { // Impulse command
        separator += 2;
        digitalWrite(messageImpulsPin, HIGH);
        messageImpulseTimer = (LENGTH_OF_MESSAGE_IMPULS * 10) / numberOfChannels;
      }
      // Sampling rate command ('s') is not modified in this template
    }
    command = strtok(0, ";"); // Next command
  }

  // Adjust sampling rate based on number of channels
  OCR1A = (interrupt_Number + 1) * numberOfChannels - 1;
  TIMSK1 |= (1 << OCIE1A); // Re-enable timer interrupt
  commandMode = 0; // Exit command mode
}

void loop() {
  // Send buffered data when not in command mode
  while (head != tail && commandMode != 1) {
    Serial.write(reading[tail]);
    tail = (tail + 1) % BUFFER_SIZE;
  }
}
