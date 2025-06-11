import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, text_sensor
from esphome.const import CONF_ID

lin_sniffer_ns = cg.esphome_ns.namespace("lin_sniffer")
LINSnifferComponent = lin_sniffer_ns.class_("LINSnifferComponent", cg.Component, uart.UARTDevice)

CONF_SENSOR = "text_sensor"

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(LINSnifferComponent),
        cv.Optional(CONF_SENSOR): cv.use_id(text_sensor.TextSensor),
    })
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], await cg.get_variable(config["uart_id"]))
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_SENSOR in config:
        sens = await cg.get_variable(config[CONF_SENSOR])
        cg.add(var.set_text_sensor(sens))