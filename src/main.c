#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <raylib.h>
#include <raymath.h>

const int windowWidth = 1200;
const int windowHeight = 800;
int marginSize, innerWidth, innerHeight;
Vector2 marginStart, marginEnd;
float marginTurnFactor = 1.0f;

typedef struct Boid {
  Vector2 pos;
  Vector2 velocity;
} Boid;

size_t length = 40;
Boid *boids;
// Really isn't that much memory
Boid *visibleBoids;

float speedLimitMin = 1.0f;
float speedLimitMax = 10.0f;
float vision = 75.0f;
float centeringFactor = 0.005f;
float avoidDistance = 20.0f;
float avoidFactor = 0.05f;
float matchFactor = 0.05f;

static inline float randf(void) { return (float)rand() / (float)RAND_MAX; }

void setup(void);
void setMargin(int size);
void randomize(void);
void update(void);
void flyTowardsCenter(Boid *boid, size_t visible);
void avoid(Boid *boid);
void matchVelocity(Boid *boid, size_t visible);
void checkMargins(Boid *boid);
void checkSpeed(Boid *boid);
void draw(void);

int main(void) {
  setup();
  while (!WindowShouldClose()) {
    update();
    draw();
  }
  return 0;
}

void setup(void) {
  boids = (Boid *)malloc(sizeof(Boid) * length);
  visibleBoids = (Boid *)malloc(sizeof(Boid) * length);
  setMargin(200);

  SetConfigFlags(FLAG_MSAA_4X_HINT);
  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(windowWidth, windowHeight, "boids");

  srand(time(NULL));
  randomize();
}

void setMargin(int size) {
  marginSize = size;
  marginStart = (Vector2){size, size};
  marginEnd = (Vector2){windowWidth - size, windowHeight - size};
  innerWidth = marginEnd.x - marginStart.x;
  innerHeight = marginEnd.y - marginStart.y;
}

void randomize(void) {
  for (size_t i = 0; i < length; i++) {
    // Position
    Vector2 pos = {(randf() * (float)innerWidth) + marginSize,
                   (randf() * (float)innerHeight) + marginSize};

    // Direction
    Vector2 dir = {randf() * 2.0f - 1.0f, randf() * 2.0f - 1.0f};
    dir = Vector2Normalize(dir);
    dir = Vector2Scale(dir, randf() * (speedLimitMax - speedLimitMin) +
                                speedLimitMin);

    boids[i] = (Boid){pos, dir};
  }
}

void update(void) {
  if (IsKeyPressed(KEY_R)) {
    randomize();
  }

  // Loop through boids
  for (size_t i = 0; i < length; i++) {
    Boid *boid = &boids[i];
    // Get visible boids
    size_t visible = 0;
    for (size_t j = 0; j < length; j++) {
      Boid *b = &boids[j];
      // Checking index should be enough
      if (j != i && Vector2Distance(b->pos, boid->pos) < vision) {
        visibleBoids[visible] = *b;
        visible++;
      }
    }
    flyTowardsCenter(boid, visible);
    avoid(boid);
    matchVelocity(boid, visible);
    checkMargins(boid);
    checkSpeed(boid);
    boid->pos = Vector2Add(
        boid->pos,
        Vector2Scale(boid->velocity,
                     GetFrameTime() * 60.0f)); // 60 for smaller numbers
  }
}

void flyTowardsCenter(Boid *boid, size_t visible) {
  float centerX = 0.0f;
  float centerY = 0.0f;
  for (size_t i = 0; i < visible; i++) {
    centerX += visibleBoids[i].pos.x;
    centerY += visibleBoids[i].pos.y;
  }

  if (visible) {
    centerX = centerX / visible;
    centerY = centerY / visible;

    boid->velocity.x += (centerX - boid->pos.x) * centeringFactor;
    boid->velocity.y += (centerY - boid->pos.y) * centeringFactor;
  }
}

void avoid(Boid *boid) {
  float moveX = 0.0f;
  float moveY = 0.0f;
  for (size_t i = 0; i < length; i++) {
    Boid *b = &boids[i];
    // Checking address should be enough
    if (boid != b) {
      if (Vector2Distance(boid->pos, b->pos) < avoidDistance) {
        moveX += boid->pos.x - b->pos.x;
        moveY += boid->pos.y - b->pos.y;
      }
    }
  }

  boid->velocity.x += moveX * avoidFactor;
  boid->velocity.y += moveY * avoidFactor;
}

void matchVelocity(Boid *boid, size_t visible) {
  float avgX = 0.0f;
  float avgY = 0.0f;
  for (size_t i = 0; i < visible; i++) {
    avgX += visibleBoids[i].velocity.x;
    avgY += visibleBoids[i].velocity.y;
  }

  if (visible) {
    avgX = avgX / visible;
    avgY = avgY / visible;

    boid->velocity.x += (avgX - boid->velocity.x) * matchFactor;
    boid->velocity.y += (avgY - boid->velocity.y) * matchFactor;
  }
}

void checkMargins(Boid *boid) {
  if (boid->pos.x < marginStart.x) {
    boid->velocity.x += marginTurnFactor;
  }
  if (boid->pos.x > marginEnd.x) {
    boid->velocity.x -= marginTurnFactor;
  }
  if (boid->pos.y < marginStart.y) {
    boid->velocity.y += marginTurnFactor;
  }
  if (boid->pos.y > marginEnd.y) {
    boid->velocity.y -= marginTurnFactor;
  }
}

void checkSpeed(Boid *boid) {
  if (Vector2Length(boid->velocity) > speedLimitMax) {
    boid->velocity =
        Vector2Scale(Vector2Normalize(boid->velocity), speedLimitMax);
  } else if (Vector2Length(boid->velocity) < speedLimitMin) {
    boid->velocity =
        Vector2Scale(Vector2Normalize(boid->velocity), speedLimitMin);
  }
}

void draw(void) {
  BeginDrawing();

  ClearBackground(RAYWHITE);

  for (size_t i = 0; i < length; i++) {
    Boid *boid = &boids[i];
    float angle = Vector2Angle((Vector2){0.0f, -1.0f}, boid->velocity);
    Vector2 tOffset = Vector2Rotate((Vector2){0.0f, -9.0f}, angle);
    Vector2 lOffset = Vector2Rotate((Vector2){-6.0f, 9.0f}, angle);
    Vector2 rOffset = Vector2Rotate((Vector2){6.0f, 9.0f}, angle);
    Vector2 top = Vector2Add(boid->pos, tOffset);
    Vector2 left = Vector2Add(boid->pos, lOffset);
    Vector2 right = Vector2Add(boid->pos, rOffset);
    DrawTriangle(top, left, right, DARKGRAY);
  }

  char text[20];
  snprintf(text, 20, "FPS: %d", GetFPS());
  DrawText(text, 8, 8, 20, GREEN);

  EndDrawing();
}
