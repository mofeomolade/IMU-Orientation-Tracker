#include <Arduino.h>
#include <Wire.h>

#define SDA 6
#define SCL 7

#define main_register 0x68 //MPU 6050 SIGNAL_PATH_RESET register address
#define wake_register 0x6B //PWR_MGMT_1 register address
#define accel_start_register 0x3B //ACCEL_XOUT_H register address
#define gyro_start_register 0x43 //GYRO_XOUT_H register address
#define wake_cmd 0X00

#define RAD_TO_DEGREES 180.0/PI
#define GYRO_DEADBAND 0.3

struct IMU {
  float accel_x, accel_y, accel_z; 
  float gyro_x, gyro_y, gyro_z;
};

struct Offset {
  float offset_accel_x, offset_accel_y, offset_accel_z; 
  float offset_gyro_x, offset_gyro_y, offset_gyro_z;
};

struct Attitude{
  float pitch;
  float roll;
  float yaw; 
};

// put function declarations here:
int write_byte(uint8_t dev_address, uint8_t reg_address, const uint8_t *data); //Write a single byte to peripheral register
int write_burst(uint8_t dev_address, uint8_t reg_address, const uint8_t *data, size_t length); //Write a specified number of bytes to multiple adjaescent peripheral registers
int read_byte(uint8_t dev_address, uint8_t reg_address, uint8_t *buffer); //Read a byte from single peripheral register
int read_burst(uint8_t dev_address, uint8_t reg_address, size_t length, uint8_t *buffer); //Read a specified number of bytes from multiple adjaescent peripheral registers
int read_IMU(IMU &data);
void offset_test(IMU &data, Offset &calibration);
void correct_IMU (IMU &data, Offset &calibration);
void filter_IMU (IMU &data, Attitude &orientation, float dt);

struct IMU imu; //Global IMU struct to be used in all processing after calibration
struct Offset offset; //Global Offset struct to hold stationary IMU drift values
struct Attitude attitude; //Global Attitude struct to hold position angles

unsigned long last_time = micros();
unsigned long last_print = millis();

void setup() {
  Wire.begin(SDA, SCL); //Initialize I2C as controller
  Serial.begin(115200);

  write_byte(main_register, wake_register, &wake_cmd); //Deactivate sleep mode by writing to PWR_MGMT_1 register

  offset_test(imu, offset); //Run initial baseline offset test and store

  //Seeding for accurate readings in main loop()
  if(read_IMU(imu) == 0){
    correct_IMU(imu, offset);

    //Initial orientation calculation so starting position reflects actual orientation
    attitude.pitch = atan2(-imu.accel_x, sqrt( imu.accel_y * imu.accel_y+ imu.accel_z * imu.accel_z)) * RAD_TO_DEGREES;
    attitude.roll = atan2(imu.accel_y, imu.accel_z) * RAD_TO_DEGREES;
    attitude.yaw = 0.0; //Yaw set to zero with respect to initial position

    last_time = micros();
  }
}

void loop() {
  //Calculate dt in seconds using last_time and current_time
  unsigned long current_time = micros();
  float dt = (current_time - last_time) / 1000000.0;
  last_time = current_time;

  if(read_IMU (imu) == 0){
    //Apply offset to IMU readings
    correct_IMU(imu, offset);

    //Apply complementary filter to IMU readinds and pass to attitude struct
    filter_IMU(imu, attitude, dt);

    if(millis() - last_print > 350){
      //Display pitch, roll and yaw to serial monitor
      Serial.print("Pitch: ");
      Serial.print(attitude.pitch);
      Serial.print(",");
      Serial.print(" Roll: ");
      Serial.print(attitude.roll);
      Serial.print(",");
      Serial.print(" Yaw: ");
      Serial.println(attitude.yaw);
      Serial.println();

      last_print = millis();
    }
  }
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
  return read_burst(dev_address, reg_address, 1, buffer); //Call read_burst length 1
}

