TOUCH CONTROLLERS SUPPORTED
---------------------------

This Synaptics DSX driver supports both the DS4 and DS5 families of touch
controllers including the following.
   S21xx
   S22xx
   S23xx
   S27xx
   S32xx
   S33xx
   S34xx
   S35xx
   S36xx
   S70xx
   S73xx
   S75xx

Touch controllers based on the following communication protocols are supported.
   RMI over I2C
   RMI over SPI
   HID over I2C



WHAT'S IN THIS TARBALL
----------------------

The following files are provided in the "kernel" folder of this tarball.

drivers/input/touchscreen/synaptics_dsx/synaptics_dsx_core.[ch]
   The source code of the core driver containing support for 2D touch and 0D
   buttons.

drivers/input/touchscreen/synaptics_dsx/synaptics_dsx_i2c.c
   The source code of the I2C module used for supporting touch controllers
   based on the RMI over I2C protocol.

drivers/input/touchscreen/synaptics_dsx/synaptics_dsx_spi.c
   The source code of the SPI module used for supporting touch controllers
   based on the RMI over SPI protocol.

drivers/input/touchscreen/synaptics_dsx/synaptics_dsx_rmi_hid_i2c.c
   The source code of the HID over I2C (full vendor mode) module used for
   supporting touch controllers based on the HID over I2C protocol.

drivers/input/touchscreen/synaptics_dsx/synaptics_dsx_rmi_dev.c
   The source code of the RMI device module used to provide direct RMI register
   access from user space programs via a character device node or the sysfs
   interface.

drivers/input/touchscreen/synaptics_dsx/synaptics_dsx_fw_update.c
   The source code of the firmware update module used for doing both in-system
   and command-line reflash.

drivers/input/touchscreen/synaptics_dsx_active_pen.c
   The source code of the active pen module used to provide active pen
   functionalities.

drivers/input/touchscreen/synaptics_dsx_proximity.c
   The source code of the proximity module used to provide proximity
   functionalities.

[optional] drivers/input/touchscreen/synaptics_dsx_test_reporting.c
   The source code of the test reporting module used for retrieving production
   test reports.

[optional] drivers/input/touchscreen/synaptics_dsx_video.c
   The source code of the video module used for sending display DCS commands via
   RMI.

[optional] drivers/input/touchscreen/synaptics_dsx_debug.c
   The source code of the debug module used for supporting firmware debug
   functionalities.

drivers/input/touchscreen/synaptics_dsx/Kconfig
   Kconfig for the Synaptics DSX driver.

drivers/input/touchscreen/synaptics_dsx/Makefile
   Makefile for the Synaptics DSX driver.

firmware/Makefile
   Example Makefile for the OMAP Panda board to include a firmware image in the
   kernel build for doing firmware update during system power-on.

include/linux/input/synaptics_dsx.h
   The Synaptics DSX header file shared between the hardware description and
   the driver.

arch/arm/configs/panda_defconfig
   Example defconfig for the OMAP Panda board to include the Synaptics DSX
   driver.

arch/arm/configs/msm8974_defconfig
   Example defconfig for the Intrinsyc APQ8074 Dragon board to include the
   Synaptics DSX driver.

arch/arm/mach-omap2/board-omap4panda.c
   Example board file for the OMAP Panda board to include support for Synaptics
   touch controllers.

arch/arm/mach-omap2/board-omap4panda_original.c
   Original board file for the OMAP Panda board that does not include support
   for Synaptics touch controllers.

arch/arm/boot/dts/apq8074-dragonboard.dtsi
   Example Device Tree dtsi file for the Intrinsyc APQ8074 Dragon board to
   include support for Synaptics touch controllers.

arch/arm/boot/dts/apq8074-dragonboard_original.dtsi
   Original Device Tree dtsi file for the Intrinsyc APQ8074 Dragon board that
   does not include support for Synaptics touch controllers.

arch/arm/boot/dts/synaptics-dsx.dtsi
   Example Device Tree dtsi file for describing Synaptics touch controllers.



HOW TO INSTALL THE DRIVER
-------------------------

** Copy the synaptics_dsx folder in drivers/input/touchscreen to the
   equivalent directory in your kernel tree.

** Copy synaptics_dsx.h in include/linux/input to the equivalent directory
   in your kernel tree.

** Add the line below in your kernel tree's drivers/input/touchscreen/Makefile.
   obj-$(CONFIG_TOUCHSCREEN_SYNAPTICS_DSX) += synaptics_dsx/

