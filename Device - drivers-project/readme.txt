Readme:

•Build Firmware:
Firmware files are uploaded in the respective directory on github. 
In order convert LPC to mass storage, Keil uVision software is required to build the files along with the library present in it.
Flash the generated hex file using flash magic onto the LPC device. 
Disconnect the device and reconnect after 10 seconds.
Go to Device Manager and verify that the device is recognized as a mass storage device.
In Linux, the logs can also be used to verify the same. The VID and PID number of device can be extracted from the logs, generated using $ dmesg.


•In order to build the driver, use the makefile uploaded on the github.
$ make all
$ sudo insmod kernel_driver.ko
 Before inserting the above module remove the inbuilt drivers for USB devices:
	$ sudo rmmod uas
	$ sudo rmmod usb_storage

Command to read a specific sector:
	$ dd if=/dev/sdc bs=512 skip=5 count=1 | hexdump –R
Command to write to specific sector:
	$ dd if=/sample.txt of=/dev/sdc skip=5 count=1

•Status of the project: 
  The Write operation status
  - CBW OUT packet(URB) is sent and getting acknowledged by the firware
  - Write OUT packet(URB) is sent and getting acknowledged by the firmware
  - CSW IN packet(URB) is being successfully Submitted to the USB core but NO acknowledgment is being recieved.
  The Read operation status
  - CBW OUT packet(URB) is sent and getting acknowledged by the firware
  - Read IN packet(URB) is sent and gets stuck in a infinite loop and system crashes.
  
 
