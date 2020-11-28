import logging
from homeassistant.components.binary_sensor import (BinarySensorEntity)
from homeassistant.const import STATE_OFF, STATE_ON

_LOGGER = logging.getLogger(__name__)

async def async_setup_platform(hass, config, async_add_entities, discovery_info=None):
    # Assign configuration variables.
    # The configuration check takes care they are present.
    #host = config[CONF_HOST]
    #username = config[CONF_USERNAME]
    #password = config.get(CONF_PASSWORD)
    if discovery_info is None:
        return

    # Setup connection with devices/cloud
    gw = hass.data['swali']

    switches = [x for x in gw.get_channels('IN') if x.enabled]

    # Add devices
    async_add_entities(SwaliBinarySensor(switch) for switch in gw.get_channels('IN'))
    return True


class SwaliBinarySensor(BinarySensorEntity):
    """Representation of an Swali Sensor (ie switch/button)."""

    def __init__(self, switch):
        self._sensor = switch
        self._name = '{}:{:02X}'.format(switch.guid, switch.channel)
        self._state = switch.state
        switch.add_callback(self._callback)

    @property
    def name(self):
        """Return the display name of this switch."""
        return self._name

    @property
    def is_on(self):
        return self._state

    @property
    def state(self):
        """Return the state of the sensor."""
        return STATE_ON if self._state else STATE_OFF

    async def _callback(self, switch, state):
        self._state = state
        self.async_schedule_update_ha_state()

    @property
    def should_poll(self):
        return False
