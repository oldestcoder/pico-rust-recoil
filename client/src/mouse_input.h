#pragma once

namespace MouseInput {

// Opens the USB mouse bridge using the COM port in bridge_port.txt.
// If the file is absent or empty, COM5 is used.
bool connect();
void disconnect();
bool isConnected();

// Sends a relative movement through the bridge's `M dx dy\n` protocol.
// Values outside the firmware's signed 8-bit range are split into valid
// commands without changing the total displacement.
void moveRelative(int dx, int dy);

} // namespace MouseInput
