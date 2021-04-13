#include <DS1302.h> //library for the clock module
#include <LiquidCrystal_I2C.h> //library for the I2C LCD module
#include <dht11.h> //library for the temperature/humidity sensor

#define REMOTE_BUTTON_A 2 //digital pin 2 for button A of the remote control
#define REMOTE_BUTTON_B 3 //digital pin 3 for button B of the remote control
#define REMOTE_BUTTON_C 4 //digital pin 4 for button C of the remote control
#define REMOTE_BUTTON_D 5 //digital pin 5 for button D of the remote control

#define SOIL_MOISTURE_PIN A0 //analog pin 0 for soil moisture sensor
#define LDR_PIN A1 //analog pin 1 for light dependent resistor
#define DHT11_PIN 9 //digital pin 9 for the temperature/humidity sensor
#define LED_SOIL_MOISTURE_PIN 10 //digital pin 10 for the yellow LED to show the state of the soil moisture sensor
#define LED_LDR_PIN 11 //digital pin 11 for the blue LED to show the state of the LDR 
#define LED_DHT11_PIN 12 //digital pin 12 for the red LED to show the state of the DHT11 (temperature) sensor 

#define LCD_ROWS 4 //amount of rows of the LCD
#define LCD_COLUMNS 20 //amount of columns of the LCD

#define MENU_LIGHTNING_INFO_ID 1 //mainmenu: lightning --> submenu: info --> id: 1
#define MENU_LIGHTNING_SETTINGS_ID 2 //mainmenu: lightning --> submenu: settings --> id: 2

#define MENU_PLANTS_INFO_ID 3 //mainmenu: plants --> submenu: info --> id: 3
#define MENU_PLANTS_SETTINGS_ID 4 //mainmenu: plants --> submenu: settings --> id: 4

#define MENU_TEMPERATURE_INFO_ID 5 //mainmenu: temperature --> submenu: info --> id: 5
#define MENU_TEMPERATURE_SETTINGS_ID 6 //mainmenu: temperature --> submenu: settings --> id: 6

#define MENU_TIME_INFO_ID 7 //mainmenu: time --> submenu: info --> id: 7
#define MENU_TIME_SETTINGS_ID 8 //mainmenu: time --> submenu: settings --> id: 8

#define TEMPERATURE_SYMBOX_ID 1
#define SOIL_MOISTURE_SYMBOL_ID 2
#define LIGHT_INTENSITY_SYMBOL_ID 3

int current_menu_index = 0; //variable to store the current menu index
int current_menu_id = 0; //variable to store the current menu id
bool in_submenu = false; //variable to store the state of the program: in a submenu or not
bool in_infomenu = true; //variable to store the state of the program: in the information menu or not

int current_water_value = 0; //variable to store the value measured by the soil moisture sensor
int water_check_value = 800; //variable to store the compare value for the soil moisture
bool water_check_state = false; //variable to store the state of the plants settings: true = water automatically when compare value is reached | false = don't water automatically when compare value is reached

int current_brightness_value = 0; //variable to store the value measured by the light dependent resistor
int brightness_check_value = 80; //variable to store the compare value for the light intensity
bool brightness_check_state = false; //variable to store the state of the lighting settings: true = turn on lights automatically when compare value is reached | false = don't turn on lights automatically when compare value is reached

int current_temperature_value = 0; //variable to store the value measured by the DHT11 (temperature sensor)
int temperature_check_value = 25; //variable to store the compare value for the temperature
bool temperature_check_state = false; //variable to store the state of the temperature settings: true = turn on heater automatically when compare value is reached | false = don't turn on heater automatically when compare value is reached

byte temperature_symbol[] = { //selfmade symbol for the temperature
  B01110,
  B01010,
  B01010,
  B01010,
  B01110,
  B11111,
  B11111,
  B01110
};

byte soil_moisture_symbol[] = { //selfmade symbol for the soil moisture
  B00100,
  B00100,
  B01010,
  B01010,
  B10001,
  B10001,
  B10001,
  B01110
};

byte light_intensity_symbol[] = { //selfmade symbol for the light intensity
  B00000,
  B01110,
  B10001,
  B10001,
  B10001,
  B01110,
  B01110,
  B00100
};

