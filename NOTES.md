# Notes

## transition from Peep Hatch Configure state to other states

Push button could be used to wake Peep up when it is in the Hatch Configure
state. When in Hatch Configure state, the Peep could turn on a LED to indicate
it is awake. If user presses button when it is awake, Peep could interpret the
action to mean that the User wants to re-pair over BLE.

## WiFi polling

Would probably be a good idea to set up the WiFi code to work in such a way
that there is a function that performs connection polling. Something like the
following below...
```
wifi_init(ssid, pass);
while (is_normal_operation && (false == is_wifi_connected)) {
  is_wifi_connected = wifi_poll_connected();

  if (some_other_event()) {
    is_normal_operation = false;
  }
}
```

