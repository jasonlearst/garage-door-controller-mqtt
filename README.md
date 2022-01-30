# MQTT Garage Door Controller

This repository contains the source code for a MQTT Garage Door Controller built on the Particle Photon controller.

## Reusing this implementation

Rename the ```_secrets.h``` template to ```secrets.h``` by removing the leading underscore.  Fill in the correct information for your MQTT broker.  Add the code contained in ```configuration.yml``` to your Home Assistant configuration to connect Home Assistant to the controller.

```yaml
cover:
  - platform: mqtt
    name: "garage"
    device_class: garage
    state_topic: "garage-cover/door/status"
    command_topic: "garage-cover/door/action"
    availability_topic: "garage-cover/availability"
```

## Inspiration for this project

This code is adapted from the instructables guide [DIY Smart Garage Door Opener + Home Assistant Integration](https://www.instructables.com/DIY-Smart-Garage-Door-Opener-Using-ESP8266-Wemos-D/).  You can find the original authors code at [https://gitlab.com/MrDIYca/garage-door-opener-mqtt](https://gitlab.com/MrDIYca/garage-door-opener-mqtt).