** Add the line below in your kernel tree's drivers/input/touchscreen/Kconfig.
   source "drivers/input/touchscreen/synaptics_dsx/Kconfig"

** Update your defconfig by referring to the defconfig files in the tarball as
   examples.

** The following configuration options need to be enabled in the defconfig.
   CONFIG_TOUCHSCREEN_SYNAPTICS_DSX
   CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_CORE

** One of the following configuration options needs to be enabled in the
   defconfig based on the communication protocol of the touch controller.
   CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_I2C
   CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_SPI
   CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_RMI_HID_I2C

** If your platform uses board files for hardware description, update your
   board file by referring to the board files in the tarball as examples. The
   changes made in the board file for the OMAP Panda board to include support
   for Synaptics touch controllers can be listed by diff'ing the files below.
   arch/arm/mach-omap2/board-omap4panda.c
   arch/arm/mach-omap2/board-omap4panda_original.c

** If your platform uses Device Tree for hardware description, update your
   Device Tree by referring to the dtsi files in the tarball as examples. The
   changes made in the dtsi file for the Intrinsyc APQ8074 Dragon board to
   include support for Synaptics touch controllers can be listed by diff'ing
   the files below.
   arch/arm/boot/dts/apq8074-dragonboard.dtsi
   arch/arm/boot/dts/apq8074-dragonboard_original.dtsi

** "make clean" your kernel.

** "make" your defconfig.

** Rebuild your kernel.

** Install the new kernel image on your Android system.

** Reboot your Android system.



FIRMWARE UPDATE DURING SYSTEM POWER-ON
--------------------------------------

If the DO_STARTUP_FW_UPDATE macro in the synaptics_dsx_fw_update.c firmware
update module is enabled, the driver uses the kernel's request_firmware()
feature to obtain a default firmware image to do firmware update during system
power-on if necessary. The default firmware image is expected to live in
the file firmware/synaptics/startup_fw_update.img.ihex in the kernel source tree
during kernel build.

To convert the .img firmware image provided by Synaptics to the .ihex format,
use the command below.
   objcopy -I binary -O ihex <firmware_name>.img startup_fw_update.img.ihex

To inlcude the firmware image in the kernel build so that it can be used for
doing firmware update during system power-on, place the .ihex file obtained
above in the firmware/synaptics directory in your kernel source tree and make
sure the line below is included in firmware/Makefile. Note that the line below
is commented out by default in the example Makefile in the tarball.
   fw-shipped-$(CONFIG_TOUCHSCREEN_SYNAPTICS_DSX_FW_UPDATE) += synaptics/startup_fw_update.img



VIRTUAL BUTTONS
---------------

The example board file (board-omap4panda.c) and dtsi file (synaptics-dsx.dtsi)
contain support for virtual buttons. In the example board file, the virtual key
map is defined in the vir_button_codes data structure. In the example dtsi file,
the virtual key map is defined in the synaptics,vir-button-codes property.

Detailed information on the virtual kep map can be found in the link below.
   http://source.android.com/devices/tech/input/touch-devices.html#virtual-key-map-files

In addition to the virtual key map file, a key layout file also needs to be
created and placed in the /system/usr/keylayout directory in the Android file
system. The key layout file to match the example board file and dtsi file needs
to be named synaptics_dsx.kl and contain the following information.
   key 102 HOME VIRTUAL
   key 158 BACK VIRTUAL

Detailed information on the key layout file can be found in the link below.
   http://source.android.com/devices/tech/input/key-layout-files.html#virtual-soft-keys

When there are virtual buttons, the maximum sensor Y value may extend beyond
the area covered by the display. In this case, the maximum Y value for the
display (i.e. 2D touch) area is specified in the DSX_MAX_Y_FOR_2D macro in the
example board file and in the synaptics,max-y-for-2d property in the example
dtsi file. Note that this maximum Y value for the display area is to be
specified in sensor resolution units, not in display resolution units (unless
the two match).



MISCELLANEOUS OPTIONS
---------------------

Wakeup Gesture
** To enable wakeup gesture support, set the WAKEUP_GESTURE macro found in
   synaptics_dsx_core.c to true.

Microbootloader Mode I2C Slave Address
** TDDI solutions define an on-chip microbootloder due to the use of an external
   flash where the bootloader, UI firmware, and various configuration areas
   reside. The microbootloader may have a unique I2C slave address and is used
   during recovery when the external flash is blank or corrupt. The unique I2C
   slave address of the microbootloader may be defined with the DSX_UB_I2C_ADDR
   macro in the board file or with the synaptics,ub-i2c-addr property in the
   dtsi file.
