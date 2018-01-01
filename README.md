# OnkyoRI2mIR
Arduino Project - read Onkyo HiFi RI codes and translate them to Maratz CD InfraRed Commands
Tried out with Arduino Uno (but probably works with other models as well)

## Background
Modern HiFi equipment often comes with the feature to control connected devices with the same remote.
E.g., the amplifier (infrared) remote includes commands like "Play" or "Stop" which has to be forwarded to the connected (CD) player. For this purpose, manufacturers add a send/receive interface (sometimes called RI for "Remote Interactive") to their 
devices which speak (sadly) proprietary protocols and tend to only operate with devices from the same manufactuer.

In this case, the amplifier is an Onkyo TX-8150 and the CD player is a Marantz CD6004, and they won't talk to each other.
The Onkyo RI protocol is simple and "known" (@cite), but Marantz did not reply to requests about their RI protocol.
However, the commands only need to go one way (amplifier -> cd player), so a simple Infrared LED can be used to mimic a 
normal Marantz remote control (for this the codes are available).
The Arduino controller reads the Onkyo IR protocol and translates it to Marantz Infrared (remote) code.
   
Requires the IRremote library, see    https://github.com/z3t0/Arduino-IRremote


## Hardware Setup
@todo
