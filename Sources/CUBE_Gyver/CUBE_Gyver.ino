/*
   Светодиодный куб 16x16 на Arduino Nano и сдвиговых регистрах 74HC595

   Разработал VECT, модифицировал AlexGyver
   http://AlexGyver.ru/LEDcube/
   https://github.com/AlexGyver/GyverLibs
   2018 AlexGyver

  Доработан @Текущий Автор 10.2023.
Мне потребовался вывод русских символов. Но у Алекса
все матрицы символов(что то там закомментировано) кривые или не доделано
в коде что-то.
  Поэтому, метод text() не универсален.
Так же переделана последовательность переключения команд.
И не задействованы некоторые не красивые эффекты(синус какой то страшный).

Исходный код взят у Алекса, все вопросы по поводу его красоты необходимо адресовать автору.
Нет даже комментариев к методам. В видео тоже ни чего не сказано. 
Как говорит Алекс Гайвер в своих видео – он не программист и код тоже взял у кого то.
Из-за недостатка времени я не переписывал с нуля весь код.
Очень много времени ушло на изготовление печатной платы и сборке куба по своему дизайну.

Данный куб делался как подарок.
*/

#define INVERT_Y 1    // инвертировать по вертикали (если дождь идёт вверх)
#define INVERT_X 0    // инвертировать по горизонтали (если текст не читается)

#define XAXIS 0
#define YAXIS 1
#define ZAXIS 2

#define POS_X 0
#define NEG_X 1
#define POS_Z 2
#define NEG_Z 3
#define POS_Y 4
#define NEG_Y 5

#define BUTTON1 18
#define BUTTON2 19
#define RED_LED 5
#define GREEN_LED 7

#define TOTAL_EFFECTS 11  // количество эффектов

#define RAIN 1
#define PLANE_BOING 3
#define SEND_VOXELS 4
#define WOOP_WOOP 5
#define CUBE_JUMP 6
#define GLOW 7
#define TEXT 2
#define LIT 0

#include <SPI.h>
#include "GyverButton.h"
#include "russianFonts.h"

GButton butt1(BUTTON1);
GButton butt2(BUTTON2);

#define RAIN_TIME 260
#define PLANE_BOING_TIME 220
#define SEND_VOXELS_TIME 140
#define WOOP_WOOP_TIME 350
#define CUBE_JUMP_TIME 200
#define GLOW_TIME 8
#define TEXT_TIME 2000
#define CLOCK_TIME 500
#define WALKING_TIME 100

uint8_t cube[8][8];
int8_t currentEffect;
uint16_t timer;
uint16_t modeTimer;
bool loading;
int8_t pos;
int8_t vector[3];
int16_t coord[3];

void setup() {
  Serial.begin(9600);

  loading = true;
  currentEffect = RAIN;
  SPI.begin();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  randomSeed(analogRead(0));
  digitalWrite(GREEN_LED, HIGH);

  butt1.setIncrStep(5);         // настройка инкремента, может быть отрицательным (по умолчанию 1)
  butt1.setIncrTimeout(100);    // настрйока интервала инкремента (по умолчанию 800 мс)
  butt2.setIncrStep(-5);        // настройка инкремента, может быть отрицательным (по умолчанию 1)
  butt2.setIncrTimeout(100);    // настрйока интервала инкремента (по умолчанию 800 мс)

  //currentEffect = LIT; //Светим как фонарик.
  currentEffect = 0;
  changeMode();
}

void loop() {
  butt1.tick();
  butt2.tick();

  if (butt1.isSingle()) {
    currentEffect++;
    if (currentEffect >= TOTAL_EFFECTS) currentEffect = 0;
    changeMode();
  }
  if (butt2.isSingle()) {
    currentEffect--;
    if (currentEffect < 0) currentEffect = TOTAL_EFFECTS - 1;
    changeMode();
  }
  
  if (butt1.isIncr()) {                                 // если кнопка была удержана (это для инкремента)
    modeTimer = butt1.getIncr(modeTimer);               // увеличивать/уменьшать переменную value с шагом и интервалом
  }
  if (butt2.isIncr()) {                                 // если кнопка была удержана (это для инкремента)
    modeTimer = butt2.getIncr(modeTimer);               // увеличивать/уменьшать переменную value с шагом и интервалом
  }

  switch (currentEffect) {
    case 0:
    lit(); //светит все 
    break;
    
    case 1:
    rain(); 
    break;
    
    case 2:
    text(); 
    break;
    
    case 3: 
    glow(); //Заполнение до полной яркости. 
    break; 
    
    case 4:
    sendVoxels(); //Улитающие вверх штуки.
    break; 
    
    case 5: 
    planeBoing(); //Вертикальные горизонтальные штуки.
    break;
    
    case 6:
    cubeJump();//Меняющий размеры квадрат
    break; 
    
    case 7: 
    woopWoop();//типа сердца только квадрат дышащий
    break; 
    
    case 8: 
    walkingCube();  //бегающий квадрат.
    break;

    case 9:
    sinusThin();
    break; 

    default:
    lit(); 
    break;
  }

  renderCube();
}

