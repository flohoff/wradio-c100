
This is a quick hack to get the OMRI-USB repository to work on non Android.

The OMRI-USB is the standard layer for Android DAB(+) (Digital Audio Broadcast)
enabled receivers e.g. Car Stereos based on Android being enabled with DAB.

The cheap DAB(+) receivers sold which are supported here are based on
the wRadio C100 chipsets. I could not find the Vendor nor the Chipset
on the Internet, but from debugging in an Android Emulator i found
initialisation sequences which let me to find the OMRI-USB codebase.

I simply removed all JAVA JNI callback stuff and glued my own simple USB::Device
class into the Receiver Code.

This works up to the point where i do see the FIC and MSC and i am being
handed the raw frames of the Packet Data i am interested in which is 
the PPP-RTK-AdV GPS corrections Data Stream on the Germany Bundesmux Channel 5C.

You will need

    apt-get install libusb-1.0.0-dev cmake build-essentials
    cmake .
    make
