#include <Arduino.h>
#include <Wire.h>

#define SDA 6
#define SCL 7

#define main_register 0x68 //MPU 6050 SIGNAL_PATH_RESET register address
#define wake_register 0x6B //PWR_MGMT_1 register address
#define accel_start_register 0x3B //ACCEL_XOUT_H register address
#define gyro_start_register 0x43 //GYRO_XOUT_H register address

// put function declarations here:
int write_byte(uint8_t dev_address, uint8_t reg_address, const uint8_t *data); //Write a single byte to peripheral register
int write_burst(uint8_t dev_address, uint8_t reg_address, const uint8_t *data, size_t length); //Write a specified number of bytes to multiple adjaescent peripheral registers
int read_byte(uint8_t dev_address, uint8_t reg_address, uint8_t *buffer); //Read a byte from single peripheral register
int read_burst(uint8_t dev_address, uint8_t reg_address, size_t length, uint8_t *buffer); //Read a specified number of bytes from multiple adjaescent peripheral registers

void process_IMU(struct IMU data);

void setup() {
  Wire.begin(SDA, SCL); //Initialize I2C as controller
  Serial.begin(115200);

  uint8_t wake_cmd = 0X00;
  write_byte(main_register, wake_register, &wake_cmd); //Deactivate sleep mode by writing to PWR_MGMT_1 register

  uint8_t IMU_buffer[14];
  
  struct IMU {
    float accel_x, accel_y, accel_z; 
    float gyro_x, gyro_y, gyro_z;
  };
}

void loop() {

}

// put function definitions here:
int write_burst(uint8_t dev_address, uint8_t reg_address, const uint8_t *data, size_t length) {
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

int write_byte(uint8_t dev_address, uint8_t reg_address, const uint8_t *data){
  return write_burst(dev_address, reg_address, data, 1);
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

int read_byte(uint8_t dev_address, uint8_t reg_address, uint8_t *buffer) {
  return read_burst(dev_address, reg_address, 1, buffer);
}