void changeMode() {
  clearCube();
  loading = true;
  timer = 0;
  randomSeed(millis());
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  delay(500);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);

  switch (currentEffect) {
    case RAIN: modeTimer = RAIN_TIME; break;
    case PLANE_BOING: modeTimer = PLANE_BOING_TIME; break;
    case SEND_VOXELS: modeTimer = SEND_VOXELS_TIME; break;
    case WOOP_WOOP: modeTimer = WOOP_WOOP_TIME; break;
    case CUBE_JUMP: modeTimer = CUBE_JUMP_TIME; break;
    case GLOW: modeTimer = GLOW_TIME; break;
    case TEXT: modeTimer = TEXT_TIME; break;
    case LIT: modeTimer = CLOCK_TIME; break;
    case 8: modeTimer = RAIN_TIME; break;
    case 9: modeTimer = RAIN_TIME; break;
    case 10: modeTimer = WALKING_TIME; break;
  }
  modeTimer = RAIN_TIME;
}

void renderCube() {
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(SS, LOW);
    if (INVERT_Y) SPI.transfer(0x01 << (7 - i));
    else SPI.transfer(0x01 << i);
    for (uint8_t j = 0; j < 8; j++) {
      if (INVERT_X) SPI.transfer(cube[7 - i][j]);
      else SPI.transfer(cube[i][j]);
    }
    digitalWrite(SS, HIGH);
    //delay(1);
  }
}

void walkingCube() {
  if (loading) {
    clearCube();
    loading = false;
    for (byte i = 0; i < 3; i++) {
      // координата от о до 700!
      coord[i] = 300;
      vector[i] = random(3, 8) * 15;
    }
  }
  timer++;
  if (timer > modeTimer) {
    timer = 0;
    clearCube();
    for (byte i = 0; i < 3; i++) {
      coord[i] += vector[i];
      if (coord[i] < 1) {
        coord[i] = 1;
        vector[i] = -vector[i];
        vector[i] += random(0, 6) - 3;
      }
      if (coord[i] > 700 - 100) {
        coord[i] = 700 - 100;
        vector[i] = -vector[i];
        vector[i] += random(0, 6) - 3;
      }
    }

    int8_t thisX = coord[0] / 100;
    int8_t thisY = coord[1] / 100;
    int8_t thisZ = coord[2] / 100;

    setVoxel(thisX, thisY, thisZ);
    setVoxel(thisX + 1, thisY, thisZ);
    setVoxel(thisX, thisY + 1, thisZ);
    setVoxel(thisX, thisY, thisZ + 1);
    setVoxel(thisX + 1, thisY + 1, thisZ);
    setVoxel(thisX, thisY + 1, thisZ + 1);
    setVoxel(thisX + 1, thisY, thisZ + 1);
    setVoxel(thisX + 1, thisY + 1, thisZ + 1);
  }
}

void sinusFill() {
  if (loading) {
    clearCube();
    loading = false;
  }
  timer++;
  if (timer > modeTimer) {
    timer = 0;
    clearCube();
    if (++pos > 10) pos = 0;
    for (uint8_t i = 0; i < 8; i++) {
      for (uint8_t j = 0; j < 8; j++) {
        int8_t sinZ = 4 + ((float)sin((float)(i + pos) / 2) * 2);
        for (uint8_t y = 0; y < sinZ; y++) {
          setVoxel(i, y, j);
        }
      }
    }
  }
}

