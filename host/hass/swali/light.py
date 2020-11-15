import logging
from homeassistant.components.light import (LightEntity)

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

    lights = [x for x in gw.get_channels('OU') if x.enabled]

    # Add devices
    async_add_entities(SwaliLight(light) for light in lights)
    return True


class SwaliLight(LightEntity):
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