int read_IMU(IMU &data) {
  uint8_t raw_buffer[14]; //Buffer to hold all accelerometer and gyroscope data
  
  //Check that the read
  if(read_burst(main_register, accel_start_register, 14, raw_buffer) == 0) {
    //Combine paired 8-bit accelerometer register readings from Big-Endian to 16-bit
    int16_t raw_accel_x = ((int16_t)raw_buffer[0] << 8) | raw_buffer[1];
    int16_t raw_accel_y = ((int16_t)raw_buffer[2] << 8) | raw_buffer[3];
    int16_t raw_accel_z = ((int16_t)raw_buffer[4] << 8) | raw_buffer[5];

    //Combine paired 8-bit gyroscope register readings from Big-Endian to 16-bit
    int16_t raw_gyro_x = ((int16_t)raw_buffer[8] << 8)  | raw_buffer[9];
    int16_t raw_gyro_y = ((int16_t)raw_buffer[10] << 8) | raw_buffer[11];
    int16_t raw_gyro_z = ((int16_t)raw_buffer[12] << 8) | raw_buffer[13];

    //Convert accelerometer readings to acceleration value G-force
    data.accel_x = raw_accel_x/16384.0;
    data.accel_y = raw_accel_y/16384.0;
    data.accel_z = raw_accel_z/16384.0;

    //Convert gyroscope readings to angular velocity value in degrees/second
    data.gyro_x = raw_gyro_x/131.0;
    data.gyro_y = raw_gyro_y/131.0;
    data.gyro_z = raw_gyro_z/131.0;

    return 0; //Successful IMU reading
  }
  return -1;
}

void offset_test(IMU &data, Offset &calibration){

  //Set up variables to hold the total offsets for a calibration test
  float sum_accel_x = 0, sum_accel_y = 0, sum_accel_z = 0;
  float sum_gyro_x = 0, sum_gyro_y = 0, sum_gyro_z = 0; 
  float sample_size = 1000; 

  //Loop 1000x to get a reasoble offset average
  for(int i = 0; i < 1000; i++){
    read_IMU(data); //Call read_IMU

    //Track the total accelerometer offset across 1000 runs
    sum_accel_x += data.accel_x;
    sum_accel_y += data.accel_y;
    sum_accel_z += data.accel_z - 1.0;
    
    //Track the total gyroscope offset across 1000 rus
    sum_gyro_x += data.gyro_x;
    sum_gyro_y += data.gyro_y;
    sum_gyro_z += data.gyro_z;
  }

  //Take the accelerometer drift avg over all runs
  calibration.offset_accel_x = sum_accel_x/sample_size;
  calibration.offset_accel_y = sum_accel_y/sample_size;
  calibration.offset_accel_z = sum_accel_z/sample_size;

  //Take the gyroscope drift average over all runs
  calibration.offset_gyro_x = sum_gyro_x/sample_size;
  calibration.offset_gyro_y = sum_gyro_y/sample_size;
  calibration.offset_gyro_z = sum_gyro_z/sample_size;
}

void correct_IMU (IMU &data, Offset &calibration){
  //Subtract offsets from IMU readings
  data.accel_x -= calibration.offset_accel_x;
  data.accel_y -= calibration.offset_accel_y;
  data.accel_z -= calibration.offset_accel_z;

  data.gyro_x -= calibration.offset_gyro_x;
  data.gyro_y -= calibration.offset_gyro_y;
  data.gyro_z -= calibration.offset_gyro_z;

  //Deadband filter
  //Angular acceleration must have large enough magnitude to filter out gyroscope noise 
  if (abs(data.gyro_x) < GYRO_DEADBAND) data.gyro_x = 0.0;
  if (abs(data.gyro_y) < GYRO_DEADBAND) data.gyro_y = 0.0;
  if (abs(data.gyro_z) < GYRO_DEADBAND) data.gyro_z = 0.0;
}

void filter_IMU (IMU &data, Attitude &orientation, float dt){
  //Complementary filter function: Angle(t) = alpha * [Angle(t-1) + Gyro_angle * delta_t] + (1 - alpha) * Accelerometer_angle
  
  const float alpha = 0.96; //96% weight in gyro reading, 4% weight in accelerometer reading

  //Use trig to calculate pitch and roll based on accelerometer readings
  float accel_pitch = atan2(-data.accel_x, sqrt( data.accel_y * data.accel_y + data.accel_z * data.accel_z )) * RAD_TO_DEGREES;
  float accel_roll = atan2(data.accel_y, data.accel_z) * RAD_TO_DEGREES;

  //Combine accelerometer trig calculations with raw gyroscope readings
  orientation.pitch = alpha * (orientation.pitch + data.gyro_y *dt) + (1.0 - alpha) * accel_pitch;
  orientation.roll = alpha * (orientation.roll + data.gyro_x *dt) + (1.0 - alpha) * accel_roll;

  //Yaw bases only on gyro because rotation about z-axis doesn't affect accelerometer reading
  orientation.yaw += data.gyro_z * dt;
}