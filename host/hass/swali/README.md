# VSCP

 - `manifest.json`: update the requirements to point at your Python library
 - `light.py`: update the code to interact with your library

### Installation

Copy this folder to `<config_dir>/custom_components/example_light/`.

Add the following entry in your `configuration.yaml`:

```yaml
light:
  - platform: vscp_light
    host: HOST_HERE
    port: PORT_HERE
    username: USERNAME_HERE
    password: PASSWORD_HERE_OR_secrets.yaml
```
