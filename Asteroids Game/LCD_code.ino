#include <SPI.h>
#include "LCD_Driver.h"
#include "GUI_Paint.h"
#include "image.h"

#define pi 3.14159
#define two_pi 6.28318
// player setup
const uint16_t player_length = 5;
const uint16_t player_width = 20;
const uint16_t player_colour = 0xFFFF;
const float  accel = 5;
const float max_speed = 8;
const uint16_t weight = 4;
float angle_accel_radians = pi/8;
int lives = 2;

float posx;
float posy;
float velx;
float vely;
float angle;

float point1x;
float point1y;

float point2x;
float point2y;

float point3x;
float point3y;

// Bullet Setup
int current_bullet = 0;
int bullet_max_amount = 10;
float bullets_x[10];
float bullets_y[10];
float bullets_angle[10]; 
bool bullets_active[10];
const float bullet_speed = 10;

// Meteors Setup

int current_meteor = 0;
int meteor_max_amount = 15;
int meteor_speed_ramp = 0.01;
int meteor_max_speed = 9;
int meteor_min_speed = 5;
int meteor_min_size = 7;
int meteor_max_size = 30;
float meteors_x[15];
float meteors_y[15];
float meteors_angle[15];
float meteors_speed[15];
int meteors_size[15];
bool meteors_active[15];

// Controls Setup
const uint16_t deadzone = 100;
float upper_deadzone;
float deadzone_upper_mult;
float deadzone_lower_mult;
float lower_deadzone;

// Misc. Setup
unsigned long round_start_time;
unsigned long start_time;
unsigned long shot_last;
unsigned long meteor_last;;
int current_update_cycle = 0;
float spawntime = 600;
float prev_round = 0;

void setup()
{
  Config_Init();
  LCD_Init();
  LCD_Clear(0x0000);
  Paint_NewImage(LCD_WIDTH, LCD_HEIGHT, 0, 0x0000);
  initialize_player();

  for (int i = 0; i < bullet_max_amount; i++){
    bullets_x[i] = 100;
    bullets_y[i] = 100;
    bullets_angle[i] = -100;
    bullets_active[i] = false;
  }


  meteor_last = millis();
  start_time = millis();
  shot_last = millis();
  round_start_time = millis();
  prev_round = round_start_time;
  
  Serial.begin(9600);
  
}
void loop()
{
  // time-based actions
  if (millis() - start_time > 33){
    check_joystick();
    clear_old_player();
    update_player();
    display_player();
    display_bullets();
    check_collision();
    
    if (current_update_cycle == 2){
    
      create_meteors();
      display_meteors();
      current_update_cycle = 0;
    }
    current_update_cycle++;
    start_time = millis();
    
  }
  
}

void initialize_player(){
  posx = 180;
  posy = 120;
  velx = 0;
  vely = 0;
  angle = pi/2;

  point1x = posx + 0.5*player_width*cos(angle);
  point1y = posy + 0.5*player_width*sin(angle);
  
  point2x = posx - 0.5*player_width*cos(angle) + player_length*cos(angle+pi/2);
  point2y = posy - 0.5*player_width*sin(angle) +player_length*sin(angle+pi/2);

  point3x = posx - player_length*cos(angle+pi/2) - 0.5*player_width*cos(angle);
  point3y = posy - player_length*sin(angle+pi/2) - 0.5*player_width*sin(angle);
  upper_deadzone = 512+deadzone;
  lower_deadzone = 512-deadzone;
  deadzone_upper_mult = 1/(1023 - upper_deadzone);
  deadzone_lower_mult = 1/(lower_deadzone);

  draw_lives();
  
}

void draw_lives(){

  if (lives == 2){
  Paint_DrawLine(8,12 , 5, 5, 0xFFFF, DOT_PIXEL_1X1);
  Paint_DrawLine(2, 12, 5, 5, 0xFFFF, DOT_PIXEL_1X1);
  Paint_DrawLine(8, 12, 2, 12, 0xFFFF, DOT_PIXEL_1X1);

  Paint_DrawLine(16,12 , 13, 5, 0xFFFF, DOT_PIXEL_1X1);
  Paint_DrawLine(10, 12, 13, 5, 0xFFFF, DOT_PIXEL_1X1);
  Paint_DrawLine(16, 12, 10, 12, 0xFFFF, DOT_PIXEL_1X1);
  }

  if (lives == 1){
    Paint_DrawLine(8,12 , 5, 5, 0xFFFF, DOT_PIXEL_1X1);
    Paint_DrawLine(2, 12, 5, 5, 0xFFFF, DOT_PIXEL_1X1);
    Paint_DrawLine(8, 12, 2, 12, 0xFFFF, DOT_PIXEL_1X1);
  }
}

