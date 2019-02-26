#include <EEPROM.h>
#include <LiquidCrystal.h>

#define playerWaitTime 10000 // The time allowed between button presses - 10s 

LiquidCrystal lcd(2, 3, 4, 5, 6, 7);  // Initialize the LCD

const int tones[] = {1915, 1700, 1519, 1432, 2700}; // tones 
const int pins[] = {8, 9, 10, 11};
const int buzzer = 13;


int score = 0;
int highScore = 0;
int nrPins = 4;
int ledTime = 500;

byte sequence[100]; // Storage for the light sequence
byte currentLength = 0; // Current length of the sequence
byte inputCount = 0; // The number of times that the player has pressed a (correct) button in a given turn 
byte lastInput = 0; // Last input from the player
byte ledLit = 0; // The LED that's suppose to be lit by the player

bool buttonPressed = false; // Used to check if a button is pressed
bool wait = false; // Is the program waiting for the user to press a button
bool resetFlag = false; // Used to indicate to the program that once the player lost


long inputTime = 0; // Timer variable for the delay between user inputs


void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(buzzer, HIGH);
    delayMicroseconds(tone);
    digitalWrite(buzzer, LOW);
    delayMicroseconds(tone);
  }
}

void setup() {
  if (digitalRead(A0) == HIGH){
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
  } 
}

  Serial.begin(9600);         
  EEPROM.get(0, highScore); // Get the highscore from memory.

  lcd.begin(16, 2);          
  lcd.print("Welcome!");      
  lcd.setCursor(0, 1);
  lcd.print("High Score: ");
  lcd.print(highScore);
  delay(2000);
  Reset();
}


// Sets all the pins as either INPUT or OUTPUT based on the value of 'dir'
void setPinDirection(int dir) {
 for (int i = 0; i < nrPins; i++) {
    pinMode(pins[i], dir); 
 }
}

// Send the same value to all the LED pins
void writeAllPins(int val) {
  for (int i = 0; i < nrPins; i++) {
    digitalWrite(pins[i], val); 
  }
}

// Makes a beep sound
void beep(int x) {
  analogWrite(buzzer, 2);
  delay(x);
  analogWrite(buzzer, 0);
  delay(x);
}

// Flashes all the LEDs together
// The blink speed - small number -> fast | big number -> slow
void flash(short blinkSpeed, bool countdown ) {
  setPinDirection(OUTPUT); /// We're activating the LEDS now  
  for(int i = 0; i < 5; i++) {
    if(countdown){
      lcd.setCursor(0, 1);
      lcd.print(5-i);
    }
    writeAllPins(HIGH);
    beep(50);
    delay(blinkSpeed);
    writeAllPins(LOW);
    delay(blinkSpeed);
  }
}

// This function resets all the game variables to their default values
void Reset() {
  lcd.clear();
  lcd.print("Get Ready!");
  score = 0;
  flash(500, true);
  currentLength = 0;
  inputCount = 0;
  lastInput = 0;
  ledLit = 0;
  buttonPressed = false;
  wait = false;
  resetFlag = false;
  
  UpdateScore();  
}


// User lost
void Lose() {
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Level: ");  
  lcd.print(currentLength);
  lcd.print(" - ");
  lcd.print(score);
  lcd.print("pt");
  lcd.setCursor(0, 0);
  if(score > highScore){ // if we beat the high score
    lcd.print("High Score! ");
    highScore = score;
    EEPROM.put(0, highScore); // Save the new high score
    flash(50, false);
  }
  else {
    lcd.print("You lose!"); 
    flash(50, false);
  }
  delay(250);
}

// Displays the score and current sequence on the screen
void UpdateScore() {
  lcd.clear(); //Clear the screen
  lcd.print("Level: ");  
  lcd.print(currentLength);
  lcd.setCursor(0,1); // Sets the cursor to the beginning of the second row 
  lcd.print("Score: ");
  lcd.print(score);
}


// The arduino shows the user what must be memorized
// Also called after losing to show you what you last sequence was
void playSequence() {
  for (int i = 0; i < currentLength; i++) {
      digitalWrite(sequence[i], HIGH);
      delay(500);
      digitalWrite(sequence[i], LOW);
      playTone(tones[i], ledTime);
      delay(250);
  }
}



void doLoseProcess() {
  Lose(); // Flash all the LEDS quickly (see Lose function)
  delay(1000);
  playSequence(); // Shows the user the last sequence - So you can count remember your best score
  delay(1000);
  Reset(); // Reset everything for a new game
}

void playGame() { 
   if (!wait) {
    setPinDirection(OUTPUT); // We're using the LEDs
    randomSeed(analogRead(A0));                   
    sequence[currentLength] = pins[random(0,nrPins)]; // Put a new random value in the next position in the sequence 
    currentLength++; // Set the new Current length of the sequence
    UpdateScore(); // Updates the score so the player can see that he/she entered the next sequence.
    playSequence(); // Show the sequence to the player
    beep(50); // Make a beep for the player to be aware
    wait = true; // Set wait to true as it's now going to be the turn of the player
    inputTime = millis(); // Store the time to measure the player's response time
  }
  else { 
    setPinDirection(INPUT); // We're using the buttons
    if (millis() - inputTime > playerWaitTime) {  // If the player takes more than the allowed time
      doLoseProcess();                            
      return;
    }      
   if (!buttonPressed) {                                   
      ledLit = sequence[inputCount]; // Find the value we expect from the player
      for (int i = 0; i < nrPins; i++) {           
        if (pins[i] == ledLit)                        
          continue; // Ignore the correct pin
        if (digitalRead(pins[i]) == HIGH) { // Is the buttong pressed
          lastInput = pins[i];
          resetFlag = true; // Set the resetFlag - this means you lost
          buttonPressed = true; // This will prevent the program from doing the same thing over and over again
        }
      }      
    }

    if (digitalRead(ledLit) == 1 && !buttonPressed) { // The player pressed the right button
      score++;
      UpdateScore();
      inputTime = millis();                       
      lastInput = ledLit;
      inputCount++; // The user pressed a (correct) button again
      buttonPressed = true; // This will prevent the program from doing the same thing over and over again
    }
    else {
      if (buttonPressed && digitalRead(lastInput) == LOW) { // Check if the player released the button
        buttonPressed = false;
        delay(20);
        if (resetFlag) { // If this was set to true up above, you lost
          doLoseProcess(); // So we do the losing sequence of events
        }
        else {
          if (inputCount == currentLength) { // Has the player finished repeating the sequence
            wait = false; // If so, this will make the next turn the program's turn
            inputCount = 0; // Reset the number of times that the player has pressed a button
            delay(1500);
          }
        }
      }
    }    
  }
}
 void loop() {
 
 playGame();
 
}

  
