# introduction
this program is used to drive a DSP like [ADAU1452](http://www.analog.com/en/products/audio-video/audio-signal-processors/sigmadsp-audio-processors/adau1452.html) from [Analog Device Sigmastudio](http://www.analog.com/en/design-center/processors-and-dsp/evaluation-and-development-software/ss_sigst_02.html) GUI; that's useful for rapid development.

the program is supposed to run on a target board with an I2C bus connected to the DSP board

in our use case, we have tested on a Cubieboard Allwinner A10 SOC running kernel 4.11.xx with buildroot OS

this src is loosely based on sample code from:
  https://wiki.analog.com/resources/tools-software/linux-software/sigmatcp

i've used __Sigma Studio 3.14.1 Build 1675__ on windows 7 toward an ADAU1452 board

# video
you can have a look at my setup and running test here:
 https://drive.google.com/open?id=0B9QRW6Oy0Gmea2dDNmpIRjZyOVk
 
# usage

it's pretty simple.

* clone the repo: __git clone https://github.com/aventuri/sigma_tcp.git__
* make for cross compiling the source in a single bin **sigma_tcp**
  * eventually adjust the __BASEDIR__ var in Makefile to match your local environment
* copy the binary in your SBC root fs
* execute as __./sigma_tcp i2c /dev/i2c-1 0x3b__, some verbose print tells you about IP and port
* get to Sigma Studio, cfg the prj to lookup for IP connection (with param from above)
* open connection (the tool will spit out some messages)
* link compile download youre DSP design into the DSP (lot's of prints on tool..)

example of this invocation:

 # ./sigma_tcp i2c /dev/i2c-1 0x3b
 Utility for driving ADAU1452 over IP/I2C using SigmaStudio
 License GPL v3 or later
 Andrea Venturi <be17068@iperbole.bo.it>
 
 current development at https://github.com/aventuri/sigma_tcp
 inspired by https://wiki.analog.com/resources/tools-software/linux-software/sigmatcp
 
 Using i2c backend
 i2c: Initalized for device /dev/i2c-1-3b
 Waiting for connections on port 8086...
 IP addresses:
 eth0: 192.168.0.103

# test i2c connection
how to connect the board to the Arm SBC is out of scope, here, but you can check it works with a simple test:
 # i2cdetect  -y 1
      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
 00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
 10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
 20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
 30: -- -- -- -- -- -- -- -- -- -- -- 3b -- -- -- -- 
 40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
 50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
 60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
 70: -- -- -- -- -- -- -- --   

usually the DSP has i2c address __0x3b__
# devel board
i've been using a pretty cheap board from aliexpress:
  https://www.aliexpress.com/item/ADAU1452-DSP-development-board-learning-board/32814063707.html

 
  