void sinusThin() {
  if (loading) {
    clearCube();
    loading = false;
  }
  timer++;
  if (timer > modeTimer) {
    timer = 0;
    clearCube();
    if (++pos > 10) pos = 0;
    for (uint8_t i = 0; i < 8; i++) {
      for (uint8_t j = 0; j < 8; j++) {
        int8_t sinZ = 4 + ((float)sin((float)(i + pos) / 2) * 2);
        setVoxel(i, sinZ, j);
      }
    }
  }
}

void rain() {
  if (loading) {
    clearCube();
    loading = false;
  }
  timer++;
  if (timer > modeTimer) {
    timer = 0;
    shift(NEG_Y);
    uint8_t numDrops = random(0, 5);
    for (uint8_t i = 0; i < numDrops; i++) {
      setVoxel(random(0, 8), 7, random(0, 8));
    }
  }
}

uint8_t planePosition = 0;
uint8_t planeDirection = 0;
bool looped = false;

void planeBoing() {
  if (loading) {
    clearCube();
    uint8_t axis = random(0, 3);
    planePosition = random(0, 2) * 7;
    setPlane(axis, planePosition);
    if (axis == XAXIS) {
      if (planePosition == 0) {
        planeDirection = POS_X;
      } else {
        planeDirection = NEG_X;
      }
    } else if (axis == YAXIS) {
      if (planePosition == 0) {
        planeDirection = POS_Y;
      } else {
        planeDirection = NEG_Y;
      }
    } else if (axis == ZAXIS) {
      if (planePosition == 0) {
        planeDirection = POS_Z;
      } else {
        planeDirection = NEG_Z;
      }
    }
    timer = 0;
    looped = false;
    loading = false;
  }

  timer++;
  if (timer > modeTimer) {
    timer = 0;
    shift(planeDirection);
    if (planeDirection % 2 == 0) {
      planePosition++;
      if (planePosition == 7) {
        if (looped) {
          loading = true;
        } else {
          planeDirection++;
          looped = true;
        }
      }
    } else {
      planePosition--;
      if (planePosition == 0) {
        if (looped) {
          loading = true;
        } else {
          planeDirection--;
          looped = true;
        }
      }
    }
  }
}

uint8_t selX = 0;
uint8_t selY = 0;
uint8_t selZ = 0;
uint8_t sendDirection = 0;
bool sending = false;

void sendVoxels() {
  if (loading) {
    clearCube();
    for (uint8_t x = 0; x < 8; x++) {
      for (uint8_t z = 0; z < 8; z++) {
        setVoxel(x, random(0, 2) * 7, z);
      }
    }
    loading = false;
  }

  timer++;
  if (timer > modeTimer) {
    timer = 0;
    if (!sending) {
      selX = random(0, 8);
      selZ = random(0, 8);
      if (getVoxel(selX, 0, selZ)) {
        selY = 0;
        sendDirection = POS_Y;
      } else if (getVoxel(selX, 7, selZ)) {
        selY = 7;
        sendDirection = NEG_Y;
      }
      sending = true;
    } else {
      if (sendDirection == POS_Y) {
        selY++;
        setVoxel(selX, selY, selZ);
        clearVoxel(selX, selY - 1, selZ);
        if (selY == 7) {
          sending = false;
        }
      } else {
        selY--;
        setVoxel(selX, selY, selZ);
        clearVoxel(selX, selY + 1, selZ);
        if (selY == 0) {
          sending = false;
        }
      }
    }
  }
}

uint8_t cubeSize = 0;
bool cubeExpanding = true;

void woopWoop() {
  if (loading) {
    clearCube();
    cubeSize = 2;
    cubeExpanding = true;
    loading = false;
  }

  timer++;
  if (timer > modeTimer) {
    timer = 0;
    if (cubeExpanding) {
      cubeSize += 2;
      if (cubeSize == 8) {
        cubeExpanding = false;
      }
    } else {
      cubeSize -= 2;
      if (cubeSize == 2) {
        cubeExpanding = true;
      }
    }
    clearCube();
    drawCube(4 - cubeSize / 2, 4 - cubeSize / 2, 4 - cubeSize / 2, cubeSize);
  }
}

uint8_t xPos;
uint8_t yPos;
uint8_t zPos;