void check_joystick(){
  
  float joy_x = analogRead(0);
  if (joy_x > upper_deadzone){
  angle += (joy_x-512)*deadzone_lower_mult*angle_accel_radians;
 
  }
 else if (joy_x < lower_deadzone){
    angle -= (512-joy_x)*deadzone_lower_mult*angle_accel_radians;
 }
  float joy_y = analogRead(1);
 if (joy_y < lower_deadzone){
    float lin_accel = (512-joy_y)*deadzone_lower_mult*accel;
    velx += lin_accel*cos(angle);
    vely += lin_accel*sin(angle);
    if (velx*velx + vely*vely > max_speed*max_speed){
      float speed_red = max_speed*max_speed / (velx*velx + vely*vely);
      velx = velx*speed_red;
      vely = vely*speed_red;
    }
   
 }

 else if (joy_y > upper_deadzone){
  if (millis() - shot_last > 200){
   shoot();
   shot_last = millis();
  }
 }
}

void update_player(){
  posx += velx;
  posy += vely;
  velx -= velx/weight;
  vely -= vely/weight;

   if (posx > 300){
      posx = 300;
   }
   if (posx < 20){
      posx = 20;
   }
   if (posy > 220){
      posy = 220;
   }
   if (posy < 20){
      posy = 20;
   }
}