//Pin 6 = RST (reset)
//Pin 7 = DATA
//Pin 8 = CLK (clock)
DS1302 rtc(6, 7, 8); //global variable for the clock module, which comes from the library DS1302.h
//0x27 = address of the lcd
LiquidCrystal_I2C lcd(0x27, LCD_COLUMNS, LCD_ROWS); //global variable for the lcd, which comes from the library LiquidCrystal_I2C.h
dht11 DHT11; //global variable for the temperature sensor, which comes from the library dht11.h

//functions for measuring sensors
void check_soil_moisture_value();
void check_brightness_value();
void check_temperature_value();

//functions for menu handling
void handle_submenu();
void handle_menu();
void handle_buttons();

//function for rtc module
void init_rtc_module();

//functions for printing on the lcd
void print_lcd(char *text, int line, int column);
void clear_line(int line);
void print_information_menu();
void print_menu();

//functions others
char *get_day_of_week(Time t);

//function pointers printing submenu's
void print_submenu_functionalities(int index);
void print_lightning_info(void);
void print_lightning_settings(void);
void print_plants_info(void);
void print_plants_settings(void);
void print_temperature_info(void);
void print_temperature_settings(void);
void print_time_info(void);
void print_time_settings(void);

//function pointers for subfunctions soil moisture
void increase_water_value(void);
void decrease_water_value(void);
void switch_water_state(void);

//funtion pointers for subfuctions light intensity
void increase_brightness_value(void);
void decrease_brightness_value(void);
void switch_brightness_state(void);

//function pointers for subfunctions temperature
void increase_temperature_value(void);
void decrease_temperature_value(void);
void switch_temperature_state(void);

typedef struct{
  char function_name[10];
  void (*body)(void);
} STRUCT_SUBMENU;

typedef struct{
  unsigned int id;
  unsigned int new_id[2];
  char *main_text;
  char *sub_text;
  void (*on_entry)(void);
  STRUCT_SUBMENU functions[3];
} STRUCT_MENU;

STRUCT_MENU menu[] = {
  {
    MENU_LIGHTNING_INFO_ID, //id 1
    {
      MENU_PLANTS_INFO_ID, //Button A pressed: go to plants info
      MENU_LIGHTNING_SETTINGS_ID //Button B pressed: go to lightning setting
    },
    "Lightning",
    "Info",
    print_lightning_info, //function pointer for this menu
    {{"", NULL}, {"", NULL}, {"", NULL}} //lightning info doens't have subfunctions
  },
  {
    MENU_LIGHTNING_SETTINGS_ID, //id 2
    {
      MENU_PLANTS_INFO_ID, //Button A pressed: go to plants info
      MENU_LIGHTNING_INFO_ID //Button B pressed: go to lightning info
    },
    "Lightning",
    "Settings",
    print_lightning_settings, //function pointer for this menu
    {{"A:Bri+", increase_brightness_value}, {"B:Bri-", decrease_brightness_value}, {"C:ON/OFF", switch_brightness_state}}
  },
  {
    MENU_PLANTS_INFO_ID, //id 3
    {
      MENU_TEMPERATURE_INFO_ID, //Button A pressed: go to temperature info
      MENU_PLANTS_SETTINGS_ID //Button B pressed: go to plants settings
    },
    "Plants",
    "Info",
    print_plants_info, //function pointer for this menu
    {{"", NULL}, {"", NULL}, {"", NULL}} //plants info doens't have subfunctions
  },
  {
    MENU_PLANTS_SETTINGS_ID, //id 4
    {
      MENU_TEMPERATURE_INFO_ID, //Button A pressed: go to temperature info
      MENU_PLANTS_INFO_ID //Button B pressed: go to plants info
    },
    "Plants",
    "Settings",
    print_plants_settings, //function pointer for this menu
    {{"A:SM+", increase_water_value}, {"B:SM-", decrease_water_value}, {"C:On/Off", switch_water_state}}
  },
  {
    MENU_TEMPERATURE_INFO_ID, //id 5
    {
      MENU_TIME_INFO_ID, //Button A pressed: go to time info
      MENU_TEMPERATURE_SETTINGS_ID //Button B pressed: go to temperature settings
    },
    "Temperature",
    "Info",
    print_temperature_info, //function pointer for this menu
    {{"", NULL}, {"", NULL}, {"", NULL}} //temperature info doens't have subfunctions
  },
  {
    MENU_TEMPERATURE_SETTINGS_ID, //id 6
    {
      MENU_TIME_INFO_ID, //Button A pressed: go to time info
      MENU_TEMPERATURE_INFO_ID //Button B pressed: go to temperature info
    },
    "Temperature",
    "Settings",
    print_temperature_settings, //function pointer for this menu
    {{"A:Temp+", increase_temperature_value}, {"B:Temp-", decrease_temperature_value}, {"C:ON/OFF", switch_temperature_state}}
  },
  {
    MENU_TIME_INFO_ID, //id 7
    {
      MENU_LIGHTNING_INFO_ID, //Button A pressed: go to lightning info
      MENU_TIME_SETTINGS_ID //Button B pressed: go to time settings
    },
    "Time",
    "Info",
    print_time_info, //function pointer for this menu
    {{"", NULL}, {"", NULL}, {"", NULL}} //time info doens't have subfunctions
  },
  {
    MENU_TIME_SETTINGS_ID, //id 8
    {
      MENU_LIGHTNING_INFO_ID, //Button A pressed: go to lightning info
      MENU_TIME_INFO_ID //Button B pressed: go to time info
    },
    "Time",
    "Settings",
    print_time_settings, //function pointer for this menu
    {{"", NULL}, {"", NULL}, {"", NULL}} //time settings doesn't have subfunctions. Maybe later the time can be set...?
  }
};

