#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/spi/spi.h"
#include <SPI.h>

namespace esphome {
namespace lora_receiver {

// Must match the struct in CubeCell
struct MeterData {
  float power_watts;
  float total_consumption_kwh;
  float total_generation_kwh;
  float battery_voltage;
  uint32_t packet_counter;
} __attribute__((packed));

class LoRaReceiverComponent : public Component, public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_8MHZ> {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  
  void set_dio1_pin(uint8_t pin) { dio1_pin_ = pin; }
  void set_rst_pin(uint8_t pin) { rst_pin_ = pin; }
  void set_busy_pin(uint8_t pin) { busy_pin_ = pin; }
  
  void set_power_sensor(sensor::Sensor *sensor) { power_sensor_ = sensor; }
  void set_consumption_sensor(sensor::Sensor *sensor) { consumption_sensor_ = sensor; }
  void set_generation_sensor(sensor::Sensor *sensor) { generation_sensor_ = sensor; }
  void set_battery_sensor(sensor::Sensor *sensor) { battery_sensor_ = sensor; }
  void set_rssi_sensor(sensor::Sensor *sensor) { rssi_sensor_ = sensor; }
  void set_snr_sensor(sensor::Sensor *sensor) { snr_sensor_ = sensor; }
  void set_packet_counter_sensor(sensor::Sensor *sensor) { packet_counter_sensor_ = sensor; }
  void set_missed_packets_sensor(sensor::Sensor *sensor) { missed_packets_sensor_ = sensor; }
  
  uint32_t seconds_since_last_packet() const {
    return (millis() - last_packet_time_) / 1000;
  }

 protected:
  uint8_t dio1_pin_;
  uint8_t rst_pin_;
  uint8_t busy_pin_;
  
  sensor::Sensor *power_sensor_{nullptr};
  sensor::Sensor *consumption_sensor_{nullptr};
  sensor::Sensor *generation_sensor_{nullptr};
  sensor::Sensor *battery_sensor_{nullptr};
  sensor::Sensor *rssi_sensor_{nullptr};
  sensor::Sensor *snr_sensor_{nullptr};
  sensor::Sensor *packet_counter_sensor_{nullptr};
  sensor::Sensor *missed_packets_sensor_{nullptr};
  
  uint32_t last_packet_counter_ = 0;
  uint32_t missed_packets_ = 0;
  uint32_t last_packet_time_ = 0;
  bool first_packet_ = true;
  
  // SX1262 registers and commands
  static constexpr uint8_t CMD_SET_STANDBY = 0x80;
  static constexpr uint8_t CMD_SET_RX = 0x82;
  static constexpr uint8_t CMD_SET_FS = 0x01;
  static constexpr uint8_t CMD_SET_PACKET_TYPE = 0x8A;
  static constexpr uint8_t CMD_SET_RF_FREQUENCY = 0x86;
  static constexpr uint8_t CMD_SET_MODULATION_PARAMS = 0x8B;
  static constexpr uint8_t CMD_SET_PACKET_PARAMS = 0x8C;
  static constexpr uint8_t CMD_SET_DIO_IRQ_PARAMS = 0x08;
  static constexpr uint8_t CMD_GET_IRQ_STATUS = 0x12;
  static constexpr uint8_t CMD_CLEAR_IRQ_STATUS = 0x02;
  static constexpr uint8_t CMD_GET_RX_BUFFER_STATUS = 0x13;
  static constexpr uint8_t CMD_READ_BUFFER = 0x1E;
  static constexpr uint8_t CMD_GET_PACKET_STATUS = 0x14;
  static constexpr uint8_t CMD_SET_REGULATOR_MODE = 0x96;
  static constexpr uint8_t CMD_SET_BUFFER_BASE_ADDRESS = 0x8F;
  static constexpr uint8_t CMD_SET_LORA_SYMB_NUM_TIMEOUT = 0xA0;
  
  static constexpr uint8_t PACKET_TYPE_LORA = 0x01;
  static constexpr uint16_t IRQ_RX_DONE = 0x02;
  static constexpr uint16_t IRQ_ALL = 0x3FF;
  
  void reset_module();
  void wait_busy();
  void write_command(uint8_t cmd, const uint8_t *data = nullptr, size_t len = 0);
  void read_command(uint8_t cmd, uint8_t *data, size_t len);
  bool init_lora();
  void receive_packet();
  void start_receive();
  uint16_t get_irq_status();
  void clear_irq_status(uint16_t irq);
};

}  // namespace lora_receiver
}  // namespace esphome