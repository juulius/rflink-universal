# Security

This experimental firmware is **not production-hardened**. The custom web interface currently has no authentication; restrict it to a trusted LAN. Do not forward its HTTP port to the internet, publish internal logs containing device IDs or secrets, or assume that a firewall compensates for every application weakness. Keep MQTT and transmit functions disabled unless required and configured deliberately.

When a public repository exists, report potentially exploitable problems privately via its security advisories rather than posting working exploit details or credentials in a public Issue. There is no guaranteed response time for this hobby project. If you distribute or sell hardware, additional security and product obligations may apply.
