#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Essentials/fundamental variables
float voltage = 0;
float disp_voltage = 0;
float ref = 0;
float current = 0;
float disp_current = 0;
float temperature = 0;
float initial_temperature = 0;

//other internal, system-ic
int set = 0;
int sysch = 0;
int x = 0;
int on_state = 0;
int e_start = 0;
int charging = 0;
int fan_pwm = 0;
int screen = 1;
int control = 1;
int mah = 0;
int r_mah = 0;
int energy = 0;
int sel = 1;
int pwr_limit = 0;

//miscellaneous variables
int adc_voltage = 0;
int b_bar_width = 0;
int battery_level = 0;
int disp_battery_level = 0;
int power = 0;
int disp_power = 0;
int ttem = 0;
int scr_block = 0;
int enter = 0;

float tteh = 0;

//dynamic variables (v)
int v1 = 0; //used for repeating once for the low battery alert
int v2 = 0; //used for setting the battery to the instantaneous soc on startup/only once
int v3 = 0; //used for setting the startTime variable equal to millis on startup
int v4 = 0; //used for setting disp_voltage equal to instantaneous voltage once in the loop
int v5 = 0; //used for setting disp_current equal to instantaneous current once in the loop
int v6 = 0; //used for setting start_temperature equal to temperature on startup/once in the loop.
int v7 = 0; //used for repeating the temperature once when temperature goes above 48*C
int v8 = 0; //repeat-once-button press to change between screens
int v9 = 0; //used to reset mah on connection to charger
int v10 = 0; //used to repeat button press only once
int v11 = 0;//repeat-once-d2-press
int v12 = 0;//repeat-once 'enter = !enter'
int v13 = 0;//repeat once of increment of pwr_limit by 10

//time measurement variables
unsigned long lastTime = 0;
unsigned long startTime = 0;
unsigned long rn = 0;
unsigned long rn1 = 0; //for measuring every second - used in mah calculation

//switch variables
int chargers = 0;
int inverter = 0;
int lamp = 0;
int psu = 0;

void setup() {
 
 e_start = 0;

 //inputs
 pinMode(2, INPUT_PULLUP);
 pinMode(4, INPUT_PULLUP);
 pinMode(7, INPUT_PULLUP);
 
 //outputs
 pinMode(9, OUTPUT); //pwm fan

 digitalWrite(12, HIGH);

 if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
  Serial.println(F("Startup failed!"));
  for(;;);
 }


  if(set == 0){
    for(int i = 0; i < 10; i++){
      ref+= analogRead(A1);
    }
    ref = ref/10;
    ref = ref * (5.0 / 1023.0);
    set = 1;
  }

  EEPROM.get(1, pwr_limit);
}

void ShutDown(){
  display.clearDisplay();
  display.setCursor(12, 30);
  display.print("Shutting Down...");
  display.display();
  delay(1800);
  display.clearDisplay();
  display.display();
  on_state = 0;
  digitalWrite(12, LOW);
}