/*
 * This code is the setup code, which will run once
 */
void setup() {
  Serial.begin(9600);
  pinMode(REMOTE_BUTTON_A, INPUT); //set pin D2 (remote_button_A) to input
  pinMode(REMOTE_BUTTON_B, INPUT); //set pin D3 (remote_button_B) to input
  pinMode(REMOTE_BUTTON_C, INPUT); //set pin D4 (remote_button_C) to input
  pinMode(REMOTE_BUTTON_D, INPUT); //set pin D5 (remote_button_D) to input

  pinMode(SOIL_MOISTURE_PIN, INPUT); //set pin A0 (SOIL_MOISTURE_PIN) to input
  pinMode(LDR_PIN, INPUT); //set pin A1 (LDR_PIN) to input
  pinMode(DHT11_PIN, INPUT); //set pin D9 (DHT11_PIN) to input

  pinMode(LED_SOIL_MOISTURE_PIN, OUTPUT); //set pin D10 (LED_SOIL_MOISTURE_PIN) to output
  pinMode(LED_LDR_PIN, OUTPUT); //set pin D11 (LED_LDR_PIN) to output
  pinMode(LED_DHT11_PIN, OUTPUT); //set pin D12 (LED_DHT11_PIN) to output

  digitalWrite(LED_SOIL_MOISTURE_PIN, LOW); //led which shows the state of the soil moisture is off at the beginning
  digitalWrite(LED_LDR_PIN, LOW); //led which shows the state of the light intensity is off at the beginning
  digitalWrite(LED_DHT11_PIN, LOW); //led which shows the state of the temperature is off at the beginning

  lcd.backlight(); //turn on the backlight of the LCD
  lcd.init(); //initialize the LCD
  lcd.begin(LCD_COLUMNS, LCD_ROWS); //set the size of the lcd: 20 X 4

  lcd.createChar(TEMPERATURE_SYMBOX_ID, temperature_symbol); //create temperature custom character of byte array
  lcd.createChar(SOIL_MOISTURE_SYMBOL_ID, soil_moisture_symbol); //create soil moisture custom character of byte array
  lcd.createChar(LIGHT_INTENSITY_SYMBOL_ID, light_intensity_symbol); //create light intensity custom character of byte array

  rtc.halt(false); //set the clock to run-mode
  rtc.writeProtect(false); //disable the write protection
  //init_rtc_module();

  check_soil_moisture_value(); //measure the soil moisture for the first time
  check_brightness_value(); //measure the light intensity for the first time
  check_temperature_value(); //measure the temperature for the first time
  print_information_menu(); //print the information menu
}

/*
 * This code will run repeatedly. It's a forever while loop
 */
void loop() {
  handle_buttons(); //handle the pressed buttons
  check_soil_moisture_value(); //measure the soil moisture and water the plants when needed.
  check_brightness_value(); //measure the light intensity and turn on the light when needed.
  check_temperature_value(); //measure the temperature and turn on the heater when needed.
  handle_submenu(); //handle the submenu's
  print_information_menu(); //update the information menu
  delay(250); //wait for 250 milliseconds
}

/*
 * this function handles the different submenu's: what should be displayed on the lcd
 */
