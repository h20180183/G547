INSTRUCTIONS:

KERNEL_VERSION USED 4.15.0.46

-- Create a text file that you wish to link to the device and copy paste the path in FILE_PATH macro defined int the begining
   of the block_driver.c(main code)
-- "sudo fdisk -l /dev/dof" - Disk info and partition info.

Commands to write to device:

-- Switch to superuser mode (sudo su) and then type following command
   echo "put the string that you wish to write to file" > /dev/dof

NOTE: Before writing to the device, please close the linked text file.(Donot keep ot open while writing as it wouldn't update)    
   
Commands to use Diskdump utility:

---- Switch to superuser mode (sudo su) and then type following command
     dd of=/dev/dof if=from_file.txt
   (This command copies text from from_file.txt to the text file linked to your device)  
   
