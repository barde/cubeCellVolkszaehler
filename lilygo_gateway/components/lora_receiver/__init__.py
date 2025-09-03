import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_SIGNAL_STRENGTH,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_WATT,
    UNIT_VOLT,
    UNIT_DECIBEL_MILLIWATT,
    UNIT_DECIBEL,
)

DEPENDENCIES = ["spi"]
AUTO_LOAD = ["sensor"]

lora_receiver_ns = cg.esphome_ns.namespace("lora_receiver")
LoRaReceiverComponent = lora_receiver_ns.class_(
    "LoRaReceiverComponent", cg.Component
)

CONF_POWER = "power"
CONF_CONSUMPTION = "consumption"
CONF_GENERATION = "generation"
CONF_BATTERY = "battery"
CONF_RSSI = "rssi"
CONF_SNR = "snr"
CONF_PACKET_COUNTER = "packet_counter"
CONF_MISSED_PACKETS = "missed_packets"
CONF_CS_PIN = "cs_pin"
CONF_DIO1_PIN = "dio1_pin"
CONF_RST_PIN = "rst_pin"
CONF_BUSY_PIN = "busy_pin"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LoRaReceiverComponent),
        cv.Required(CONF_DIO1_PIN): cv.int_,
        cv.Required(CONF_RST_PIN): cv.int_,
        cv.Required(CONF_BUSY_PIN): cv.int_,
        cv.Optional(CONF_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_CONSUMPTION): sensor.sensor_schema(
            unit_of_measurement="kWh",
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            accuracy_decimals=3,
        ),
        cv.Optional(CONF_GENERATION): sensor.sensor_schema(
            unit_of_measurement="kWh",
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            accuracy_decimals=3,
        ),
        cv.Optional(CONF_BATTERY): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=2,
        ),
        cv.Optional(CONF_RSSI): sensor.sensor_schema(
            unit_of_measurement=UNIT_DECIBEL_MILLIWATT,
            device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_SNR): sensor.sensor_schema(
            unit_of_measurement=UNIT_DECIBEL,
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=1,
        ),
        cv.Optional(CONF_PACKET_COUNTER): sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_MISSED_PACKETS): sensor.sensor_schema(
            state_class=STATE_CLASS_MEASUREMENT,
            accuracy_decimals=0,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    # Configure pins
    cg.add(var.set_dio1_pin(config[CONF_DIO1_PIN]))
    cg.add(var.set_rst_pin(config[CONF_RST_PIN]))
    cg.add(var.set_busy_pin(config[CONF_BUSY_PIN]))
    
    # Register sensors
    if CONF_POWER in config:
        sens = await sensor.new_sensor(config[CONF_POWER])
        cg.add(var.set_power_sensor(sens))
    
    if CONF_CONSUMPTION in config:
        sens = await sensor.new_sensor(config[CONF_CONSUMPTION])
        cg.add(var.set_consumption_sensor(sens))
    
    if CONF_GENERATION in config:
        sens = await sensor.new_sensor(config[CONF_GENERATION])
        cg.add(var.set_generation_sensor(sens))
    
    if CONF_BATTERY in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY])
        cg.add(var.set_battery_sensor(sens))
    
    if CONF_RSSI in config:
        sens = await sensor.new_sensor(config[CONF_RSSI])
        cg.add(var.set_rssi_sensor(sens))
    
    if CONF_SNR in config:
        sens = await sensor.new_sensor(config[CONF_SNR])
        cg.add(var.set_snr_sensor(sens))
    
    if CONF_PACKET_COUNTER in config:
        sens = await sensor.new_sensor(config[CONF_PACKET_COUNTER])
        cg.add(var.set_packet_counter_sensor(sens))
    
    if CONF_MISSED_PACKETS in config:
        sens = await sensor.new_sensor(config[CONF_MISSED_PACKETS])
        cg.add(var.set_missed_packets_sensor(sens))