void handle_submenu(){
  if(in_submenu){ //when inside a submenu
    char string_buffer[20]; //help buffer to temporarily store a string
    switch(current_menu_index + 1){
      case MENU_TIME_INFO_ID: //id 7
        print_lcd(rtc.getDateStr(), 1, 0); //get the current date and print it on the lcd
        print_lcd(rtc.getTimeStr(), 1, 11); //get the current time and print it on the lcd
        break;

      case MENU_PLANTS_INFO_ID: //id 3
        clear_line(1); //clear line 1
        sprintf(string_buffer, "Soil moisture: %d", current_water_value); //convert the soil moisture value into a string
        print_lcd(string_buffer, 1, 0); //print the soil moisture buffer on the lcd
        break;

      case MENU_PLANTS_SETTINGS_ID: //id 4
        clear_line(1); //clear line of the lcd
        sprintf(string_buffer, "SM > %d", water_check_value); //convert the soil moisture compare value into a string
        print_lcd(string_buffer, 1, 0); //print the compare value on the lcd
        print_lcd(water_check_state ? "ON" : "OFF", 1, 17); //print the state of the soil moisture settings on the lcd --> print ON when water_check_state is true, otherwise OFF
        break;
      
      case MENU_LIGHTNING_INFO_ID: //id 1
        clear_line(1); //clear line 1 of the lcd
        sprintf(string_buffer, "Brightness: %d", current_brightness_value); //convert the light intensity value into a string
        print_lcd(string_buffer, 1, 0); //print the light intensity buffer on the lcd
        break;

      case MENU_LIGHTNING_SETTINGS_ID: //id 2
        clear_line(1); //clear line 1
        sprintf(string_buffer, "Brightness < %d", brightness_check_value); //convert the light intensity compare value into a string
        print_lcd(string_buffer, 1, 0); //print the compare value on the lcd
        print_lcd(brightness_check_state ? "ON" : "OFF", 1, 17); //print the state of the lighting settings on the lcd --> print ON when brightness_check_state is true, otherwise OFF
        break;

      case MENU_TEMPERATURE_INFO_ID: //id 5
        //the temperature value of the DHT11 can't be converted to a string (using sprintf). Now do it in 3 steps.
        print_lcd("Temperature: ", 1, 0); //print the word 'temperature' on the lcd
        lcd.setCursor(13, 1); //move cursor to line 1, column 13
        lcd.print((float)DHT11.temperature); //print current temperature on the lcd

        //the humidity value of the DHT11 can't be converted to a string (using sprintf). Now do it in 3 steps.
        print_lcd("Humidity: ", 2, 0); //print the wordt 'Humidity' on the lcd
        lcd.setCursor(10, 2); //move cursor to line 2, column 10
        lcd.print((float)DHT11.humidity); //print current humidity on the lcd
        break;

      case MENU_TEMPERATURE_SETTINGS_ID: //id 6
        clear_line(1);//clear line 1 of the lcd
        sprintf(string_buffer, "Temperature < %d", temperature_check_value); //convert the temperature compare value into a string
        print_lcd(string_buffer, 1, 0); //print the compare value on the lcd
        print_lcd(temperature_check_state ? "ON" : "OFF", 1, 17); //print the state of the temperature settings on the lcd --> print ON when temperature_check_state is true, otherwise OFF
        break;
        
      default:
        break; 
    }
  }
}  

/*
 * This function measures the soil moisture.
 * When the measured value is more than the compare value, give the plant water 
 * (because i don't have the hardware to do this, just a LED will turn on which representes the waterseal or pump).
 */
void check_soil_moisture_value(){
  current_water_value = analogRead(SOIL_MOISTURE_PIN); //measrue the current soil moisture
  if(water_check_state){ 
    if(current_water_value > water_check_value){
      digitalWrite(LED_SOIL_MOISTURE_PIN, HIGH); //turn on the LED
    } else{
      digitalWrite(LED_SOIL_MOISTURE_PIN, LOW); //turn off the LED
    }
  } else {
    digitalWrite(LED_SOIL_MOISTURE_PIN, LOW); //turn off the LED
  }
}

/*
 * This function measures the light intensity.
 * When the measured value is less than the compare value, turn on the lights 
 * (because i don't have the hardware to do this, just a LED will turn on which represents all lights)
 */
