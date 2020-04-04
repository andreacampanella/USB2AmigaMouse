#include "mbed.h"
#include "USBHostMSD.h"
#include "USBHostMouse.h"
#include "USBHostKeyboard.h"
#include "USBHostSteamController.h"
#include "FATFileSystem.h"
#include <stdlib.h>

inline int max ( int a, int b ) { return a > b ? a : b; }

#define MAX_QUADRATURE_FREQ 3500

volatile int x_cur = 0;   
volatile int y_cur = 0;
volatile int x_pos = 0;
volatile int y_pos = 0;

Ticker ticker;

DigitalOut mouse_port_right_button(D8);
DigitalOut mouse_port_left_button(D9);
DigitalOut mouse_port_forward(D10);
DigitalOut mouse_port_left(D12);

DigitalOut mouse_port_back(D11);
DigitalOut mouse_port_right(D13);



DigitalOut game_port_right_button(PC_5);
DigitalOut game_port_left_button(PC_6);
DigitalOut game_port_forward(PC_8);
DigitalOut game_port_left(PC_10);

DigitalOut game_port_back(PC_12);
DigitalOut game_port_right(PC_11);



//DigitalOut led_blue(LED3);

void update_quadrature() 
{
    //led_blue = !led_blue;
    
    if (x_pos != 0 || y_pos != 0)
    {
        if (x_pos >0)
        { 
            x_pos--;
            x_cur = (x_cur+1) & 3;
        }
        else if (x_pos < 0)
        {
            x_pos++;
            x_cur = (x_cur-1) & 3;
        }

        if (y_pos >0)
        { 
            y_pos--;
            y_cur = (y_cur+1) & 3;
        }
        else if (y_pos < 0)
        {
            y_pos++;
            y_cur = (y_cur-1) & 3;
        }      
        
        mouse_port_back = ((x_cur + 1)/2) & 1;
        mouse_port_right = (x_cur/2) & 0x1;
        
        mouse_port_forward = ((y_cur + 1)/2) & 1;
        mouse_port_left = (y_cur/2) & 0x1;
    }
}

void onMouseEvent(uint8_t buttons, int16_t x, int16_t y, int8_t z) {
    //printf("ME: buttons: %d, x: %d, y: %d, z: %d\r\n", buttons, x, y, z);
    
    // Disabling ticker as a quick method of disabling the ticker intterrupt.
    // This is to stop the IRQ being called when we have updated x_pos but not
    // y_pos.
    ticker.detach();
    
    x_pos += x;
    y_pos += y;
    
    // This should be enough to calculate the pulse width.  As we just want to
    // calculate the speed we need to pulse to not lag.
    
    int dist = max(abs(x_pos),abs(y_pos)) * 30;
    
    dist = dist > MAX_QUADRATURE_FREQ ? MAX_QUADRATURE_FREQ : dist; /* Cap frequency to 3khz. */  
    
    mouse_port_left_button = ~buttons & 1;
    mouse_port_right_button = ~buttons & 0x2;
    
    if (x_pos != 0 || y_pos != 0)
    {
        ticker.attach(update_quadrature, 1.0 / dist);
        
        //ticker.attach_us(update_quadrature, 1000000 / dist);
    }
    Thread::yield();
}

uint32_t prev_buttons = 0;
int prev_x, prev_y;

void onJSEvent(uint32_t buttons, int8_t x, int8_t y, int16_t x_b, int16_t y_b) {
    //printf("buttons: %x, x: %d, y: %d, x_b: %d y_b:%d  :", buttons, x, y, x_b, y_b);


    game_port_left_button = ~buttons & 0x40;
    game_port_right_button = ~buttons & 0x10;
    
    if (buttons & 0x100000)
    {
        if (prev_buttons & 0x100000)
        {
            //printf("x:%d y:%d", prev_x - x_b, prev_y - y_b);
            
            ticker.detach();
            
            x_pos += -((prev_x - x_b)/128);
            y_pos += (prev_y - y_b)/128;
            
            int dist = max(abs(x_pos),abs(y_pos)) * 30;
    
            dist = dist > MAX_QUADRATURE_FREQ ? MAX_QUADRATURE_FREQ : dist; /* Cap frequency to 3khz. */  
            
            if (x_pos != 0 || y_pos != 0)
            {
                ticker.attach(update_quadrature, 1.0 / dist);
                
                //ticker.attach_us(update_quadrature, 1000000 / dist);
            }
        }
        prev_x = x_b; prev_y = y_b;
    }
    //printf("\r\n");
    
    prev_buttons = buttons;
    
    mouse_port_left_button = ~buttons & 2;
    mouse_port_right_button = ~buttons & 0x1;
    
    game_port_forward = !( y > 40);
    game_port_back = !( y < -40 );
    game_port_left = !( x < -40 );
    game_port_right = !( x > 40 );     
    //Thread::wait(1);
    Thread::yield();
}
 
