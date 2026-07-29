#include <Arduino.h>
#include <Wire.h>

// put function declarations here:
int write_register(uint8_t dev_address, uint8_t reg_address, const uint8_t *data, size_t length);
int read_register_byte(uint8_t dev_address, uint8_t reg_address);

void setup() {
  Wire.begin(4, 5); //Initialize I2C as controller (SDA, SCL)
  Serial.begin(115200);
}

void loop() {
  //WHO_AM_I testing

  int test_byte = read_register_byte(104, 118); //Run helper function to access WHO_AM_I memory address
  Serial.println(test_byte, HEX);

  delay(1000); //Poll once per second
}

// put function definitions here:
size_t write_register(uint8_t dev_address, uint8_t reg_address, const uint8_t *data, size_t length) {
  Wire.beginTransmission(dev_address); //Initialize comms with the peripheral's main chip
  Wire.write(reg_address); //Tell the peripheral's chip which register to write to

  Wire.write(data, length); //Send data array to the register
  return Wire.endTransmission(); //End transmission sequence

  /* 
  Return value guide:
  0: Success
  1: Transmit buffer overflow
  2: NACK on transmit of address
  3: NACK on transmit of data
  4: misc. error
  5: Timeout error
  */
}

int read_register_byte(uint8_t dev_address, uint8_t reg_address) {
  
  //Empty write to set peripheral's memory pointer to desired register
  Wire.beginTransmission(dev_address);
  Wire.write(reg_address);
  Wire.endTransmission();

  Wire.requestFrom(dev_address, 1); //Request byte from peripheral
  
  //Read bytes as long as they are available and there aren't more than the requested amount
  if (Wire.available()){
  return(Wire.read()); //Pass the received byte back to main function
  }

  return -1; //Error check
}