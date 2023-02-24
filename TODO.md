# TODO

- [X] Build and run an image on an [Heltec WiFi LoRa 32 V2] board
- [X] Implement out-of-tree samples
  - [X] [send](samples/drivers/lora/send/)
  - [X] [receive](samples/drivers/lora/receive/)
- [X] Upstream [overlay](https://github.com/zephyrproject-rtos/zephyr/pull/51404)
- [X] Connect an [ST7735R] SPI TFT Display

## SPI DEVICES

The [ST7735R] TFT controller is connected to the LoRa PINs.

| [ST7735R] | [Heltec WiFi LoRa 32 V2] |
| --------- | ------------------------ |
| GND       | GND                      |
| VCC       | 3V3                      |
| SCK       | GPIO5/V_SPI_CS0/LoRa_SCK |
| SDA       | GPIO27/TOUCH7LoRa_MOSI   |
| RES       | GPIO17/U2_TXD            |
| RS        | GPIO21/SDA/V_SPI_HD      |
| CS        | GPIO23/V_SPI_D           |
| LEDA      | 3V3                      |

[Heltec WiFi LoRa 32 V2]: https://cdn.shopify.com/s/files/1/1509/1638/files/NodeMCU_ESP32_mit_OLED_Display_433Mhz_LoRa_Pinout_Diagram.pdf?11943858538744564432
[ST7735R]: https://www.az-delivery.de/en/products/1-77-zoll-spi-tft-display
