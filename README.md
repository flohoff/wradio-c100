
This repository contains code for the wRadio C100 DAB USB Radios 
being sold cheap on Amazon, Ebay etc as "Android Only".

It implements most of the DAB(+) Stack to enable receiving the
"PPP-RTK-AdV" Packet Data Stream containing SSRZ correction data.

If you run this the code will initialize the wRadio USB DAB Receiver,
tune to the "Bundesmux" on Channel 5C and starts to decode the Data Packet
only Subchannel 32.

It will apply FEC (Forward Error Correction) on packets received, and pass
on the RTCM frames to an NTRIP Caster on localhost. 

Currently everything is hardcoded.

    * Channel 5c (178.352MHz)
    * Ensemble 0x10bc
    * Service ID 0xe0d210bc (PPP-RTK-AdV)
    * NTRIP Caster on Localhost Port 2101
    * Mountpoint ZZ-DAB-SSRZ

The origin of this code is https://github.com/hradio/omri-usb.git which is 
the original Code of the IRT (Institut für Radiotechnik) and written
by Fabian Sattler. The original code is the bases for the OMRI-USB
default DAB(+) Layer in Android used a lot in Android based Car Stereos.

The cheap DAB(+) receivers sold which are supported here are based on
the wRadio C100 chipsets. I could not find the Vendor nor the Chipset
on the Internet, but from debugging in an Android Emulator i found
initialisation sequences which let me to find the OMRI-USB codebase.

I simply removed all JAVA JNI callback stuff and glued my own simple USB::Device
class into the Receiver Code.

You will need

    apt-get install libusb-1.0.0-dev cmake build-essentials
    cmake .
    make


TODO
====

* Reconnect/Restart/die on USB Disconnect
* fic/fib parsing may get out of sync and never finds sync
* Rare segfault in FEC code overwriting the Stack
* Reconnect NTRIP Caster
* Correctly parse NTRIP responses, support Authentication etc
* Configurable settings by command line

