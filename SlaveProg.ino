#define SAMPLE_RATE 512
#define BAUD_RATE 115200
#define INPUT_PIN A0
#define MOTOR_PIN1 10  // Motor 1 control
#define MOTOR_PIN2 11  // Motor 2 control

void setup() {
  // Initialize serial communication
  Serial.begin(BAUD_RATE);

  // Set motor pins as outputs
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
}

void loop() {
  // Calculate elapsed time
  static unsigned long past = 0;
  unsigned long present = micros();
  unsigned long interval = present - past;
  past = present;

  // Run timer
  static long timer = 0;
  timer -= interval;

  // Sample
  if (timer < 0) {
    timer += 1000000 / SAMPLE_RATE;
    int sensor_value = analogRead(INPUT_PIN);
    Serial.println(sensor_value);

    // Motor control conditions
    if (sensor_value >= 400 && sensor_value < 500) {
      // Move left
      digitalWrite(MOTOR_PIN1, HIGH);
      digitalWrite(MOTOR_PIN2, LOW);
      Serial.println("Moving Left");
    } 
    else if (sensor_value >= 500 && sensor_value < 600) {
      // Move right
      digitalWrite(MOTOR_PIN1, LOW);
      digitalWrite(MOTOR_PIN2, HIGH);
      Serial.println("Moving Right");
    } 
    else if (sensor_value >= 600 && sensor_value < 700) {
      // Move forward
      digitalWrite(MOTOR_PIN1, HIGH);
      digitalWrite(MOTOR_PIN2, HIGH);
      Serial.println("Moving Forward");
    } 
    else if (sensor_value >= 700 && sensor_value < 800) {
      // Stop
      digitalWrite(MOTOR_PIN1, LOW);
      digitalWrite(MOTOR_PIN2, LOW);
      Serial.println("Stopping");
    } 
    else {
      digitalWrite(MOTOR_PIN1, HIGH);
      digitalWrite(MOTOR_PIN2, HIGH);
    }
  }
}
