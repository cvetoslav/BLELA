#include <Arduino.h>
#include <ArduinoBLE.h>
#include "BBTimer.hpp"


// BLE stuff
BLEService LAService("6a09fdbd-664d-4e00-8e92-d9e51c6eba93"); // create service

BLEUnsignedIntCharacteristic setupCharacteristic("5a61383f-e7e0-4e92-bc52-4ff803be0f1c", BLERead | BLEWrite);    // configuration characteristic
BLEUnsignedIntCharacteristic startCharacteristic("3642a423-afd4-4ab1-95b2-5f7bff961610", BLERead | BLEWrite);    // start of  characteristic
BLECharacteristic dataCharacteristic("ea91a314-ab37-4f3b-8a58-d3660b5ab81a", BLERead | BLENotify, 20);    // peripheral to clients data characteristic
BLEBoolCharacteristic readyCharacteristic("d80ca4bd-7c02-4728-bf8a-3e6c10c4b378", BLERead | BLENotify);


// HW Timer stuff
BBTimer timer3(BB_TIMER3);

uint32_t cnt = 0;
uint16_t buffer[0x10000];
uint32_t cnt_lim = 0x1000;


// callback functions
void timer_cb();
void ble_connected_cb(BLEDevice dev);
void ble_disconnected_cb(BLEDevice dev);
void ble_char_written_cb(BLEDevice dev, BLECharacteristic ch);


void setup() {
  // pin setup
	pinMode(LED_BUILTIN, OUTPUT);
  //nrf_gpio_port_dir_output_set(NRF_P1, uint32_t out_mask);
  for(int i=2; i <= 12; i++)
	  pinMode(i, INPUT);

  // initial timer setup
	timer3.setupTimer(100, timer_cb);
	//timer3.timerStart();

  //analogWrite(LED_BUILTIN, 100);

  // BLE setup
  if (!BLE.begin()) while (1);

  BLE.setLocalName("BLELA");
  BLE.setAdvertisedService(LAService);
  LAService.addCharacteristic(setupCharacteristic);
  LAService.addCharacteristic(startCharacteristic);
  LAService.addCharacteristic(dataCharacteristic);
  LAService.addCharacteristic(readyCharacteristic);
  BLE.addService(LAService);
  setupCharacteristic.writeValue(10000);
  startCharacteristic.writeValue(0);
  dataCharacteristic.writeValue(buffer, 20);
  readyCharacteristic.writeValue(true);
  setupCharacteristic.setEventHandler(BLEWritten, ble_char_written_cb);
  startCharacteristic.setEventHandler(BLEWritten, ble_char_written_cb);
  //readyCharacteristic.setEventHandler(BLEWritten, ble_char_written_cb);
  //dataCharacteristic.setEventHandler(BLEWritten, ble_char_written_cb);
  BLE.setEventHandler(BLEConnected, ble_connected_cb);
  BLE.setEventHandler(BLEDisconnected, ble_disconnected_cb);
  BLE.advertise();

  // serial setup
  Serial.begin(9600); while(!Serial);
  Serial.print("BLE address: ");
  Serial.println(BLE.address());
  Serial.println("Setup complete. Advertising...");
}

void loop() {
  BLE.poll();  // process BLE events

  if(cnt == cnt_lim)
  {
    Serial.println("Polling complete.");

    for(int i=0; i < cnt; i+=10)
    {
      dataCharacteristic.writeValue(buffer + i, 20);
      delay(20);
    }

    Serial.println("Data sent.");

    readyCharacteristic.writeValue(true);
    cnt = 0;
  }
  
  delay(10);
}


void timer_cb()
{
  uint32_t x = nrf_gpio_port_in_read(NRF_P1);
  uint32_t y = nrf_gpio_port_in_read(NRF_P0);
  buffer[cnt++] = (x & 0b1111111110001111) | ((y & (1 << 21)) >> 17) | ((y & (1 << 23)) >> 18) | ((y & (1 << 27)) >> 21);
  if(cnt == cnt_lim) timer3.timerStop();
}

void ble_connected_cb(BLEDevice dev)
{
  Serial.print(" -> ");
  Serial.print(dev.address());
  Serial.println(" connected.");
}

void ble_disconnected_cb(BLEDevice dev)
{
  Serial.print(" <- ");
  Serial.print(dev.address());
  Serial.println(" disconnected.");
}

void ble_char_written_cb(BLEDevice dev, BLECharacteristic ch)
{
  if(setupCharacteristic.written())
  {
    uint32_t f = setupCharacteristic.value();
    Serial.print("Setup freq: ");
    Serial.print(f);
    Serial.println(" Hz");

    timer3.setupTimer(1000000 / f, timer_cb);
  }
  else if(startCharacteristic.written())
  {
    uint32_t t = startCharacteristic.value();
    if (t > 0)
    {
      cnt_lim = t;
      readyCharacteristic.writeValue(false);
      timer3.timerStart();
    }
  }
}
