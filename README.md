# OnkyoRI2mIR
Arduino Project - read Onkyo HiFi RI codes and translate them to Maratz CD InfraRed Commands
Tried out with Arduino Uno (but probably works with other models as well).

It can be quite easily adjusted to output codes for other IR remotes instead.

## Background
Modern HiFi equipment often comes with the feature to control connected devices with the same remote.
E.g., the amplifier (infrared) remote includes commands like "Play" or "Stop" which has to be forwarded to the connected (CD) player. For this purpose, manufacturers add a send/receive interface (sometimes called RI for "Remote Interactive") to their 
devices which speak (sadly) proprietary protocols and tend to only operate with devices from the same manufactuer.

In this case, the amplifier is an Onkyo TX-8150 and the CD player is a Marantz CD6004, and they won't talk to each other.
The Onkyo RI protocol is simple and "known" (https://github.com/docbender/Onkyo-RI.git), but Marantz did not reply to requests about their RI protocol.
However, the commands only need to go one way (amplifier -> cd player), so a simple Infrared LED can be used to mimic a 
normal Marantz remote control (for this the codes are available).
The Arduino controller reads the Onkyo IR protocol and translates it to Marantz Infrared (remote) code.
   
Requires the IRremote library, see    https://github.com/z3t0/Arduino-IRremote


## Hardware Setup

The program uses pin3 for output and pin4 for input. 
Make sure the resistors are appropriate to avoid possible hardware damage.

<pre>
    pin3 ---- (x) infrared LED ---[R1 = 330 OHM]----------- GND

    pin4 ---- o ----[R2 = 10 K OHM]------------------------ GND
              |
              |
              ...
              |
              |
              o Onkyo Amplifier RI port (middle cable of the 3.5mm mono phone jack)
</pre>
I also connected the GROUND of the 3.5 mono phone jack to the Adruino GND pin


Optionally, connect pin2 to a pushbutton (triggers "STOP", for debugging)
<pre>

                 /         
                / 
    pin2 ---- v     ------[R3 = 330 OHM] ------------------ GND
</pre>

## Disclaimer

THIS CODE IS FREE OF ANY WARRANTIES, USE (AND MODIFY) IT AT YOUR OWN RISK.

I realize that it is quite simplistic and specific (I'm new to Arduino programming); it solves a simple problem by simple means.
As long as you take care with the resitors, nothing bad will happen to your hardware.
 
Contact me if you have suggestions for improvement.