void loop() {
 
 //obtaining fundamentals

  voltage = 0;
  current = 0;

  for(int i = 0; i < 100; i++){
    voltage+= analogRead(A0);
  }

  voltage = voltage * (4.968 / 1023.0);
  voltage = voltage * 4.97;

  voltage = voltage / 100;
  disp_voltage = voltage;

  for(int i = 0; i < 100; i++){
    current+= analogRead(A1);
  }

  current = current / 100;
 
  current = current * (4.968 / 1023.0);
  current = current - ref;
  current = current / 0.066;

  power = voltage * current;

  temperature = analogRead(A3);
  temperature = temperature * (4.968 / 1023.0);
  temperature = 10000.0 * (5.0 / temperature - 1.0);
  temperature = 1.0 / (1.0 / 298.5 + log(temperature / 10000.0) / 3950.0);
  temperature = temperature - 273.15; //obtains temperature in *C

  if(v6 == 0){
    initial_temperature = temperature;
    v6 = 1;
  }

  if(power < 0){
    power = power * -1;
  }

  if(disp_power < power){
    disp_power = disp_power * 0.2 + power * 0.8;
  }

  if(disp_power > power){
    disp_power = disp_power * 0.85 + power * 0.15;
  }

//obtaining fundamentals end here

//battery and calculations begin

  battery_level = map(disp_voltage*1000, 9800, 12600, 0, 100); //calculation of battery level to display

  if(battery_level > 100){
    battery_level = 100;
  }
  if(battery_level < 0){
    battery_level = 0;
  }

  if(v2 == 0){
    disp_battery_level = battery_level;
    v2 = 1;
  }

  if(disp_battery_level > battery_level){
    if(charging == 0){
      disp_battery_level = battery_level;
    }
  }
  if(disp_battery_level != battery_level){
    if(charging == 1){
      disp_battery_level = battery_level;
    }
  }

//battery and calculations end here

//system check starts here

if(sysch == 0){
  display.clearDisplay();
  display.setCursor(20, 50);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.print("System check...");
  display.drawRect(40, 10, 40, 30, WHITE);
  display.setCursor(52, 16);
  display.print("SYS");

  display.drawCircle(76, 30, 5, WHITE); //search icon
  display.fillCircle(76, 30, 4, BLACK);

  display.drawLine(80, 34, 90, 44, WHITE);
  display.drawLine(81, 34, 91, 44, WHITE);

  display.display();
  delay(1770);

  display.clearDisplay();

  if(disp_battery_level > 0){
    on_state = 1;
  }
  if(disp_battery_level < 1){
    display.clearDisplay();
    display.setTextSize(3);
    display.setTextColor(WHITE);

    //drawing the low battery icon
    display.drawRoundRect(43, 16, 40, 20, 2, WHITE);
    display.fillRoundRect(82, 20, 3, 12, 1, WHITE);
    display.fillRect(43, 16, 3, 20, WHITE);
    display.drawRoundRect(44, 17, 38, 18, 2, BLACK);

    display.setCursor(39, 19);
    display.setCursor(30, 48);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println("Battery low!");
    display.display();

    if(v3 == 0){
      startTime = millis();
      v3 = 1;
    }

    while(millis() - startTime < 10000){
      if(digitalRead(2) == LOW){
        e_start = 1;
        on_state = 1;
        display.clearDisplay();
        display.setCursor(20, 30);
        display.print("Emergency mode");
        display.display();
        delay(1470);
        display.clearDisplay();
        display.display();
        break;
      }
    }
      if(e_start == 0){
        ShutDown();
      }
  }
  sysch = 1;
}

//system check ends here

//main on state code starts here 

if(on_state == 1){
  digitalWrite(12, HIGH);

  b_bar_width = map(disp_battery_level, 0, 100, 0, 38);

  if(b_bar_width > 38){
    b_bar_width = 38;
  }
  if(b_bar_width < 0){
    b_bar_width = 0;
  }

  display.clearDisplay();

  //screen button selection sensing
  if(digitalRead(4) == LOW){
    if(enter == 0){
    if(v8 == 0){
      screen++;
      sel = 1;
      v8 = 1;
    }
    }
  }

  if(digitalRead(4) == HIGH){
    v8 = 0;
  }
  if(screen > 4){
    screen = 1;
  }

  if(digitalRead(7) == LOW){
      if(v10 == 0){
        sel++;
        v10 = 1;
      }
    }
  if(digitalRead(7) == HIGH){
    v10 = 0;
   }
  if(sel > 4){
    sel = 1;
   }


  if(charging == 0){
    if(v9 == 0){
      mah = 0;
      v9 = 1;
    }
    if(millis() - rn1 >= 1000){
    mah = mah + (current * 1000)/3600;
  }}
  if(charging == 1){
    v9 = 0;
  }

//test
r_mah = 9000;

  tteh = (int)(r_mah / disp_current * 1000);

  if(tteh > 37){
    tteh = 37.0;
  }
  ttem = (r_mah / current * 1000) - tteh;
  ttem = ttem * 60;
  if(ttem > 59){
    ttem = 59;
  }
  if(ttem < 1){
    ttem = 0;
  }

  if(digitalRead(2) == HIGH){
    v11 = 0;
  }



  if(screen == 1){
    //drawing screen selector
    display.fillCircle(6, 10, 2, WHITE);
    display.drawCircle(12, 10, 2, WHITE);
    display.drawCircle(18, 10, 2, WHITE);
    display.drawCircle(24, 10, 2, WHITE);
  //drawing the battery bar's width
  display.drawRoundRect(38, 5, 40, 15, 3, WHITE);
  display.fillRoundRect(77, 9, 3, 7, 1, WHITE);

  display.fillRoundRect(38, 5, b_bar_width, 15, 3, WHITE);

  display.drawRoundRect(39, 6, 38, 13, 3, BLACK);

  display.setCursor(52, 25);
  display.print(disp_battery_level);
  display.println("%");
  
  display.setCursor(80, 42);
  display.setTextSize(2);
  display.print(disp_power);
  display.setTextSize(1);
  display.setCursor(120, 48);
  display.println("W");

  //clock icon
  display.fillCircle(9, 53, 4, WHITE);
  display.drawFastHLine(9, 53, 2, BLACK);
  display.drawFastVLine(9, 51, 3, BLACK);

  display.setCursor(15, 50);
  display.print(tteh, 0);
  display.print("h");
  display.print(":"); 
  display.print(ttem);
  display.print("m");

  display.setCursor(95, 0);
  display.print(temperature, 0);
  display.setCursor(110, 0);
  display.print("*C");
  }

  if(screen == 2){
    //drawing screen selector
    display.drawCircle(6, 10, 2, WHITE);
    display.fillCircle(12, 10, 2, WHITE);
    display.drawCircle(18, 10, 2, WHITE);
    display.drawCircle(24, 10, 2, WHITE);

    display.setCursor(10, 20);
    display.print("Voltage:");
    display.setCursor(10, 30);
    display.print("Current:");
    display.setCursor(10, 40);
    display.print("Power  :");
    display.setCursor(10, 50);
    display.print("Runtime:");

  }

  if(screen == 3){
    //drawing screen selector
    display.drawCircle(6, 10, 2, WHITE);
    display.drawCircle(12, 10, 2, WHITE);
    display.fillCircle(18, 10, 2, WHITE);
    display.drawCircle(24, 10, 2, WHITE);

    display.setCursor(37, 7);
    display.print("Controls:");

    display.setCursor(10, 20);
    display.print("Chargers:");
    display.setCursor(10, 30);
    display.print("Inverter:");
    display.setCursor(10, 40);
    display.print("Lamp:");
    display.setCursor(10, 50);
    display.print("Power supply:");

    if(sel == 1){
      display.fillTriangle(95, 21, 98, 23, 95, 26, WHITE);
      if(digitalRead(2) == LOW){
        if(v11 == 0){
          chargers = !chargers;
          v11 = 1;
        }
      }
    }

    if(sel == 2){
      display.fillTriangle(95, 31, 98, 33, 95, 36, WHITE);
      if(digitalRead(2) == LOW){
        if(v11 == 0){
          inverter = !inverter;
          v11 = 1;
        }
      }
    }

    if(sel == 3){
      display.fillTriangle(95, 41, 98, 43, 95, 46, WHITE);
      if(digitalRead(2) == LOW){
        if(v11 == 0){
          lamp = !lamp;
          v11 = 1;
        }
      }
    }

    if(sel == 4){
      display.fillTriangle(95, 51, 98, 53, 95, 56, WHITE);
      if(digitalRead(2) == LOW){
        if(v11 == 0){
          psu = !psu;
          v11 = 1;
        }
      }
    }

    if(chargers == 0){
      display.drawCircle(110, 23, 3, WHITE);
    }
    if(chargers == 1){
      display.fillCircle(110, 23, 3, WHITE);
    }

    if(inverter == 0){
      display.drawCircle(110, 33, 3, WHITE);
    }
    if(inverter == 1){
      display.fillCircle(110, 33, 3, WHITE);
    }

    if(lamp == 0){
      display.drawCircle(110, 43, 3, WHITE);
    }
    if(lamp == 1){
      display.fillCircle(110, 43, 3, WHITE);
    }

    if(psu == 0){
      display.drawCircle(110, 53, 3, WHITE);
    }
    if(psu == 1){
      display.fillCircle(110, 53, 3, WHITE);
    }

  }

  if(screen == 4){
    //drawing screen selector
    display.drawCircle(6, 10, 2, WHITE);
    display.drawCircle(12, 10, 2, WHITE);
    display.drawCircle(18, 10, 2, WHITE);
    display.fillCircle(24, 10, 2, WHITE);

    display.setCursor(37, 7);
    display.print("Settings:");

    display.setCursor(15, 22);
    display.print("Power limit:");
    display.setCursor(97, 22);
    display.print(pwr_limit);
    display.setCursor(120, 22);
    display.print("W");

    display.setCursor(15, 32);
    display.print("RGB color:");
    display.setCursor(97, 32);
    display.print("Red"); //must be replaced by variable

  if(sel == 1){
    display.fillTriangle(5, 23, 8, 25, 5, 28, WHITE);
      if(digitalRead(2) == LOW){
        if(v12 == 0){
          enter = !enter;
            if(enter == 0){
              EEPROM.put(1, pwr_limit);
            }
            v12 = 1;
        }
      }

      if(digitalRead(2) == HIGH){
        v12 = 0;
      }

    if(enter == 1){
      display.drawRect(94, 19, 34, 12, WHITE);

      if(digitalRead(4) == LOW){
        if(v13 == 0){
          pwr_limit+= 10;
          v13 = 1;
        }
      }
      if(digitalRead(4) == HIGH){
        v13 = 0;
      }
      if(pwr_limit > 300){
        pwr_limit = 0;
      }
    }
  }
  if(sel == 2){
    display.fillTriangle(5, 33, 8, 35, 5, 38, WHITE);
  }
  if(sel == 3){
    display.fillTriangle(5, 43, 8, 45, 5, 48, WHITE);
  }
  if(sel == 4){
    display.fillTriangle(5, 53, 8, 55, 5, 58, WHITE);
  }
}

//overtemperature warning
  if(temperature > 48){
    if(v7 == 0){
      display.clearDisplay();
      display.setCursor(15, 30);
      display.setTextSize(1);
      display.print("Overtemperature!");
      display.display();

      delay(1300); //to be replaced with millis delay

      display.clearDisplay();
      display.display();

      v7 = 1;
      }
    }

    if(temperature < 32){
      v7 = 0;
    }


  display.display(); //main display-updating line!

//Display on state ends here

//Background/hardware processes start here


  fan_pwm = map(temperature, 22, 37, 50, 255);
  if(fan_pwm < 0){
    fan_pwm = 0;
  }
  if(fan_pwm > 255){
    fan_pwm = 255;
  }
  analogWrite(9, fan_pwm);




  if(disp_battery_level < 6){
    if(e_start == 0){
      if(v1 == 0){
        display.clearDisplay();
        display.drawRect(10, 10, 108, 40, WHITE);
        display.setCursor(30, 25);
        display.print("Battery low!");

        if(rn == 0){
          rn = millis();
        }
        if(millis() - rn > 2000){
          display.clearDisplay();
          v1 = 1;
        }
      }
    }
  }
  if(disp_battery_level > 5){
    v1 = 0;
    rn = 0;
  }
  if(disp_battery_level < 1){
    if(e_start == 0){
      ShutDown();
    }
  }
}

//battery bar, emergency, shutdown, etc. end here
}
