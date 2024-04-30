#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <driver/adc.h>
#include "esp32-hal-cpu.h"


//Temp measurement variables
float Temp; //Temperature
float Vout; //Voltage after adc
float SenVal; // Analog Sensor value
float Beta = 0.9;//RC factor
float FilteredVal;//voltage after digital filter


bool deviceConnected = false;

void getTemp();//adc & temp function declaration
void BLETransfer(int16_t);


#define enviornmentService BLEUUID((uint16_t)0x181A)//serviceUUID

 BLECharacteristic temperatureCharacteristic(BLEUUID((uint16_t)0x2A6E),//characteristic UUID
 BLECharacteristic::PROPERTY_READ |BLECharacteristic::PROPERTY_NOTIFY);

class MyServerCallbacks: public BLEServerCallbacks 
{
    void onConnect(BLEServer* pServer) 
    {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) 
    {
      deviceConnected = false;
    }
};

void setup() 
{ 
  setCpuFrequencyMhz(80); //Set CPU clock to 80MHz
  adc1_config_width(ADC_WIDTH_BIT_12);//12 bit resolution
  adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);
  
  // Create the BLE Device
  BLEDevice::init("MyESP32");
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_N14);//set tx power for BLE

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pEnviornment = pServer->createService(enviornmentService);

  // Create a BLE Characteristic
  pEnviornment->addCharacteristic(&temperatureCharacteristic);

  // Create a BLE Descriptor
  temperatureCharacteristic.addDescriptor(new BLE2902());
  BLEDescriptor TemperatureDescriptor(BLEUUID((uint16_t)0x2901));
  TemperatureDescriptor.setValue("Thermocouple Temperature °C");
  temperatureCharacteristic.addDescriptor(&TemperatureDescriptor);

  pServer->getAdvertising()->addServiceUUID(enviornmentService);

  pEnviornment->start(); // Start the service

  pServer->getAdvertising()->start();// Start advertising  
}

void loop() 
{ 
  if (deviceConnected)//if there is a client connected/paired
  {
    getTemp();//call adc/temp conversion function
    char txtString[10]; //buffer used to store temp value as a string
    sprintf(txtString, "%2.3f", Temp);//convert the temp float to a string
    temperatureCharacteristic.setValue(txtString);//set the characteristic to the temp string
    temperatureCharacteristic.notify();//send value
  }
   delay(1000);//send value 1/s
}

void getTemp()
{
  SenVal = adc1_get_raw(ADC1_CHANNEL_5); //Analog value from AD8495 output pin
  Vout = (SenVal *3.434)/ 4095; //Conversion analog to digital for the temperature read voltage 
  FilteredVal = FilteredVal - (Beta*(FilteredVal-Vout));//digital filter
  Temp = (184.61*FilteredVal)-229.939;//calibrated conversion of digital voltage level to temperature 
}
