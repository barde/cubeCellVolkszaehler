/*
 * Simple LED Blink Test for CubeCell
 * Tests if the board is running at all
 */

#include "Arduino.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("CubeCell Simple Test Starting...");
  
  // Initialize RGB LED
  pinMode(RGB, OUTPUT);
  
  Serial.println("Setup complete, starting blink...");
}

void loop() {
  // Simple LED blink
  digitalWrite(RGB, HIGH);
  Serial.println("LED ON");
  delay(1000);
  
  digitalWrite(RGB, LOW);
  Serial.println("LED OFF");
  delay(1000);
}