void cubeJump() {
  if (loading) {
    clearCube();
    xPos = random(0, 2) * 7;
    yPos = random(0, 2) * 7;
    zPos = random(0, 2) * 7;
    cubeSize = 8;
    cubeExpanding = false;
    loading = false;
  }

  timer++;
  if (timer > modeTimer) {
    timer = 0;
    clearCube();
    if (xPos == 0 && yPos == 0 && zPos == 0) {
      drawCube(xPos, yPos, zPos, cubeSize);
    } else if (xPos == 7 && yPos == 7 && zPos == 7) {
      drawCube(xPos + 1 - cubeSize, yPos + 1 - cubeSize, zPos + 1 - cubeSize, cubeSize);
    } else if (xPos == 7 && yPos == 0 && zPos == 0) {
      drawCube(xPos + 1 - cubeSize, yPos, zPos, cubeSize);
    } else if (xPos == 0 && yPos == 7 && zPos == 0) {
      drawCube(xPos, yPos + 1 - cubeSize, zPos, cubeSize);
    } else if (xPos == 0 && yPos == 0 && zPos == 7) {
      drawCube(xPos, yPos, zPos + 1 - cubeSize, cubeSize);
    } else if (xPos == 7 && yPos == 7 && zPos == 0) {
      drawCube(xPos + 1 - cubeSize, yPos + 1 - cubeSize, zPos, cubeSize);
    } else if (xPos == 0 && yPos == 7 && zPos == 7) {
      drawCube(xPos, yPos + 1 - cubeSize, zPos + 1 - cubeSize, cubeSize);
    } else if (xPos == 7 && yPos == 0 && zPos == 7) {
      drawCube(xPos + 1 - cubeSize, yPos, zPos + 1 - cubeSize, cubeSize);
    }
    if (cubeExpanding) {
      cubeSize++;
      if (cubeSize == 8) {
        cubeExpanding = false;
        xPos = random(0, 2) * 7;
        yPos = random(0, 2) * 7;
        zPos = random(0, 2) * 7;
      }
    } else {
      cubeSize--;
      if (cubeSize == 1) {
        cubeExpanding = true;
      }
    }
  }
}

bool glowing;
uint16_t glowCount = 0;

void glow() {
  if (loading) {
    clearCube();
    glowCount = 0;
    glowing = true;
    loading = false;
  }

  timer++;
  if (timer > modeTimer) {
    timer = 0;
    if (glowing) {
      if (glowCount < 448) {
        do {
          selX = random(0, 8);
          selY = random(0, 8);
          selZ = random(0, 8);
        } while (getVoxel(selX, selY, selZ));
        setVoxel(selX, selY, selZ);
        glowCount++;
      } else if (glowCount < 512) {
        lightCube();
        glowCount++;
      } else {
        glowing = false;
        glowCount = 0;
      }
    } else {
      if (glowCount < 448) {
        do {
          selX = random(0, 8);
          selY = random(0, 8);
          selZ = random(0, 8);
        } while (!getVoxel(selX, selY, selZ));
        clearVoxel(selX, selY, selZ);
        glowCount++;
      } else {
        clearCube();
        glowing = true;
        glowCount = 0;
      }
    }
  }
}

uint8_t charCounter = 0;
uint8_t charPosition = 7;
uint8_t text_layer = 8; //Слой текста на котором отображается.

/*
  Выводит фиксированный текст. Из букв описанных в russianFonts.h.
  
 Метод привязан к массиву. Мне было лень писать универсальный метод, 
 потому что Алекс не дал нормальную матрицу русских букв, или что то не доделал.
 Поэтому тут есть магическая цифра 3. И куча других цифр в стиле Алекса Гайвера.
*/
void text() {
  if (loading) {
    clearCube();
    charPosition = -1;
    charCounter = 0;
    text_layer = 8;
    loading = false;
  }

  timer++;
  
  if (timer > 500) {
    timer = 0;

    if(text_layer == 0) 
    {
       text_layer = 8;
       charCounter++;
       if (charCounter > 3) {
            charCounter = 0;
       }           

      //Чистка
       for (uint8_t i = 0; i < 8; i++) {
          cube[i][0] = 0;    
        }       
    }

    for (uint8_t i = 0; i < 8; i++) {
            if(text_layer != 8) cube[i][text_layer] = 0;
            cube[i][text_layer - 1] = getRussianText(charCounter, i);
        }

    text_layer--;  
  }
}

