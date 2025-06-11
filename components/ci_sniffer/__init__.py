from esphome.components import uart, text_sensor
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_UART_ID

CONF_FRAME_SENSOR = "frame_sensor"

ci_sniffer_ns = cg.esphome_ns.namespace("ci_sniffer")
CISnifferComponent = ci_sniffer_ns.class_("CISnifferComponent", cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(CISnifferComponent),
    cv.GenerateID(CONF_UART_ID): cv.use_id(uart.UARTComponent),
    cv.Optional(CONF_FRAME_SENSOR): cv.use_id(text_sensor.TextSensor),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], await cg.get_variable(config[CONF_UART_ID]))
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_FRAME_SENSOR in config:
        sens = await cg.get_variable(config[CONF_FRAME_SENSOR])
        cg.add(var.set_text_sensor(sens))