void check_brightness_value(){
  current_brightness_value = analogRead(LDR_PIN); //measrue the light intensity
  if(brightness_check_state){ 
    if(current_brightness_value < brightness_check_value){
      digitalWrite(LED_LDR_PIN, HIGH); //turn on the LED
    } else{
      digitalWrite(LED_LDR_PIN, LOW); //turn off the LED
    }
  } else {
    digitalWrite(LED_LDR_PIN, LOW); //turn off the LED
  }
}

/*
 * This function measures the temperature.
 * When the measured value is less than the compare value, turn on the heater
 * (because i don't have the hardware to do this, just a LED will turn on which represents the heater)
 */
void check_temperature_value(){
  DHT11.read(DHT11_PIN); //read the temperature and humidity
  current_temperature_value = (float)DHT11.temperature; //store the current temperature
  if(temperature_check_state){
    if(current_temperature_value < temperature_check_value){
      digitalWrite(LED_DHT11_PIN, HIGH); //turn on the LED
    } else{
      digitalWrite(LED_DHT11_PIN, LOW); //turn off the LED
    }
  } else {
    digitalWrite(LED_DHT11_PIN, LOW); //turn off the LED
  }
}

/*
 * function which is called when a submenu is entered
 */
void enter_function(){
  if(!in_submenu){
    in_submenu = true; //switch the state of the program to true
    (*menu[current_menu_index].on_entry)(); //call the function pointer of the selected menu
  }
}

/*
 * this function is called when a submenu is leaved
 */
void exit_function(){
  if(in_submenu){
    in_submenu = false; //switch the state of the program to false
    print_menu(); //print the menu
  }
}

/*
 * This function handles the menu navigation
 * int button = buttons A, B, C or D of the remote control.
 */
void handle_menu(int button){
  bool button_pressed = false;
  switch(button){
    case REMOTE_BUTTON_A: //button A is pressed
        if(in_submenu){ //when in a submenu
          if(menu[current_menu_index].functions[REMOTE_BUTTON_A - 2].body != NULL){ //function pointer of function A isn't equal to NULL
              (menu[current_menu_index].functions[REMOTE_BUTTON_A - 2].body)(); //call the function pointer of function A
          }
        } else {
          current_menu_id = menu[current_menu_index].new_id[REMOTE_BUTTON_A - 2]; //go to the next menu item
          button_pressed = true;
        }
        break;
        
    case REMOTE_BUTTON_B: //button B is pressed
        if(in_submenu){ //when in a submenu
          if(menu[current_menu_index].functions[REMOTE_BUTTON_B - 2].body != NULL){ //function pointer of function B isn't equal to NULL
              (menu[current_menu_index].functions[REMOTE_BUTTON_B - 2].body)(); //call the function pointer of function B
          }        
        } else {
          current_menu_id = menu[current_menu_index].new_id[REMOTE_BUTTON_B - 2]; //go to the next submenu item
          button_pressed = true;
        }
        break;
        
    case REMOTE_BUTTON_C: //button C is pressed
        if(in_infomenu){
          in_infomenu = false;
          print_menu(); //leave the info menu and go to the main menu
        } else if(in_submenu){ //when in a submenu
          if(menu[current_menu_index].functions[REMOTE_BUTTON_C - 2].body != NULL){ //function pointer of function C isn't equal to NULL
              (menu[current_menu_index].functions[REMOTE_BUTTON_C - 2].body)(); //call the function pointer of function C
          }        
        } else {
          enter_function(); //enter the selected submenu
        }
        break;
        
    case REMOTE_BUTTON_D: //button D is pressed
        if(in_submenu){
          exit_function(); //exit the selected submenu
        } else{ //when in the main menu
          lcd.clear();
          in_infomenu = true; //go to the info menu
        }
        
        break;
        
    default: //no button is pressed
        button_pressed = false;
        break;
  }

  if(button_pressed){ 
    current_menu_index = 0;
    while(menu[current_menu_index].id != current_menu_id){ //check the selected menu item
      current_menu_index += 1;
    }
    print_menu(); //print the selected menu item
  }
}

/*
 * This function checks which button is pressed. Just one button can be pressed at the same time
 */
