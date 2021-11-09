void shiftOut(int myDataPin, int myClockPin, word myDataOut) {
  // this shifts 16 bits out MSB first, 
  // on the rising edge of the clock,
  // clock idles low

  // internal function setup
  int i = 0;
  int pinState;

  // clear everything out just in case to
  // prepare shift register for bit shifting
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);

  // for each bit in the byte myDataOut
  // NOTICE THAT WE ARE COUNTING DOWN in our for loop
  // this means that %00000001 or "1" will go through such
  // that it will be pin Q0 that lights. 
  for (i = 15; i >= 0; i--)  {
    digitalWrite(myClockPin, 0);

    // if the value passed to myDataOut and a bitmask result 
    // true then... so if we are at i=6 and our value is
    // %11010100 it would the code compares it to %01000000 
    // and proceeds to set pinState to 1.
    if (myDataOut & (1 << i)) {
      pinState = 1;
    } else {  
      pinState = 0;
    }

    // sets the pin to HIGH or LOW depending on pinState
    digitalWrite(myDataPin, pinState);
    // register shifts bits on upstroke of clock pin  
    digitalWrite(myClockPin, 1);
    // zero the data pin after shift to prevent bleed through
    digitalWrite(myDataPin, 0);
  }

  // stop shifting
  digitalWrite(myClockPin, 0);
}
