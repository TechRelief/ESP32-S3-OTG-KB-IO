/*
This code shows how the ESP32-S3-USB-OTG board can input keystrokes from a USB keyboard using it's USB Host female port 
and output the keystrokes via bluetooth.  In otherwords we have created a keyboard dongle.
This code can be the starting point for a number of projects such as a wireless keyboard controller or a
keyboard dongle that add functionality to a keyboard such as macros or recording keystrokes etc.
On the EPS32-S3-USB_OTG board Use the USB Micro port for uploading the code or for the serial monitor.
If using a different ESP32-S3 based board you will have to determine how to manage the USB port(s).
Note: This code by default expects a battery connected to the BAT pads, look for the comment below on how to change
this to use the USB_DEV port for power.
*/
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "EspUsbHostEx.h"
#include <BLEDevice.h>
#include <BLEAdvertising.h>

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library for TFT Display
BLEMultiAdvertising advert(1); // Max number of advertisement data 

#define TFT_GREY 0x5AEB // New colour
#define BTN_OK_PIN 0 //OK button pin
#define BTN_DW_PIN 11 //Down button pin
#define BTN_UP_PIN 10 //Up button pin
#define BTN_MENU_PIN 14 //Menu button pin
#define LED_YELLOW_PIN 16 //Yellow LED pin
#define LED_GREEN_PIN 15 //Green LED pin
#define FONT_NO 4 //Default font number
#define TEXT_SIZE 1 //Default font size
#define TEXT_WIDTH 240
#define TEXT_HEIGHT 240

// Temporary buffers for display text handling
char ts1[80];
char ts2[80];
char ts3[80];
char ts4[80];
char ts5[80];

