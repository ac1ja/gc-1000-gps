// byte buildStatusData(bool AMLED, bool PMLED, bool hiSpecLED, bool captureLED, bool mhz15LED, bool mhz10LED, bool mhz5LED, bool dataLED) {
//   byte statusByte = 0;

//   statusByte = ((AMLED == true) ? 1 : 0);
//   statusByte += ((PMLED == true) ? 2 : 0);
//   statusByte += ((hiSpecLED == true) ? 4 : 0);
//   statusByte += ((captureLED == true) ? 8 : 0);
//   statusByte += ((mhz15LED == true) ? 16 : 0);
//   statusByte += ((mhz10LED == true) ? 32 : 0);
//   statusByte += ((mhz5LED == true) ? 64 : 0);
//   statusByte += ((dataLED == true) ? 128 : 0);

//   return (statusByte);
// }

// byte buildTimeData(byte digit, byte position) {
//   return(digit + (position << 4));
// }
