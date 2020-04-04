/* mbed USBHost Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "USBHostMouse.h"

#if USBHOST_MOUSE

USBHostMouse::USBHostMouse() {
    host = USBHost::getHostInst();
    init();
}

void USBHostMouse::init() {
    dev = NULL;
    int_in = NULL;
    onUpdate = NULL;
    onButtonUpdate = NULL;
    onXUpdate = NULL;
    onYUpdate = NULL;
    onZUpdate = NULL;
    report_id = 0;
    dev_connected = false;
    mouse_device_found = false;
    mouse_intf = -1;

    buttons = 0;
    x = 0;
    y = 0;
    z = 0;
}

bool USBHostMouse::connected() {
    return dev_connected;
}

bool USBHostMouse::connect()
{
     int len_listen;
     uint8_t rep_buf[] = 
     {
	0,
     };
   
    if (dev_connected) {
        return true;
    }

    for (uint8_t i = 0; i < MAX_DEVICE_CONNECTED; i++) {
        if ((dev = host->getDevice(i)) != NULL) {

            if(host->enumerate(dev, this))
                break;
            if (mouse_device_found) {
                {
                    /* As this is done in a specific thread
                     * this lock is taken to avoid to process the device
                     * disconnect in usb process during the device registering */
                    USBHost::Lock  Lock(host);
                    int_in = dev->getEndpoint(mouse_intf, INTERRUPT_ENDPOINT, IN);
                    if (!int_in)
                        break;

                    USB_INFO("New Mouse device: VID:%04x PID:%04x [dev: %p - intf: %d %d]", dev->getVid(), dev->getPid(), dev, mouse_intf,int_in);
                    dev->setName("Mouse", mouse_intf);
                    host->registerDriver(dev, mouse_intf, this, &USBHostMouse::init);

                    int_in->attach(this, &USBHostMouse::rxHandler);
                    len_listen = int_in->getSize();
                    if (len_listen > sizeof(report)) {
                        len_listen = sizeof(report);
                    }
                }
		 
	       /* Call set protocol to select boot protocol, otherwise the device may use report mode! */
	       printf("ret = %d\r\n",host->controlWrite(  dev, USB_HOST_TO_DEVICE | USB_REQUEST_TYPE_CLASS | USB_RECIPIENT_INTERFACE,
				     0x0b, 0x00, mouse_intf, rep_buf, 0));
		 
	       
                int ret=host->interruptRead(dev, int_in, report, len_listen, false);
                MBED_ASSERT((ret==USB_TYPE_OK) || (ret ==USB_TYPE_PROCESSING) || (ret == USB_TYPE_FREE));
                if ((ret==USB_TYPE_OK) || (ret ==USB_TYPE_PROCESSING))
                    dev_connected = true;
                if (ret == USB_TYPE_FREE)
                    dev_connected = false;
                return true;
            }
        }
    }
    init();
    return false;
}

void USBHostMouse::rxHandler() {
    int len_listen = int_in->getLengthTransferred();
    int buttons_t, x_t, y_t, z_t;
    {
        int index;
        for (index=0;index < len_listen; index++)
        {
            printf("%2x ",report[index]);
        }
        
        if (index>0) printf("  %x:%x \r\n",dev->getVid(), dev->getPid());
    }
    
    // We really should be parsing the pid report but this is a good workaround 
    // for my wireless mouse for now!
    // Or NOT! Turns out that you need to use Set Protocol to select boot protocol or
    // the device may use report mode instead.
    /*if (dev->getVid() == 0x62a &&  dev->getPid() == 0x4101)
    {
        buttons_t = report[1] & 0x07;
        x_t = (((int)report[3] & 0xf) << 8) + (report[2]);
        y_t = (report[3] >> 4) + ((int)report[4] << 4);
        
        if (y_t & 0x800) y_t |= 0xf000; // Fix 4 highest bits of negative nos.
        if (x_t & 0x800) x_t |= 0xf000;
        
        z_t = report[5];
    }
    else*/
    {
        buttons_t = report[0] & 0x07;
        x_t = (int8_t)report[1];
        y_t = (int8_t)report[2];
        z_t = (int8_t)report[3];
    }
    if (len_listen !=0) {

        if (onUpdate) {
            (*onUpdate)(buttons_t, x_t, y_t, z_t);
        }

        if (onButtonUpdate && (buttons != (buttons_t))) {
            (*onButtonUpdate)(buttons_t);
        }

        if (onXUpdate && (x != x_t)) {
            (*onXUpdate)(x_t);
        }

        if (onYUpdate && (y != y_t)) {
            (*onYUpdate)(y_t);
        }

        if (onZUpdate && (z != z_t)) {
            (*onZUpdate)(z_t);
        }

        // update mouse state
        buttons = buttons_t;
        x = x_t;
        y = y_t;
        z = z_t;
    }
    /*  set again the maximum value */
    len_listen = int_in->getSize();

    if (len_listen > sizeof(report)) {
        len_listen = sizeof(report);
    }

    if (dev)
        host->interruptRead(dev, int_in, report, len_listen, false);
}

/*virtual*/ void USBHostMouse::setVidPid(uint16_t vid, uint16_t pid)
{
    // we don't check VID/PID for mouse driver
}

/*virtual*/ bool USBHostMouse::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
    if ((mouse_intf == -1) &&
        (intf_class == HID_CLASS) &&
        (intf_subclass == 0x01) &&
        (intf_protocol == 0x02)) {
        mouse_intf = intf_nb;
        return true;
    }
    return false;
}

/*virtual*/ bool USBHostMouse::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{
    if (intf_nb == mouse_intf) {
        if (type == INTERRUPT_ENDPOINT && dir == IN) {
            mouse_device_found = true;
            return true;
        }
    }
    return false;
}

#endif