void handle_buttons(){
  int states = 0; 
  if(digitalRead(REMOTE_BUTTON_A) == 1){ //check if button A is pressed
    states = REMOTE_BUTTON_A; //if button A is pressed, states is set to 2 (pinnumber of button A)
  } else if (digitalRead(REMOTE_BUTTON_B) == 1){ //check if button B is pressed
    states = REMOTE_BUTTON_B; //if button B is pressed, states is set to 3 (pinnumber of button B)
  } else if (digitalRead(REMOTE_BUTTON_C) == 1){ //check if button C is pressed
    states = REMOTE_BUTTON_C; //if button C is pressed, states is set to 4 (pinnumber of button C)
  } else if (digitalRead(REMOTE_BUTTON_D) == 1){ //check if button D is pressed
    states = REMOTE_BUTTON_D; //if button D is pressed, states is set to 5 (pinnumber of button D)
  } else { //no button is pressed
    states = 0;
  }

  handle_menu(states); //handle the menu navigation with the pressed button
}

/*
 * this function initializes the rtc (realtime clock) module.
 * The code only needs to be uploaded once. When uploaded, the clock module updates the time itself. The current time is initialized at Saturday 03-04-2021 21.51.
 */
void init_rtc_module(){
  rtc.setDOW(SATURDAY); //set Day-of-week to Saturday
  rtc.setTime(21, 51, 0); // Set the time to 19:39:00 (24hr format)
  rtc.setDate(03, 04, 2021); // Set the date to april 3rd, 2021
}

/*
 * This function prints the whole information menu.
 * The following information is printed:
 * - Current day of week (dow), date and time
 * - Temperature: current value, compare value and state
 * - Soil moisture: current value, compare value and state
 * - Light intensity: current value, compare value and state
 */
void print_information_menu(){
  if(in_infomenu){
    lcd.clear(); //clear the whole lcd
    // ----- Current dow, date and time -----
    Time t = rtc.getTime();
    char time_buffer[20];
    sprintf(time_buffer, "%s %02d-%02d-%d %02d:%02d", get_day_of_week(t), t.date, t.mon, t.year, t.hour, t.min); //convert time information in stringformat 'dow dd-mm-yy hh:mm'
    print_lcd(time_buffer, 0, 0); //print the time information at line 0 of the lcd

    // ---------- temperature --> current value ----------
    lcd.setCursor(1,1); //move cursor to line 1, column 0
    lcd.write(TEMPERATURE_SYMBOX_ID); //print the temperature symbol
    lcd.setCursor(3,1); //move cursor to line 1, column 2
    lcd.print(current_temperature_value); //print the current temperature
    lcd.print((char)223); //print the degree symbol
    lcd.print("C"); //print the Celsius symbol

    //---------- temperature --> compare value ----------
    lcd.setCursor(10,1);//move cursor to line 1, column 10
    lcd.print(temperature_check_value); //print the temperature compare value
    lcd.print((char)223); //print the degree symbol
    lcd.print("C"); //print the Celsius symbol

    //---------- temperature --> check state ----------
    lcd.setCursor(17, 1); //move cursor to line 1, column 17
    lcd.print(temperature_check_state ? "X" : "-"); //if temperature_check_state = true: print 'X', otherwise '-'

    // ---------- soil moisture --> current value ----------
    lcd.setCursor(1,2); //move cursor to line 2, column 0
    lcd.write(SOIL_MOISTURE_SYMBOL_ID); //print the soil moisture symbol
    lcd.setCursor(3,2); //move cursor to line 2, column 2
    lcd.print(current_water_value); //print the current soil moisture

    // ---------- soil moisture --> compare value ----------
    lcd.setCursor(10,2); //move cursor to line 2, column 10
    lcd.print(water_check_value); //print the soil moisture compare value

    // ---------- soil moisture --> check state ----------
    lcd.setCursor(17, 2); //move cursor to line 2, column 17
    lcd.print(water_check_state ? "X" : "-"); //if water_check_state = true: print 'X', otherwise '-'

    // ---------- light intensity --> current value ----------
    lcd.setCursor(1,3); //move cursor to line 3, column 0
    lcd.write(LIGHT_INTENSITY_SYMBOL_ID); //print the light intensity symbol
    lcd.setCursor(3,3); //move cursor to line 3, column 2
    lcd.print(current_brightness_value); //print the light intensity

    // ---------- light intensity --> compare value ----------
    lcd.setCursor(10,3); //move cursor to line 3, column 10
    lcd.print(brightness_check_value); //print the light intensity compare value

    // ---------- light intensity --> check state ----------
    lcd.setCursor(17, 3); //move cursor to line 3, column 17
    lcd.print(brightness_check_state ? "X" : "-"); //if brightness_check_state = true: print 'X', otherwise '-'
  }
}

