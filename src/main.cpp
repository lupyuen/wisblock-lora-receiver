//  Based on https://github.com/RAKWireless/WisBlock/blob/master/examples/communications/LoRa/LoRaP2P/LoRaP2P_RX/LoRaP2P_RX.ino
/**
 * @file LoRaP2P_RX.ino
 * @author rakwireless.com
 * @brief Receiver node for LoRa point to point communication
 * @version 0.1
 * @date 2020-08-21
 * 
 * @copyright Copyright (c) 2020
 * 
 * @note RAK5005-O GPIO mapping to RAK4631 GPIO ports
   RAK5005-O <->  nRF52840
   IO1       <->  P0.17 (Arduino GPIO number 17)
   IO2       <->  P1.02 (Arduino GPIO number 34)
   IO3       <->  P0.21 (Arduino GPIO number 21)
   IO4       <->  P0.04 (Arduino GPIO number 4)
   IO5       <->  P0.09 (Arduino GPIO number 9)
   IO6       <->  P0.10 (Arduino GPIO number 10)
   SW1       <->  P0.01 (Arduino GPIO number 1)
   A0        <->  P0.04/AIN2 (Arduino Analog A2
   A1        <->  P0.31/AIN7 (Arduino Analog A7
   SPI_CS    <->  P0.26 (Arduino GPIO number 26) 
 */
#include <Arduino.h>
#include <SX126x-RAK4630.h> //http://librarymanager/All#SX126x
#include <SPI.h>

// Function declarations
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnRxTimeout(void);
void OnRxError(void);

#ifdef NRF52_SERIES
#define LED_BUILTIN 35
#endif

// Define LoRa parameters. To receive LoRa packets from BL602, sync the parameters with
// https://github.com/lupyuen/bl_iot_sdk/blob/lora/customer_app/sdk_app_lora/sdk_app_lora/demo.c#L41-L77
// TODO: Change RF_FREQUENCY for your region
#define RF_FREQUENCY          923000000	// Hz
#define TX_OUTPUT_POWER       22		// dBm
#define LORA_BANDWIDTH        0		    // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 7         // [SF7..SF12]
#define LORA_CODINGRATE       1		    // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH  8	        // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT   0	        // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false
#define RX_TIMEOUT_VALUE      3000
#define TX_TIMEOUT_VALUE      3000

//  Callback Functions for LoRa Events
static RadioEvents_t RadioEvents;

//  Buffer for received LoRa Packet
static uint8_t RcvBuffer[64];

//  Setup Function is called upon startup
void setup()
{

    //  Initialize the LoRa Module
    lora_rak4630_init();

    //  Initialize the Serial Port for debug output
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println("=====================================");
    Serial.println("LoRaP2P Rx Test");
    Serial.println("=====================================");

    //  Set the LoRa Callback Functions
    RadioEvents.TxDone    = NULL;
    RadioEvents.RxDone    = OnRxDone;
    RadioEvents.TxTimeout = NULL;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError   = OnRxError;
    RadioEvents.CadDone   = NULL;

    //  Initialize the LoRa Transceiver
    Radio.Init(&RadioEvents);

    //  Set the LoRa Frequency
    Radio.SetChannel(RF_FREQUENCY);

    //  Configure the LoRa Transceiver for receiving messages
    Radio.SetRxConfig(
        MODEM_LORA, 
        LORA_BANDWIDTH, 
        LORA_SPREADING_FACTOR,
        LORA_CODINGRATE, 
        0,        //  AFC bandwidth: Unused with LoRa
        LORA_PREAMBLE_LENGTH,
        LORA_SYMBOL_TIMEOUT, 
        LORA_FIX_LENGTH_PAYLOAD_ON,
        0,        //  Fixed payload length: N/A
        true,     //  CRC enabled
        0,        //  Frequency hopping disabled
        0,        //  Hop period: N/A
        LORA_IQ_INVERSION_ON, 
        true      //  Continuous receive mode
    );

    //  Start receiving LoRa packets
    Serial.println("Starting Radio.Rx");
    Radio.Rx(RX_TIMEOUT_VALUE);
}

//  Loop Function is called repeatedly to handle events
void loop()
{
    //  Handle Radio events
    Radio.IrqProcess();

    //  We are on FreeRTOS, give other tasks a chance to run
    delay(100);
    yield();
}

/**@brief Function to be executed on Radio Rx Done event
 */
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
    //  We have received a valid packet. Show the timestamp in seconds.
    Serial.printf("OnRxDone: Timestamp=%d, ", millis() / 1000);
    delay(10);
    memcpy(RcvBuffer, payload, size);

    //  Show the signal strength, signal to noise ratio
    Serial.printf("RssiValue=%d dBm, SnrValue=%d, Data=", rssi, snr);

    //  Show the packet received
    for (int idx = 0; idx < size; idx++)
    {
        Serial.printf("%02X ", RcvBuffer[idx]);
    }
    Serial.println("");

    //  Receive the next packet
    Radio.Rx(RX_TIMEOUT_VALUE);
}

/**@brief Function to be executed on Radio Rx Timeout event
 */
void OnRxTimeout(void)
{
    //  We haven't received a packet during the timeout period.
    //  We disable the timeout message because it makes the log much longer.
    //  Serial.println("OnRxTimeout");

    //  Receive the next packet
    Radio.Rx(RX_TIMEOUT_VALUE);
}

/**@brief Function to be executed on Radio Rx Error event
 */
void OnRxError(void)
{
    //  We have received a corrupted packet, probably due to weak signal.
    //  Show the timestamp in seconds.
    Serial.printf("OnRxError: Timestamp=%d\n", millis() / 1000);

    //  Receive the next packet
    Radio.Rx(RX_TIMEOUT_VALUE);
}