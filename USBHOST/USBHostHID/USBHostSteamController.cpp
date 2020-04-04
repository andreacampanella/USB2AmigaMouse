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

#include "USBHostSteamController.h"

#if USBHOST_STEAM

USBHostSteamController::USBHostSteamController() {
    host = USBHost::getHostInst();
    init();
}

void USBHostSteamController::init() {
    dev = NULL;
    int_in = NULL;
    onUpdate = NULL;
    onButtonUpdate = NULL;
    onXUpdate = NULL;
    onYUpdate = NULL;
    onZUpdate = NULL;
    report_id = 0;
    dev_connected = false;
    steam_device_found = false;
    steam_intf = -1;

    buttons = 0;
    x = 0;
    y = 0;
    z = 0;
    
    right_pad_x = 0;
    right_pad_y = 0;
}

bool USBHostSteamController::connected() {
    return dev_connected;
}

bool USBHostSteamController::connect()
{
    int len_listen;

    if (dev_connected) {
        return true;
    }

    for (uint8_t i = 0; i < MAX_DEVICE_CONNECTED; i++) {
        if ((dev = host->getDevice(i)) != NULL) {

            if(host->enumerate(dev, this))
                break;
            if (steam_device_found) {
                {
                    printf("found!");
                    /* As this is done in a specific thread
                     * this lock is taken to avoid to process the device
                     * disconnect in usb process during the device registering */
                    USBHost::Lock  Lock(host);
                    int_in = dev->getEndpoint(steam_intf, INTERRUPT_ENDPOINT, IN);
                    if (!int_in)
                        break;
                    USB_INFO("New SteamController device: VID:%04x PID:%04x [dev: %p - intf: %d]", dev->getVid(), dev->getPid(), dev, steam_intf);
                    dev->setName("Steam", steam_intf);
                    host->registerDriver(dev, steam_intf, this, &USBHostSteamController::init);

                    int_in->attach(this, &USBHostSteamController::rxHandler);
                    len_listen = int_in->getSize();
                    if (len_listen > sizeof(report)) {
                        len_listen = sizeof(report);
                    }
                }
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

void USBHostSteamController::rxHandler() {
    int len_listen = int_in->getLengthTransferred();
    int buttons_t, x_t, y_t, z_t;
    /*{
        int index;
        for (index=0;index < len_listen; index++)
        {
            printf("%2x ",report[index]);
        }
        
        if (index>0) printf("  %x:%x %d\r\n",dev->getVid(), dev->getPid(),len_listen);
    }*/
    

    if ((len_listen !=0) && (report[2] == 1))
    {          
        buttons_t = report[8] + (report[9] << 8) + (report[10] << 16); // << 4;
        x_t = report[17]; // report[16] lsb
        y_t = report[19]; // report[18] lsb
        
        z_t = 0; //report[4];
    
        right_pad_x = (report[21] << 8) + report[20];
        right_pad_y = (report[23] << 8) + report[22];

        if (onUpdate) {
            (*onUpdate)(buttons_t, x_t, y_t, right_pad_x, right_pad_y);
        }

/*        if (onButtonUpdate && (buttons != (report[0]))) {
            (*onButtonUpdate)(report[0]);
        }

        if (onXUpdate && (x != report[1])) {
            (*onXUpdate)(x_t);
        }

        if (onYUpdate && (y != report[2])) {
            (*onYUpdate)(y_t);
        }

        if (onZUpdate && (z != report[3])) {
            (*onZUpdate)(z_t);
        }*/

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

/*virtual*/ void USBHostSteamController::setVidPid(uint16_t vid, uint16_t pid)
{
    // we don't check VID/PID for mouse driver
}

/*virtual*/ bool USBHostSteamController::parseInterface(uint8_t intf_nb, uint8_t intf_class, uint8_t intf_subclass, uint8_t intf_protocol) //Must return true if the interface should be parsed
{
    //printf("intf_nb %d (%d) intf_class %x intf_subclass %x intf_protocol %x\r\n", intf_nb, steam_intf, intf_class,intf_subclass, intf_protocol);
    
    if ((steam_intf == -1) &&
        (intf_class == HID_CLASS) &&
        (intf_subclass == 0x00) &&
        (intf_protocol == 0x00)) {
        steam_intf = intf_nb;
        
        printf("true!");
        return true;
    }
    return false;
}

/*virtual*/ bool USBHostSteamController::useEndpoint(uint8_t intf_nb, ENDPOINT_TYPE type, ENDPOINT_DIRECTION dir) //Must return true if the endpoint will be used
{
    if (intf_nb == steam_intf) {
        if (type == INTERRUPT_ENDPOINT && dir == IN) {
            steam_device_found = true;
            return true;
        }
    }
    return false;
}

#endif