//Выводим текст на русском. Правда у вас только 4 буквы. Нужно дописать матрицу в russianFonts.h. Алекс не дал нормальную.
uint8_t getRussianText(uint8_t charPos, uint8_t i)
{
   return rusText[charPos][i];
}

void lit() {
  if (loading) {
    clearCube();
    for (uint8_t i = 0; i < 8; i++) {
      for (uint8_t j = 0; j < 8; j++) {
        cube[i][j] = 0xFF;
      }
    }
    loading = false;
  }
}

void setVoxel(uint8_t x, uint8_t y, uint8_t z) {
  cube[7 - y][7 - z] |= (0x01 << x);
}

void clearVoxel(uint8_t x, uint8_t y, uint8_t z) {
  cube[7 - y][7 - z] ^= (0x01 << x);
}

bool getVoxel(uint8_t x, uint8_t y, uint8_t z) {
  return (cube[7 - y][7 - z] & (0x01 << x)) == (0x01 << x);
}

void setPlane(uint8_t axis, uint8_t i) {
  for (uint8_t j = 0; j < 8; j++) {
    for (uint8_t k = 0; k < 8; k++) {
      if (axis == XAXIS) {
        setVoxel(i, j, k);
      } else if (axis == YAXIS) {
        setVoxel(j, i, k);
      } else if (axis == ZAXIS) {
        setVoxel(j, k, i);
      }
    }
  }
}

void shift(uint8_t dir) {

  if (dir == POS_X) {
    for (uint8_t y = 0; y < 8; y++) {
      for (uint8_t z = 0; z < 8; z++) {
        cube[y][z] = cube[y][z] << 1;
      }
    }
  } else if (dir == NEG_X) {
    for (uint8_t y = 0; y < 8; y++) {
      for (uint8_t z = 0; z < 8; z++) {
        cube[y][z] = cube[y][z] >> 1;
      }
    }
  } else if (dir == POS_Y) {
    for (uint8_t y = 1; y < 8; y++) {
      for (uint8_t z = 0; z < 8; z++) {
        cube[y - 1][z] = cube[y][z];
      }
    }
    for (uint8_t i = 0; i < 8; i++) {
      cube[7][i] = 0;
    }
  } else if (dir == NEG_Y) {
    for (uint8_t y = 7; y > 0; y--) {
      for (uint8_t z = 0; z < 8; z++) {
        cube[y][z] = cube[y - 1][z];
      }
    }
    for (uint8_t i = 0; i < 8; i++) {
      cube[0][i] = 0;
    }
  } else if (dir == POS_Z) {
    for (uint8_t y = 0; y < 8; y++) {
      for (uint8_t z = 1; z < 8; z++) {
        cube[y][z - 1] = cube[y][z];
      }
    }
    for (uint8_t i = 0; i < 8; i++) {
      cube[i][7] = 0;
    }
  } else if (dir == NEG_Z) {
    for (uint8_t y = 0; y < 8; y++) {
      for (uint8_t z = 7; z > 0; z--) {
        cube[y][z] = cube[y][z - 1];
      }
    }
    for (uint8_t i = 0; i < 8; i++) {
      cube[i][0] = 0;
    }
  }
}

void drawCube(uint8_t x, uint8_t y, uint8_t z, uint8_t s) {
  for (uint8_t i = 0; i < s; i++) {
    setVoxel(x, y + i, z);
    setVoxel(x + i, y, z);
    setVoxel(x, y, z + i);
    setVoxel(x + s - 1, y + i, z + s - 1);
    setVoxel(x + i, y + s - 1, z + s - 1);
    setVoxel(x + s - 1, y + s - 1, z + i);
    setVoxel(x + s - 1, y + i, z);
    setVoxel(x, y + i, z + s - 1);
    setVoxel(x + i, y + s - 1, z);
    setVoxel(x + i, y, z + s - 1);
    setVoxel(x + s - 1, y, z + i);
    setVoxel(x, y + s - 1, z + i);
  }
}

void lightCube() {
  for (uint8_t i = 0; i < 8; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      cube[i][j] = 0xFF;
    }
  }
}

void clearCube() {
  for (uint8_t i = 0; i < 8; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      cube[i][j] = 0;
    }
  }
}
