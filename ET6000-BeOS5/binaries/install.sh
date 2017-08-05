#!/bin/sh -f
echo This script installs the Tseng Labs ET6000, ET6100 and ET6300 driver files...
copyattr -d et6000.driver /boot/home/config/add-ons/kernel/drivers/bin
ln -sf ../../bin/et6000.driver /boot/home/config/add-ons/kernel/drivers/dev/graphics/et6000.driver
copyattr -d et6000.accelerant /boot/home/config/add-ons/accelerants
echo ...Installation is completed.
echo You need to reboot the computer for the installed driver to be loaded.
