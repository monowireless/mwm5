[reference environment]
    macOS 10.14 Mojave (gcc 9.2)
    macOS 10.15 Catalina (gcc 9.3 or clang)

[required package (building)]
    - brew
    - gcc-9, g++-9 (if build with gcc)

    included in lib directory.
    - sdl2
    - sdl2_net
    - D2XX (https://ftdichip.com/)

[FTDI D2XX driver]
    In order to access an FTDI usb chip via D2XX interface by the application,
    the default usbserial driver needs to be unloaded in advance.

    Remove manually, 
      $ sudo kextunload -b com.apple.driver.AppleUSBFTDI
    
    or

    Install D2xxHelper (find from FTDI website or lib/FTDI/D2xxHelper_v???.pkg)
    to supress default driver loading. Once installed, usb driver won't be loaded
    if the device has default vendor/product-id.

[Compile Options for make]
  OSX_COMPILERTYPE=clang
    -> build with clang (required macOS 10.15)
  DEBUG_BUILD=1
    -> build debug binary
      