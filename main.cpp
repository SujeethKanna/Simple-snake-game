#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <vector>

#define DC 4
#define CS 5
#define up 33
#define down 26
#define left 27
#define right 25

const int screen_height = 240;
const int screen_width = 320;
const int game_height = 220;
const int game_width = 320;


struct Point {
  int x;
  int y;
};

// A vector (better for dynamic allocation) to contain all the coordinates of the parts of the snake.
std::vector<Point> snake; 

Adafruit_ILI9341 tft = Adafruit_ILI9341(CS, DC);

Point food_spot = {0, 0};
int score = 0;
int snake_size = 1;
int food_size = 8;
int seg_size = 8; 
int speed_delay = 100;

// Direction:1=Up, 2=Down, 3=Left, 4=Right
int current_direction = 0;
int previous_direction = 0;

void spawn_food() {
  // Ensuring that the food spwans within the grid 
  int cols = screen_width / seg_size;
  int rows = (screen_height - 20)/ seg_size; //giving a header space at the top to not interfere with the score box.
  
  int x = (esp_random() % (cols - 2) + 1) * seg_size;
  int y = (esp_random() % (rows - 2) + 1) * seg_size;

  //Check to ensure food doesn't spawn on head
  while (!snake.empty() && x == snake[0].x && y == snake[0].y) {
    x = (esp_random() % (cols - 2) + 1) * seg_size;
    y = (esp_random() % (rows - 2) + 1) * seg_size;
  }
  
  food_spot = {x, y};
  tft.fillCircle(food_spot.x, food_spot.y, food_size, ILI9341_RED);
}

void reset_game() {
  score = 0;
  current_direction = 0;
  previous_direction = 0;
  
  tft.fillRect(0,0,screen_width,screen_height, ILI9341_BLACK);

  tft.fillRect(0,0,screen_width,screen_height, ILI9341_BLACK);
  
  snake.clear();
  // Start in middle
  snake.push_back({screen_width/2, screen_height/2});
  
  // Draw head
  tft.fillRect(snake[0].x, snake[0].y, seg_size, seg_size, ILI9341_GREEN);
  
  spawn_food();
  
  // Draw Score
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print("SCORE: ");
  tft.print(score);
}

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(3);
  pinMode(up, INPUT_PULLDOWN);
  pinMode(down, INPUT_PULLDOWN);
  pinMode(left, INPUT_PULLDOWN);
  pinMode(right, INPUT_PULLDOWN);
  reset_game();
}

void loop() {
  // 1. Input Handling and implementing the constraint of direction change.
  if (digitalRead(up) == HIGH && previous_direction != 2)    current_direction = 1;
  else if (digitalRead(down) == HIGH && previous_direction != 1)   current_direction = 2;
  else if (digitalRead(left) == HIGH && previous_direction != 4)   current_direction = 3;
  else if (digitalRead(right) == HIGH && previous_direction != 3)  current_direction = 4;

  if (current_direction == 0) {
    delay(50);
    return;
  }

  // 2. Calculating New Head Position.
  int newX = snake[0].x;
  int newY = snake[0].y;
  if (current_direction == 1) newY -= seg_size; // Up
  if (current_direction == 2) newY += seg_size; // Down
  if (current_direction == 3) newX -= seg_size; // Left
  if (current_direction == 4) newX += seg_size; // Right

  // 3. Wall Collision Check.
  if (newX < 0 || newX >= screen_width || newY < 0 || newY >= screen_height) {
    reset_game();
    return;
  }

  // 4. Self Collision Check (Loop through body to see if we hit ourself).
  for (int i = 0; i < snake.size(); i++) {
    if (newX == snake[i].x && newY == snake[i].y) {
       reset_game();
       return;
    }
  }

  // 5. Move Logic: Add new head.
  snake.insert(snake.begin(), {newX, newY});
  
  // Draw new head
  tft.fillRect(newX, newY, seg_size, seg_size, ILI9341_GREEN);

  // 6. Food Check
  if (abs(newX - food_spot.x) < food_size + 5 && abs(newY - food_spot.y) < food_size + 5)
    // Adding a small flexibility to the collision window to detect collisions better.
   {
    score += 5;
    
    // Updating Score Text
    tft.fillRect(0, 0, 140, 20, ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.print("SCORE: ");
    tft.print(score);
    
    // Removing old food and spawning a new one.
    tft.fillCircle(food_spot.x, food_spot.y, food_size, ILI9341_BLACK);
    spawn_food();
    
  } else {
    // If the snake did not eat the food, we must remove the tail in order to maintain size.
    Point tail = snake.back();
    tft.fillRect(tail.x, tail.y, seg_size, seg_size, ILI9341_BLACK); // Erase tail from screen
    snake.pop_back(); // Remove tail from memory
  }

  previous_direction = current_direction;
  delay(speed_delay);
}
