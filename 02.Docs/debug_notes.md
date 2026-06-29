## 2026-06-11
# ILI9488 LCD Bug
I follow AI chatbot to dev this LCD driver. I choose Color Mode like RGB565 - 16 bit

```
lcd_send_cmd(lcd, 0x3A); // COLMOD
uint8_t pixel_format = 0x55; // 16-bit RGB565
lcd_send_data(lcd, &pixel_format, 1);
ESP_LOGI(TAG, "LCD color mode set to RGB565...");
```
Then my module always show white screen. It's an issue.
I doubt and debug many things. I check my SPI protocol can initialize successfully.  So what is the reason why?
I changed the color mode, I see it can work well.

## 2026-06-29
# SD CARD Bug
I wrote the SD card SPI driver. I control LCD CS and SD CS pin manually.
When I wrote done. I check, the LOG always come out: 

```
I (354) SD_CARD_DRIVER: SD card device added to SPI bus... 
I (354) SD_CARD_DRIVER: Response[0] = 0x00 
I (364) SD_CARD_DRIVER: CMD0 response: 0x00 
I (364) LCD_DRIVER: Initializing LCD... 
I (374) LCD_DRIVER: LCD device added to SPI bus
```
I don't know the reason why. I need CMD0 response 0x01. 
I debug on Logic Analyzer. I see that whenever SPI SCK is at LOW level, LCD_CS and SD_CS pins are all pulled down.  
--> Actually, this is normal behaviour. Might be some hardware make any effects on these pins when Reset ESP32.

I doubt that SD_CS pin might has any problems because it is pulled down at the beginer of my program.
So, I tried to disable all LCD execution, but SD Card execution. Issue still appears, I see that the pull down of SD_CS pin is the normal behaviour.

I tried to check the SD_MOSI and the MOSI at LCD side on the "LCD 3.5" SPI Ili9488 TFT" module by VOM. I see it's not connected as I know before. And sometimes I thought I connect the pin row on the module wrongly, but it's not kkkkk

I see that SD_CS and MOSI can show normally on Logic Analyzer regarding SCK pulse (LCD side). So I think the bug comes from HARDWARE.
I disconnect MOSI (LCD side) and connect to SD_MOSI (SD side). I got the log below:

```
I (354) SD_CARD_DRIVER: Initializing SD card on SPI bus... 
I (364) SD_CARD_DRIVER: Idle[0] = 0xFF 
I (364) SD_CARD_DRIVER: Idle[1] = 0xFF 
I (364) SD_CARD_DRIVER: Idle[2] = 0xFF 
I (374) SD_CARD_DRIVER: Idle[3] = 0xFF 
I (374) SD_CARD_DRIVER: Idle[4] = 0xFF 
I (384) SD_CARD_DRIVER: Idle[5] = 0xFF 
I (384) SD_CARD_DRIVER: Idle[6] = 0xFF 
I (394) SD_CARD_DRIVER: Idle[7] = 0xFF 
I (394) SD_CARD_DRIVER: Idle[8] = 0xFF 
I (404) SD_CARD_DRIVER: Idle[9] = 0xFF 
I (404) SD_CARD_DRIVER: SD card device added to SPI bus... 
I (414) SD_CARD_DRIVER: Response[0] = 0xFF 
I (414) SD_CARD_DRIVER: Response[1] = 0xFF 
I (424) SD_CARD_DRIVER: Response[2] = 0xFF 
I (424) SD_CARD_DRIVER: Response[3] = 0xFF 
I (424) SD_CARD_DRIVER: Response[4] = 0xFF 
I (434) SD_CARD_DRIVER: Response[5] = 0xFF 
I (434) SD_CARD_DRIVER: Response[6] = 0xFF 
I (444) SD_CARD_DRIVER: Response[7] = 0xFF 
I (444) SD_CARD_DRIVER: CMD0 response: 0xFF 
I (454) main_task: Returned from app_main()
```

I thought something wrong in the Hardware. So I found a tutourial using Arduino IDE for developing SD card on this module. I see I have to connect SD side to LCD side. It works.

Nguyên nhân:
SPI bus của LCD và SD trên module không được nối như mình tưởng.
Phải nối chung các đường SCK/MOSI/MISO giữa header LCD và header SD.

Triệu chứng:
- Logic Analyzer thấy CMD0 gửi đúng.
- SD không phản hồi.
- Sau khi nối chung bus thì CMD0 trả 0x01.
