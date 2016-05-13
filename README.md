# tea-alarm
Arduino/ESP8266 tea temperature alarm

Arduino sketch for my WiFi enabled temperature sensing tea alarm project. The set-up employs a MLX9015 contactless thermopile to measure the temperature of a cup of tea, and sending data to thingspeak.com with an ESP8266-01 module (AT commands). When the temperature drops below a set threshold (stored in EEPROM) an audio alarm is sounded and SMS message sent to a designated number via ThingHTTP and Twilio. When the temperature drops below a lower threshold, the status is tweeted via ThingTweet. 

The threshold temperature is set via a programming mode that is initiated with a sub-zero temperature object and then increased or decreased with hot and cold objects respecively.

For more information see: https://hackaday.io/project/10677-eye-o-tea
