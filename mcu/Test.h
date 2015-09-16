#ifndef TEST_H
#define TEST_H

// Test Point Constants
#define TEST_POINT_1 2
#define TEST_POINT_2 1
#define TEST_POINT_3 0

void ledtest(void)
{
    /*************************************************************
        LED Test:
            Flash LEDs in this sequence with 200 ms delay
                Green Yellow Red Yellow Green

        Expected output: 
            Green, Yellow, Red, Yellow, Green LED flash
            in that order

        Pass
    *************************************************************/
    initLEDs();

    LED_ON(GREEN);
    _delay_ms(200);
    LED_OFF(GREEN);

    LED_ON(YELLOW);
    _delay_ms(200);
    LED_OFF(YELLOW);

    LED_ON(RED);
    _delay_ms(200);
    LED_OFF(RED);

    LED_ON(YELLOW);
    _delay_ms(200);
    LED_OFF(YELLOW);

    LED_ON(GREEN);
    _delay_ms(200);
    LED_OFF(GREEN);

}

void irtest(void)
{
    /*************************************************************
        IR Test:
            Initialize IR which turns on the sensor block LED

        Expected output: 
            Green LED and Sensor block LED turns on

        Pass (had to modify R feedback to 100 kOhms)
    *************************************************************/
    initLEDs();
    initIR();

    LED_ON(GREEN);
}

void computeruarttest(void)
{
    /*************************************************************
        Computer UART Test:
            writes hello world to GPS

        Expected output: 
            Green LED turns on and computer UART receives
                "Hello, World!" plus a newline

        Pass
    *************************************************************/

    initLEDs();
    initUARTs();

    putc0('H');
    putc0('e');
    putc0('l');
    putc0('l');
    putc0('o');
    putc0(',');
    putc0(' ');
    putc0('W');
    putc0('o');
    putc0('r');
    putc0('l');
    putc0('d');
    putc0('!');
    putc0('\n');

    LED_ON(GREEN);
}

void gpsuarttest(void)
{
    /*************************************************************
        GPS UART Test:
            write the following packet to the GPS:
                [ 10 8E A5 00 00 00 00 10 03 ]
            which turns off the auto packets and should receive:
                [ 10 8F A5 00 00 00 00 10 03 ]
            which it writes to the computer UART

        Expected output: 
            Green LED turns on and computer UART receives
                [ 10 8F A5 00 00 00 00 10 03 ]

        Pass
    *************************************************************/

    initLEDs();
    initUARTs();

    putc1(0x10);
    putc1(0x8E);
    putc1(0xA5);
    putc1(0x00);
    putc1(0x00);
    putc1(0x00);
    putc1(0x00);
    putc1(0x10);
    putc1(0x03);

    // Receive 9 bytes off the GPS UART
    for(int i = 0; i < 9; i++)
    {
        // Wait until there is a character to read
        while(!(UCSR1A & (1 << RXC1)));
        // Read that character
        int get = getc1();
        if(get > -1)
        {
            uint8_t byte = (get & 0xFF);
            putc0(byte);
        }
        else if(get == -1)
        {
            // EB for Empty Buffer (should never happen)
            putc0(0xEB);
        }
        else if(get == -2)
        {
            // FE for Framing Error
            putc0(0xFE);
        }
        else if(get == -3)
        {
            // 0E for 0vErrun
            putc0(0x0E);
        }
    }

    LED_ON(GREEN);
}

void bmp180test(void)
{
    /*************************************************************
        BMP 180 Test
            Initializes, calibrates, and reads BMP 180 sensor

        Expected output: 
            Green LED comes on and computer UART receives
                Temperature and Pressure
            Red LED means there was an initialization error
            Yellow LED means there was a read error

        Pass
            Temperature: 25.95625 C     0x00001039
            Pressure: 100125 Pa         0x0001871D

    *************************************************************/

    // Initialize LEDs, UART, and TWI
    initLEDs();
    initUARTs();
    initTWI();

    // Initialize sensor
    uint8_t bmpiniterror = initBMP180();
    if(bmpiniterror)
    {
        // Red means initialization error
        LED_ON(RED);
        putc0(bmpiniterror);
    }

    // Read sensor
    int32_t temperature;
    int32_t pressure;
    uint8_t bmpreaderror = readBMP180Sensors(&temperature,&pressure);
    if(bmpreaderror)
    {
        // Yellow means read error
        LED_ON(YELLOW);
    }

    //Send data over computer UART
    putc0((uint8_t)(temperature >> 24));
    putc0((uint8_t)(temperature >> 16));
    putc0((uint8_t)(temperature >> 8));
    putc0((uint8_t)(temperature >> 0));

    putc0((uint8_t)(pressure >> 24));
    putc0((uint8_t)(pressure >> 16));
    putc0((uint8_t)(pressure >> 8));
    putc0((uint8_t)(pressure >> 0));

    LED_ON(GREEN);
}

void bme280test()
{
    /*************************************************************
        BME 280 Test
            Initializes, calibrates, and reads BME 280 sensor

        Expected output: 
            Green LED comes on and computer UART receives
                Temperature, Pressure, and Humidity
            Red LED means there was an initialization error
            Yellow LED means there was a read error or a
                conversion error

        Pass
            Temperature: 29.778°C       0x0b49
            Pressure: 100444 Pa         0x0001885c
            Humidity: 55.495 %rH        0x0000ddfb

    *************************************************************/

    // Initialize LEDs, UART, and TWI
    initLEDs();
    initUARTs();
    initTWI();

    // Initialize sensor
    uint8_t bmpiniterror = initBME280();
    if(bmpiniterror)
    {
        LED_ON(RED);
    }

    int16_t temperature = 0xFFFF;
    uint32_t pressure = 0xFFFFFFFF;
    uint32_t humidity = 0xFFFFFFFF;

    // Start Conversion
    uint8_t startError = startBME280Readout16XOversampling();
    if(startError)
    {
        LED_ON(YELLOW);
    }
    // Wait for conversion to complete
    _delay_ms(113);
    // Readout conversion
    uint8_t readError = getFinishedBME280Readout(&temperature,&pressure,&humidity);
    if(readError)
    {
        LED_ON(YELLOW);
    }

    // Send data over computer UART
    putc0((uint8_t)(temperature >> 8));
    putc0((uint8_t)(temperature >> 0));

    // Send data over computer UART
    putc0((uint8_t)(pressure >> 24));
    putc0((uint8_t)(pressure >> 16));
    putc0((uint8_t)(pressure >> 8));
    putc0((uint8_t)(pressure >> 0));

    // Send data over computer UART
    putc0((uint8_t)(humidity >> 24));
    putc0((uint8_t)(humidity >> 16));
    putc0((uint8_t)(humidity >> 8));
    putc0((uint8_t)(humidity >> 0));

    LED_ON(GREEN);
}

#endif
