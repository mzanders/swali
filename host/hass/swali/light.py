import logging

from pyswali import gateway, light

from homeassistant.const import EVENT_HOMEASSISTANT_STOP
from pyswali import Gateway, Light

from homeassistant.components.light import (Light)

_LOGGER = logging.getLogger(__name__)

async def async_setup_platform(hass, config, async_add_entities, discovery_info=None):
    # Assign configuration variables.
    # The configuration check takes care they are present.
    #host = config[CONF_HOST]
    #username = config[CONF_USERNAME]
    #password = config.get(CONF_PASSWORD)

    # Setup connection with devices/cloud
    gw = Gateway()
    await gw.connect()
    await gw.scan()

    async def on_hass_stop(event):
        """Close connection when hass stops."""
        await gw.close()
    hass.bus.async_listen_once(EVENT_HOMEASSISTANT_STOP, on_hass_stop)

    lights = [x for x in gw.get_channels('OU') if x.enabled]

    # Add devices
    async_add_entities(SwaliLight(light) for light in lights)

    await gw.start_update()

    return True


class SwaliLight(Light):
    """Representation of an Swali Light."""

    def __init__(self, light):
        """Initialize an AwesomeLight."""
        self._light = light
        self._name = light.name
        self._state = light.state
        light.add_callback(self._callback)

    @property
    def name(self):
        """Return the display name of this light."""
        return self._name

    @property
    def is_on(self):
        """Return true if light is on."""
        return self._state

    async def async_turn_on(self, **kwargs):
        """Instruct the light to turn on.
        You can skip the brightness part if your light does not support
        brightness control.
        """
        await self._light.set(True)

    async def async_turn_off(self, **kwargs):
        """Instruct the light to turn off."""
        await self._light.set(False)

    async def _callback(self, light, state):
        self._state = state
        self.async_schedule_update_ha_state()

    @property
    def should_poll(self):
        return False
