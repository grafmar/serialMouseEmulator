/* Serial mouse emulator
 * 2020 grafmar based on code of
 * 2018 Meido-Tek Productions (Lameguy64)
 *
 * This sketch is to emulate a serial mouse with an Arduino. This can be used to prevent
 * the PC from going to sleep or lock. This sketch can also be extendet to emulate a mouse
 * using buttons or potentiometers.
 *
 * This requires hardware changes on the Arduino board
 *
 * The schematic for the converter should be included along with this file.
 *
 * Usage Instructions:
 *  - Upload this sketch to your converter's Arduino.
 *  - L13 light will blink every time the mouse driver tries to initialize the mouse (by
 *    toggling RTS) to indicate that the converter is working properly and the mouse driver
 *    should recognize it as a 3-button Logitech mouse.
 *  - TX-LED will light up when data is sent through serial. For example when a mouse event
 *    is sent.
 *
 *  As always, feel free to improve upon this chode and issue a pull request to merge with
 *  the main repository.
 */


 /* Pin settings, feel free to change to what you find most convenient to use.
  */
#define RTS_PROBE       7   /* Must connect to Pin 7 (RTS) of the USB-serial converter chip */
#define TEN_SECONDS     10000u  // 10s in ms

bool rts_status = false;

int x_status = 0;
int y_status = 0;

unsigned long lastMillis = 0;
uint16_t patternCounter = 0;
int16_t lastPosX = 0;
int16_t lastPosY = 0;


/* Serial mouse packet encoding routine */
/* Encodes a 3 byte mouse packet that complies with Microsoft Mouse protocol */
void encodePacket(int8_t x, int8_t y, bool lb, bool rb, uint8_t* output) {
  uint8_t cx = x;
  uint8_t cy = y;

  output[0] = 0x40 |            /* Packet 0 (start packet bit)*/
    (rb << 4) | (lb << 5) |     /* Mouse buttons */
    ((cx >> 6) & 0x03) |        /* last 2 bits of X */
    (((cy >> 6) & 0x3) << 2);   /* Last 2 bits of Y */

  output[1] = cx & 0x3f;        /* Packet 1 ( first 6 bits of X ) */
  output[2] = cy & 0x3f;        /* Packet 2 ( first 6 bits of Y ) */
}

/* Three button mouse event */
void sendMSMouseEvent(int8_t deltaX, int8_t deltaY, bool lb, bool rb) {
  uint8_t packet[3];
  encodePacket(deltaX, deltaY, lb, rb, packet);

  Serial.write(packet, 3);
}

/* Four button mouse event */
void sendMSMouseEvent(int8_t deltaX, int8_t deltaY, bool lb, bool rb, bool mb) {
  uint8_t packet[4];
  encodePacket(deltaX, deltaY, lb, rb, packet);
  if (mb) {
    packet[3] = 0x20;
  }
  else {
    packet[3] = 0x00;
  }

  Serial.write(packet, 4);
}

void setup() {
  /* Set pin for RTS probe as input */
  pinMode(RTS_PROBE, INPUT);

  /* For blinking L13 LED */
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  /* Initialize serial for mouse data */
  Serial.begin(1200, SERIAL_7N1);
}


void loop() {
  /* Send init bytes when RTS has been toggled to high (inverted on USB-serial chip) */
  if (!digitalRead(RTS_PROBE)) {
    if (!rts_status) {
      // Send 'M' after 14ms
      delay(14);
      Serial.write('M');

      /* For Logitec mouse a '3' is sent after 63ms */
      delay(63);
      //Serial.write( '3' );

      rts_status = true;
      digitalWrite(13, HIGH);
      delay(500);
      digitalWrite(13, LOW);
    }
  }
  else {
    rts_status = false;
  }

  // move the mouse in a pattern every 10s
  if ((millis() - lastMillis) > TEN_SECONDS) {
    if (patternCounter > 360u) {
      lastMillis = millis();  // reset lastMillis
      patternCounter = 0u;    // reset pattern counter
    } else {
      const int16_t radius = 50;
      int16_t posX = radius * sin(1 * 2 * PI * patternCounter / 360); // lying 8 pattern -> oo
      int16_t posY = radius * sin(2 * 2 * PI * patternCounter / 360); // lying 8 pattern -> oo
      //int16_t posX = radius * sin(2 * PI * patternCounter / 360);   // circle
      //int16_t posY = - radius * cos(2 * PI * patternCounter / 360); // circle
      int16_t deltaX = posX - lastPosX;
      int16_t deltaY = posY - lastPosY;
      if ((deltaX != 0) || (deltaY != 0)) {
        sendMSMouseEvent(deltaX, deltaY, false, false);
      }
      //sendMSMouseEvent(posX - lastPosX, posY - lastPosY, false, false, false);
      delay(2);
      lastPosX = posX;
      lastPosY = posY;
      patternCounter++;
    }
  }
}