/// @brief Clear the TFT display
void cls()
{
  tft.fillRect(0, 0, TEXT_WIDTH, TEXT_HEIGHT, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(1,1);
}

// void displayAt(String text, int32_t x, int32_t y)
// {
//   // tft.fillRect(0, 0, TEXT_WIDTH, TEXT_HEIGHT, TFT_BLACK);
//   tft.setTextColor(TFT_YELLOW, TFT_BLACK);
//   //tft.setTextSize(TEXT_SIZE);
//   //tft.drawCentreString(text, TEXT_WIDTH/2, TEXT_HEIGHT/2, FONT_NO);
//   tft.setCursor(x, y);
//   tft.print(text);
//   //tft.drawString(text, x, y);
// }


/// @brief The USB Host class used to administer and controll the USB Host port.
///        It includes several "call-back" methods such as onKeyboard(...) and onKeyboardKey(...) to return keycode information when a key is pressed.
///        Enable or Disable the RAW_REPORT and/or the KEY_REPORT defines in EspUsbHostEx.h depending on which versions you need for porcessing.
class UsbHost : public EspUsbHost
{
  #ifdef RAW_REPORT
  void onKeyboard(hid_keyboard_report_t report, hid_keyboard_report_t prevReport, hid_key_modifier mod)
  {
    sprintf(ts1, " %02X %02X %02X %02X %02X %02X", report.keycode[0], report.keycode[1], report.keycode[2], report.keycode[3], report.keycode[4], report.keycode[5]);
    sprintf(ts2, " modi:  %02X", mod.modbyte);
    sprintf(ts3, " Shift: %s\n Alt: %s\n Ctrl: %s\n Win: %s", 
            mod.shift ? "Yes" : "No", 
            mod.alt ? "Yes" : "No", 
            mod.ctrl ? "Yes" : "No", 
            mod.win ? "Yes" : "No");
    String keyStr = XlatHidCode(report.keycode[0]);
    sprintf(ts4, " Name: %s\n", keyStr);
    cls();
    //tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.println("Keys:");
    tft.println(ts1);
    tft.println(ts2);
    tft.println(ts3);
    tft.println(ts4);
    sendInputReport(report);  //Send it out via BLE
  }
  #endif

  #ifdef KEY_REPORT
  void onKeyboardKey(uint8_t ascii, uint8_t keycode, hid_key_modifier mod)
  {
    sprintf(ts1, " Key:  %i - %02XH", keycode, keycode);
    sprintf(ts2, " modi:  %i = %02XH", mod.keyModifier, mod.keyModifier);
    sprintf(ts3, " Char: %c = %02XH", ascii, ascii);
    sprintf(ts4, " Shift: %s\n Alt: %s\n Ctrl: %s\n Win: %s", 
            mod.shift ? "Yes" : "No", 
            mod.alt ? "Yes" : "No", 
            mod.ctrl ? "Yes" : "No", 
            mod.win ? "Yes" : "No");
    String keyStr = XlatHidCode(keycode);
    sprintf(ts5, " Name: %s\n", keyStr);
    cls();
    //tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.println();
    tft.println(ts1);
    tft.println(ts2);
    tft.println(ts3);
    tft.println(ts4);
    tft.println(ts5);
  };
  #endif

};

UsbHost usbHost;

void setup()
{
  pinMode(BTN_DW_PIN, INPUT_PULLUP);
  pinMode(BTN_MENU_PIN, INPUT_PULLUP);
  pinMode(BTN_OK_PIN, INPUT_PULLUP);
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);

  //Esp-s1-usb-otg specific board setup:
  /*Calls into C:\Users\{your_user_id}\.platformio\packages\framework-arduinoespressif32\variants\esp32s3usbotg\variant.cpp
               C:\Users\{your_user_id}\.platformio\packages\framework-arduinoespressif32\variants\esp32s3usbotg\pins_arduino.h
   The initVariant() call initializes pinModes and output levels for:
     BOOST_EN (GPIO13) to LOW - Power is not boosted, i.e. battery is not used by default.
     LIMIT_EN (GPIO17) to LOW - Host port output power is NOT limited to 500mA by default.
     DEV_VBUS_EN (GPIO12) to LOW - Defaults to No Host Power Output
     USB_HOST_EN (GPIO18) to LOW - Defaults to the USB_DEV port
     Then it disables the TFT Display by turning it off by default by setting:
     LCD_RST (GPIO8) to LOW - Disable TFT Display.
     LCD_BL (GPIO9) to LOW - Turns Backlight off.
  */
  initVariant();
  usbHostPower(USB_HOST_POWER_BAT);  //Use battery power, if you want to use the USB-DEV port for power change this to: usbHostPower(USB_HOST_POWER_VBUS);
  usbHostEnable(true); // Enable the USB HOST Port
  updateLeds();  //Initialize the green and yellow LEDs.
  // start the Bluetooth task
  xTaskCreate(bluetoothTask, "bluetooth", 20000, NULL, 5, NULL);  //Second to last parameter is called uxPriority and was set to 5 on some boards this may need to be tweeked.

  // Setup the TFT display
  tft.init();
  tft.setRotation(0); // 0 = Vertical (flipped); 1 = Horizontal; 2 = Vertical, 3 = horizontal (flipped)
  cls();
  tft.setTextFont(FONT_NO);
  tft.setTextSize(TEXT_SIZE);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(1, 1);
  tft.println("");
  tft.println("USB Host Started...");
  tft.println("Make sure to connect");
  tft.println("a Battery and switch");
  tft.println("it on to power the");
  tft.println("keyboard.");
  Serial.begin(115200);
  usbHost.begin();
  usbHost.setHIDLocal(HID_LOCAL_International);
}

int8_t buttonPressed()
{
  if (digitalRead(BTN_MENU_PIN) == 0)
    return BTN_MENU_PIN;
  else if (digitalRead(BTN_OK_PIN) == 0)
    return BTN_OK_PIN;
  else if (digitalRead(BTN_DW_PIN) == 0)
    return BTN_DW_PIN;
  else if (digitalRead(BTN_UP_PIN) == 0)
    return BTN_UP_PIN;
  return -1;
}

void loop(void)
{
  usbHost.task();
}