void mouse_task(void const *) {

    USBHostMouse mouse;
    printf("mouse started\n");
    while(1) {
       
        // try to connect a USB mouse
        while(!mouse.connect())
            Thread::wait(500);

        // when connected, attach handler called on mouse event
        mouse.attachEvent(onMouseEvent);

        // wait until the mouse is disconnected
        while(mouse.connected())
            Thread::wait(500);
        printf("mouse seen disconnected\n");
    }
}
void onKey(uint8_t key) {
    printf("Key: %c\r\n", key);
}
void keyboard_task(void const *) {
    
    USBHostKeyboard keyboard;
    
    while(1) {
        // try to connect a USB keyboard
        while(!keyboard.connect())
            Thread::wait(500);
    
        // when connected, attach handler called on keyboard event
        keyboard.attach(onKey);
        
        // wait until the keyboard is disconnected
        while(keyboard.connected())
            Thread::wait(500);
    }
}

void steamctrl_task(void const *) {
    
    USBHostSteamController steam_controller;
    
    while(1) {
        // try to connect a USB keyboard
        while(!steam_controller.connect())
            Thread::wait(500);
    
        // when connected, attach handler called on keyboard event
        steam_controller.attachEvent(onJSEvent);
        
        // wait until the keyboard is disconnected
        while(steam_controller.connected())
            Thread::wait(500);
    }
}



void msd_task(void const *) {

    USBHostMSD msd;
    int i = 0;
    FATFileSystem fs("usb");
    int err;
    printf("wait for usb memory stick insertion\n");
    while(1) {

        // try to connect a MSD device
        while(!msd.connect()) {
            Thread::wait(500);
        }
        if (fs.mount(&msd) != 0) continue;
        else  
            printf("file system mounted\n");

        if  (!msd.connect()) {
            continue;
        }

        // in a loop, append a file
        // if the device is disconnected, we try to connect it again

        // append a file
        File file;
        err = file.open(&fs, "test1.txt", O_WRONLY | O_CREAT |O_APPEND);

        if (err == 0) {
            char tmp[100];
            sprintf(tmp,"Hello fun USB stick  World: %d!\r\n", i++);
            file.write(tmp,strlen(tmp));
            sprintf(tmp,"Goodbye World!\r\n");
            file.write(tmp,strlen(tmp));
            file.close();
        } else {
            printf("FILE == NULL\r\n");
        }
        Thread::wait(500);
        printf("again\n");
        // if device disconnected, try to connect again
        while (msd.connected()) {
            Thread::wait(500);
        }
        while (fs.unmount() < 0) {
            Thread::wait(500);
            printf("unmount\n"); 
        }
    }
}

int main() 
{   
    mouse_port_left_button = 1;
    mouse_port_right_button = 1;
    
    mouse_port_back = 1;
    mouse_port_right = 1;
    mouse_port_forward = 1;
    mouse_port_left = 1;
        
    game_port_left_button = 1;
    game_port_right_button = 1;
        
    game_port_forward = 1;
    game_port_back = 1;
    game_port_left = 1;
    game_port_right = 1;     

    //Thread msdTask(msd_task, NULL, osPriorityNormal, 1024 * 4);
    Thread mouseTask(mouse_task, NULL, osPriorityNormal, 2048* 4);
    Thread steamctrlTask(steamctrl_task, NULL, osPriorityNormal, 2048* 4);
    //Thread keyboardTask(keyboard_task, NULL, osPriorityNormal, 1024 * 4);
    
    int x,y = 0;
    for(y=-10; y<10; y++)
    {
        x = y & 3;
        printf("phase %d[%d] %d\r\n",((x + 1)/2) & 1, x, (x/2) & 0x1 );
    }
    while(1) {
        //led=!led;
        Thread::wait(500);
    }
}