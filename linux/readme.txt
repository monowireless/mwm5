[reference build environment]
    Ubuntu 18.04.

[required package (building)]
    - build-essentials
    - gcc-9, g++-9

    - libsdl2-dev
    - D2XX (included, https://ftdichip.com/)

[Compile Options for make]
  DEBUG_BUILD=1
    -> build debug binary

[removing ftdi VCP driver]
  The standard driver ftdi_sio conflicts with D2XX driver.
 
    - Unload device driver manually and run TWELITE_Stage.
        If you manually unload FTDI device, use the command:
        $ sudo rmmod ftdi_sio
        $ sudo rmmod usbserial

        then run TWELITE_Stage with root permission like:
        $ sudo ./TWELITE_Stage.run

    - Using UDEV to automate unloading and setting permission.
      This procedure will stop loading standard FTDI driver and put user's permission 
      to use the USB devices.
      NOTE: ONCE THIS SETTING PERFORMED, THE OTHER APPLICATION CAN NOT ACCESS TWELITE
            USB DEVICE (MONOSTICK, TWELITE-R) DUE TO THE ABSENSE OF THE STANDARD DRIVER.

      1. add udev entry (e.g. /etc/udev/rules.d/51-ftdi.rules) as below:
        ACTION=="add", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", MODE="0666",
            RUN+="/bin/sh -c 'rmmod ftdi_sio && rmmod usbserial'"

      2. then reboot system or restart udev as shown below:
        $ sudo udevadm control --reload-rules
        $ sudo udevadm trigger

      3. Now plug MONOSTICK or TWELITE LITE-R and TWELITE Stage will recoginize the
         the device through FTDI serial number of 8 digits characters.

