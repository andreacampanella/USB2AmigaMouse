# USB2AmigaMouse

This project it's been forked from Louise Newberry's project ( https://os.mbed.com/users/geekylou/code/Nucleo_usbhost_quadrature_encoder/ )

The aim it's to give it more visibility and a good cleanup and documentation. 

>Use

Program a build base on this repository to a STM32 nucleo board (I tested this on a NUCLEO STM32F411 board). You will then need connect the Nucleo board to your computers joystick and mouse ports as follows (if you only use a mouse you won't need a joystick lead :) ). If you use a steam controller the thumb stick and left pad are mapped to the Amiga's joystick port and the right pad and front trigger buttons are mapped to the mouse.
'''
mouse_port_forward(D10)     - Amiga Joystick pin 1
mouse_port_back(D11)          - Amiga Joystick pin 2
mouse_port_left(D12)          - Amiga Joystick pin 3
mouse_port_right(D13)         - Amiga Joystick pin 4
mouse_port_left_button(D9)    - Amiga Joystick pin 6
GND                           - Amiga joystick pin 8
mouse_port_right_button(D8)   - Amiga Joystick pin 9
'''

game_port_forward(PC_8)       - Amiga Joystick pin 1
game_port_back(PC_12)         - Amiga Joystick pin 2
game_port_left(PC_10)         - Amiga Joystick pin 3
game_port_right(PC_11)        - Amiga Joystick pin 4
game_port_left_button(PC_6)   - Amiga Joystick pin 6
GND                           - Amiga joystick pin 8
game_port_right_button(PC_5)  - Amiga Joystick pin 9

You will also need to attach a USB socket to the nuclea board so you can use the Nucleo's usb OTG support.

(1) 5V     - 5v
(2) PA11 - Data-
(3) PA12 - Data+
(4) GND - GND
According to the usb spec it is required to attach a 15k ohm resistor between D-/+ and ground but it appears the USB host works fine without this.