void check_collision(){
  for (int i = 0; i < meteor_max_amount; i++){
    if (meteors_active[i]){
     for (int j = 0; j < bullet_max_amount; j++){
      if (bullets_active[j]){
        if ((meteors_x[i] - bullets_x[j])*(meteors_x[i] - bullets_x[j]) + (meteors_y[i] - bullets_y[j])*(meteors_y[i] - bullets_y[j]) < meteors_size[i]*meteors_size[i] + 9){
          Paint_DrawCircle((int)meteors_x[i],(int)meteors_y[i],  meteors_size[i], 0x0000, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
          Paint_DrawPoint((int)bullets_x[j],(int)bullets_y[j], 0x0000, DOT_PIXEL_DFT, DOT_STYLE_DFT);
          meteors_active[i] = false;
          bullets_active[j] = false;
        }
      }
     }
     float px = posx;
     float py = posy;
     if ((meteors_x[i] - px)*(meteors_x[i] - px) + (meteors_y[i] - py)*(meteors_y[i] - py) < meteors_size[i]*meteors_size[i] + 9){   
      restart();
     }
    }
  }
}

void restart(){
   for (int i = 0; i < bullet_max_amount; i++){
    bullets_x[i] = 100;
    bullets_y[i] = 100;
    bullets_angle[i] = -100;
    bullets_active[i] = false;
  }

   for (int i = 0; i < meteor_max_amount; i++){
    meteors_active[i] = false;
  }

  if (lives > 0){
    lives--;
    Paint_Clear(0x0000);
    draw_lives();
    
  }
  else{
  Paint_Clear(0xF800);
  round_start_time = millis();
  initialize_player();
  Paint_Clear(0x0000);
  lives = 2;
  draw_lives();
  }
  
  
}

void create_meteors(){
 
 
  if (millis() - meteor_last > spawntime){
    
     if (200 - (millis() - round_start_time)*0.01 < 75){
        spawntime = 75;
     }
     else{
       spawntime = 200 - (millis() - round_start_time)*0.01;
     }
     int index = -1;

     
     for (int i = 0; i < meteor_max_amount; i++){
       if (meteors_active[i] == false){ index = i; } 
     }
     long rnd_num = random(0,4);
     if (index != -1)
      meteors_size[index] = random(meteor_min_size, meteor_max_size);{

     // 0 - 0.25 is X rand & Y 0, 0.25 - 0.5 is X 0 && Y rand, 0.5 - 0.75 is X rand & Y 240, 0.75 - 1 is X 320 & Y rand
     if(rnd_num >= 3){ meteors_x[index] = 320 + meteors_size[index]; meteors_y[index] = random(0,240); meteors_angle[index] = random(pi*1.5, 2.5*pi);}
     if(rnd_num >= 2 && rnd_num < 3){ meteors_x[index] = random(0,360); meteors_y[index] = 240 + meteors_size[index]; meteors_angle[index] = random(pi, 2*pi);}
     if(rnd_num >= 1 && rnd_num < 2){ meteors_x[index] = -meteors_size[index]; meteors_y[index] = random(0,240); meteors_angle[index] = random(0.5*pi, 1.5*pi); }
     if(rnd_num < 1){ meteors_x[index] = random(0,360); meteors_y[index] = - meteors_size[index]; meteors_angle[index] = random(0, pi);}

     
     meteors_speed[index] = random(meteor_min_speed + 3*(millis()- round_start_time)*0.0001, meteor_max_speed + 3*(millis()- round_start_time)*0.0001);
     meteors_active[index] = true;
    
     }
     meteor_last = millis();
  
  }
}

void display_meteors(){
  for (int i = 0; i < meteor_max_amount; i++){
    if (meteors_active[i] == true){
       Paint_DrawCircle((int)meteors_x[i],(int)meteors_y[i],  meteors_size[i], 0x0000, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
       meteors_x[i] += meteors_speed[i]*cos(meteors_angle[i]);
       meteors_y[i] += meteors_speed[i]*sin(meteors_angle[i]);
   

       if (meteors_x[i] < 320 + meteors_size[i] && meteors_y[i] < 240 + meteors_size[i] && meteors_x[i] > -meteors_size[i]  && meteors_y[i] > -meteors_size[i]){
          Paint_DrawCircle((int)meteors_x[i],(int)meteors_y[i],  meteors_size[i], 0xFFFF, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
       }
       else{
        meteors_active[i] = false;
       }
       
    }
   
  }
}


void clear_old_player(){
  Paint_DrawLine(point2x, point2y, point1x, point1y, 0x0000, DOT_PIXEL_1X1);
  Paint_DrawLine(point3x, point3y, point1x, point1y, 0x0000, DOT_PIXEL_1X1);
  Paint_DrawLine(point2x, point2y, point3x, point3y, 0x0000, DOT_PIXEL_1X1);
}
void display_player(){
  // finding the points of the triangle
  point1x = posx + 0.5*player_width*cos(angle);
  point1y = posy + 0.5*player_width*sin(angle);
  
  point2x = posx - 0.5*player_width*cos(angle) + player_length*cos(angle+pi/2);
  point2y = posy - 0.5*player_width*sin(angle) + player_length*sin(angle+pi/2);

  point3x = posx - player_length*cos(angle+pi/2) - 0.5*player_width*cos(angle);
  point3y = posy - player_length*sin(angle+pi/2) - 0.5*player_width*sin(angle);

  Paint_DrawLine(point2x, point2y, point1x, point1y, player_colour, DOT_PIXEL_1X1);
  Paint_DrawLine(point3x, point3y, point1x, point1y, player_colour, DOT_PIXEL_1X1);
  Paint_DrawLine(point2x, point2y, point3x, point3y, player_colour, DOT_PIXEL_1X1);
}

void shoot(){
   int px = posx;
   int py = posy;
   bullets_x[current_bullet] = px + 0.5*player_width*cos(angle);
   bullets_y[current_bullet] = py + 0.5*player_width*sin(angle); 
   bullets_angle[current_bullet] = angle;
   bullets_active[current_bullet] = true;
   current_bullet++;
   if (current_bullet > bullet_max_amount){
     current_bullet = 0;
   }
}

void display_bullets(){
  
  for (int i = 0; i < bullet_max_amount; i++){
    if (bullets_active[i] == true){
       Paint_DrawPoint((int)bullets_x[i],(int)bullets_y[i], 0x0000, DOT_PIXEL_DFT, DOT_STYLE_DFT);
       bullets_x[i] += bullet_speed*cos(bullets_angle[i]);
       bullets_y[i] += bullet_speed*sin(bullets_angle[i]);
       if (bullets_x[i] > 0 && bullets_y[i] > 0 && bullets_x[i] < 320 && bullets_y[i] < 320){
       Paint_DrawPoint((int)bullets_x[i],(int)bullets_y[i], player_colour, DOT_PIXEL_DFT, DOT_STYLE_DFT);
       }
       else {
          bullets_active[i] = false;
       }
    }
    
  }
}




/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