/*
 * This function returns the day of the week in a three character string
 * Time t = the current time
 */
char *get_day_of_week(Time t){
  char* day_char;
  switch(t.dow){ 
    case MONDAY:
      day_char = "MON";
      break;
    case TUESDAY:
      day_char = "TUE";
      break;
    case WEDNESDAY:
      day_char = "WED";
      break;
    case THURSDAY:
      day_char = "THU";
      break;
    case FRIDAY:
      day_char = "FRI";
      break;
    case SATURDAY:
      day_char = "SAT";
      break;
    case SUNDAY:
      day_char = "SUN";
      break;
  }
  return day_char;
}

/*
 * This funciton prints the menu when not inside a submenu
 */
void print_menu(){
  lcd.clear(); //clear the whole lcd
  
  char string_buffer[20]; //help buffer to store a string
  size_t string_length = strlen(menu[current_menu_index].main_text); //calculate the length of the text of the selected mainmenu item
  sprintf(string_buffer, "< %s >", menu[current_menu_index].main_text); //store the text of the mainmenu item in the buffer and add the < and >
  print_lcd(string_buffer, 1, 10 - (string_length / 2) - 2); //print the mainmenu text in the middle of line 1 of the lcd

  print_lcd("--------------------", 2, 0); //print a dividing line at line 2

  string_length = strlen(menu[current_menu_index].sub_text); //calculate the length of the text of the selected submenu item
  sprintf(string_buffer, "< %s >", menu[current_menu_index].sub_text); //store the text of the submenu item in the buffer and add the < and >
  print_lcd(string_buffer, 3, 10 - (string_length / 2) - 2); //print the submenu text in the middle of line 3 of the lcd
}

/*
 * This function prints a given string at a given position
 * char *text = the string to be displayed
 * int line = on which line the string has to be displayed
 * int column = on which digit the string has to start
 */
void print_lcd(char *text, int line, int column){
  lcd.setCursor(column, line); //set the cursor to the given line and digit
  lcd.print(text); //print the given text
}

/*
 * This function clears a given line in an ugly way
 * int line = the line to be cleared
 */
void clear_line(int line){
  print_lcd("                    ", line, 0);
}

/*
 * This function prints the text of all submenu functionalities
 * int index = the index of the submenu in the menu list
 */
void print_submenu_functionalities(int index){
  print_lcd(menu[index].functions[0].function_name, 2, 0); //print function A at line 2, column 0
  size_t string_length = strlen(menu[index].functions[1].function_name); //calculate the length of function B
  print_lcd(menu[index].functions[1].function_name, 2, 20 - string_length); //print function B at the end of line 2
  print_lcd(menu[index].functions[2].function_name, 3, 0); //print function C at line 3, column 0
}

/*
 * Function pointer for the lightning info submenu (menu id 1)
 */
void print_lightning_info(void){
  lcd.clear(); //clear the whole display
  char *title = "lightning info"; //submenu title
  size_t string_length = strlen(title); //calculate the length of the submenu title
  print_lcd(title, 0, 10 - (string_length / 2)); //print the submenu title in the middle of line 0

  print_submenu_functionalities(current_menu_index); //print the submenu functionalities
}

/*
 * Function pointer for the lightning settings submenu (menu id 2)
 */
void print_lightning_settings(void){
  lcd.clear(); //clear the whole display
  char *title = "Lightning settings"; //submenu title
  size_t string_length = strlen(title); //calculate the length of the submenu title
  print_lcd(title, 0, 10 - (string_length / 2)); //print the submenu title in the middle of line 0

  print_submenu_functionalities(current_menu_index); //print the submenu functionalities
}

/*
 * Function pointer for the plants info submenu (menu id 3)
 */
void print_plants_info(void){
  lcd.clear(); //clear the whole display
  char *title = "plants info"; //submenu title
  size_t string_length = strlen(title); //calculate the length of the submenu title
  print_lcd(title, 0, 10 - (string_length / 2)); //print the submenu title in the middle of line 0

  print_submenu_functionalities(current_menu_index); //print the submenu functionalities
}

/*
 * Function pointer for the plants settings submenu (menu id 4)
 */
void print_plants_settings(void){
  lcd.clear(); //clear the whole display
  char *title = "plants settings"; //submenu title
  size_t string_length = strlen(title); //calculate the length of the submenu title
  print_lcd(title, 0, 10 - (string_length / 2)); //print the submenu title in the middle of line 0

  print_submenu_functionalities(current_menu_index); //print the submenu functionalities
}

