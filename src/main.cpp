#include <Arduino.h>
#include <Wire.h>

#define SDA 6
#define SCL 7
#define main_register 0x68 //MPU 6050 SIGNAL_PATH_RESET register
#define wake_register 0x6B //PWR_MGMT_1 register
#define sensor_start_register 0x3B //ACCEL_XOUT_H register. All other ACCEL + GYRO registers follw immediately after

// put function declarations here:
int write_byte(uint8_t dev_address, uint8_t reg_address, const uint8_t *data, size_t length); //Write a single byte to peripheral register
int write_burst(uint8_t dev_address, uint8_t reg_address, const uint8_t *data, size_t length); //Write a specified number of bytes to multiple adjaescent peripheral registers
int read_byte(uint8_t dev_address, uint8_t reg_addess); //Read a byte from single peripheral register
int read_burst(uint8_t dev_address, uint8_t reg_address, size_t length, uint8_t *buffer); //Read a specified number of bytes from multiple adjaescent peripheral registers

void setup() {
  Wire.begin(SDA, SCL); //Initialize I2C as controller
  Serial.begin(115200);

  uint8_t wake_cmd = 0X00;
  write_byte(main_register, wake_register, &wake_cmd, 1); //Deactivate sleep mode by writing to PWR_MGMT_1 register
}

void loop() {
  
  uint8_t IMU_buffer[14];
  
  if (read_burst(main_register, sensor_start_register, 14, IMU_buffer) == 0){
    
    for(size_t i = 0; i < 14; i++){
      Serial.println(IMU_buffer[i]);
    }
  }
  
  Serial.println("-------------------------------------------")
  delay(1000); //Poll every second
}

// put function definitions here:
int write_byte(uint8_t dev_address, uint8_t reg_address, const uint8_t *data, size_t length) {
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

int read_byte(uint8_t dev_address, uint8_t reg_address) {
  
  //Empty write to set peripheral's internal memory pointer to desired register
  Wire.beginTransmission(dev_address);
  Wire.write(reg_address);
  Wire.endTransmission();

  Wire.requestFrom(dev_address, 1); //Request byte from peripheral
  
  if (Wire.available()){
  return(Wire.read()); //Pass the received byte back to main function
  }

  return -1; //Error check
}

int read_burst(uint8_t dev_address, uint8_t reg_address, size_t length, uint8_t *buffer) {
  
  //Empty write to set peripheral's internal memory pointer to desired start register
  Wire.beginTransmission(dev_address);
  Wire.write(reg_address);
  
  if (Wire.endTransmission() != 0){ //Check that Wire.endTransmission is successful i.e. returns 0
    return -1; //Transmission error
  }

  size_t bytes_received = Wire.requestFrom(dev_address, length);
  if(bytes_received != length){ //Check that the bytes available on I2C bus match the reqested burst length
    return -2; //Length mismatch error
  }

  //Loop through wire read 
  for(size_t i = 0; i < length; i++){
    if (Wire.available()){
      buffer[i] = Wire.read();
    }
  }

  return 0; //Successful read
}