/*
 * Function pointer for the temperature info submenu (menu id 5)
 */
void print_temperature_info(void){
  lcd.clear(); //clear the whole display
  char *title = "temperature info"; //submenu title
  size_t string_length = strlen(title); //calculate the length of the submenu title
  print_lcd(title, 0, 10 - (string_length / 2)); //print the submenu title in the middle of line 0

  print_submenu_functionalities(current_menu_index); //print the submenu functionalities
}

/*
 * Function pointer for the temperature settings submenu (menu id 6)
 */
void print_temperature_settings(void){
  lcd.clear(); //clear the whole display
  char *title = "temperature settings"; //submenu title
  size_t string_length = strlen(title); //calculate the length of the submenu title
  print_lcd(title, 0, 10 - (string_length / 2)); //print the submenu title in the middle of line 0

  print_submenu_functionalities(current_menu_index); //print the submenu functionalities
}

/*
 * Function pointer for the time info submenu (menu id 7)
 */
void print_time_info(void){
  lcd.clear(); //clear the whole display
  char *title = "time info"; //submenu title
  size_t string_length = strlen(title); //calculate the length of the submenu title
  print_lcd(title, 0, 10 - (string_length / 2)); //print the submenu title in the middle of line 0

  print_submenu_functionalities(current_menu_index); //print the submenu functionalities
}

/*
 * Function pointer for the time settings submenu (menu id 8)
 */
void print_time_settings(void){
  lcd.clear(); //clear the whole display
  char *title = "time settings"; //submenu title
  size_t string_length = strlen(title); //calculate the length of the submenu title
  print_lcd(title, 0, 10 - (string_length / 2)); //print the submenu title in the middle of line 0

  print_submenu_functionalities(current_menu_index); //print the submenu functionalities
}

/*
 * Funcion pointer which increases the compare value of the soil moisture.
 * NOTE: compare value can't be bigger than 1024
 */
void increase_water_value(void){
  water_check_value++;
  if (water_check_value > 1024){
    water_check_value = 0;
  }
}

/*
 * Funcion pointer which decreases the compare value of the soil moisture.
 * NOTE: compare value can't be less than 0
 */
void decrease_water_value(void){
  water_check_value--;
  if (water_check_value < 0){
    water_check_value = 1024;
  }
}

/*
 * Funcion pointer which changes the state of the soil moisture settings:
 * - When boolean is true: plants will be watered automatically when compare value is reached
 * - When boolean is false: plants won't be watered automatically when compare value is reached
 */
void switch_water_state(void){
  water_check_state = !water_check_state; //toggle the boolean
}

/*
 * Funcion pointer which increases the compare value of the light intensity.
 * NOTE: compare value can't be bigger than 1024
 */
void increase_brightness_value(void){
  brightness_check_value++;
  if(brightness_check_value > 1024){
    brightness_check_value = 0;
  }
}

/*
 * Funcion pointer which decreases the compare value of the soil moisture.
 * NOTE: compare value can't be less than 0
 */
void decrease_brightness_value(void){
  brightness_check_value--;
  if(brightness_check_value < 0){
    brightness_check_value = 1024;
  }
}

/*
 * Funcion pointer which changes the state of the light intensity settings:
 * - When boolean is true: lights will turn on automatically when compare value is reached
 * - When boolean is false: lights won't turn on automatically when compare value is reached
 */
void switch_brightness_state(void){
  brightness_check_state = !brightness_check_state; //toggle the boolean
}

/*
 * Funcion pointer which increases the compare value of the temperature.
 * NOTE: compare value can't be bigger than 50
 */
void increase_temperature_value(void){
  temperature_check_value++;
  if(temperature_check_value > 50){
    temperature_check_value = -20;
  }
}

/*
 * Funcion pointer which decreases the compare value of the temperature.
 * NOTE: compare value can't be less than -20
 */
void decrease_temperature_value(void){
  temperature_check_value--;
  if(temperature_check_value < -20){
    temperature_check_value = 50;
  }
}

/*
 * Funcion pointer which changes the state of the temperature settings:
 * - When boolean is true: heater will turn on automatically when compare value is reached
 * - When boolean is false: heater won't turn on automatically when compare value is reached
 */
void switch_temperature_state(void){
  temperature_check_state = !temperature_check_state; //toggle the boolean
}
