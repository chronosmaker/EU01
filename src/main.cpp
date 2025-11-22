#include "Common.h"
#include "SSD1327_GFX.h"

#define DEBUG_PERF

// ============================ 显示配置 ============================
// 创建SSD1327显示对象
SSD1327_GFX display(96, 96, &SPI, OLED_DC, OLED_RESET, OLED_CS, 8000000);

// ============================ 性能配置 ============================
const int TARGET_FPS = 30;
const unsigned long TARGET_FRAME_TIME = 1000000 / TARGET_FPS;

// ============================ 星空背景配置 ============================
const int MAX_STARS = 80;           // 最大星星数量
const int TARGET_STARS = 60;        // 目标星星数量
const unsigned long STAR_SPAWN_INTERVAL = 200;  // 生成间隔
const unsigned long MIN_LIFETIME = 10000;       // 最小寿命
const unsigned long MAX_LIFETIME = 30000;       // 最大寿命

// 星星结构体
struct Star {
  int16_t x, y;           // 位置坐标
  uint8_t baseBrightness; // 基础亮度
  uint8_t speed;          // 闪烁速度
  uint16_t birthTime;     // 生成时间
  uint16_t lifetime;      // 寿命
  bool active;            // 活跃状态
  float phase;            // 相位偏移（0-2π）
};

// 星空相关变量
Star stars[MAX_STARS];
int currentStarCount = 0;
uint16_t starTimer = 0;

// ============================ 三体系统配置 ============================
const int NUM_BODIES = 3;
const float G = 1.2f;               // 引力常数
const float dt = 1.0f;              // 时间步长
const float DAMPING = 0.96f;        // 边界碰撞能量损失
const float MIN_DISTANCE = 20.0f;   // 最小初始距离

// 优化的球体结构体
struct Body {
  float x, y;       // 位置坐标
  float vx, vy;     // 速度分量
  float mass;       // 质量
  int8_t radius;    // 绘制半径
  uint8_t color;    // 球体颜色（灰度值）

  // 轨迹跟踪
  static const int TRAIL_LENGTH = 25; // 轨迹长度
  int8_t trailX[TRAIL_LENGTH];        // x坐标轨迹
  int8_t trailY[TRAIL_LENGTH];        // y坐标轨迹
  uint8_t trailIndex;                 // 当前轨迹索引
  uint8_t trailColor;                 // 轨迹颜色
  float trailIntensity;               // 轨迹强度
};

// 三体系统变量
Body bodies[NUM_BODIES];
float distanceCache[NUM_BODIES][NUM_BODIES];  // 距离缓存

// ============================ 数学优化函数 ============================

/**
 * @brief 快速平方根近似计算
 */
inline float fastSqrt(float x) {
  union {
    float f;
    int32_t i;
  } conv;

  conv.f = x;
  conv.i = 0x5f3759df - (conv.i >> 1);
  return x * conv.f * (1.5f - 0.5f * x * conv.f * conv.f);
}

/**
 * @brief 快速距离计算（避免开方）
 */
inline float fastDistanceSq(float dx, float dy) {
  return dx * dx + dy * dy;
}

// ============================ 轨迹管理函数 ============================

/**
 * @brief 初始化球体轨迹数据
 * @param bodyIndex 球体索引
 */
void initializeTrail(int bodyIndex) {
  for (int i = 0; i < Body::TRAIL_LENGTH; i++) {
    bodies[bodyIndex].trailX[i] = (int8_t)bodies[bodyIndex].x;
    bodies[bodyIndex].trailY[i] = (int8_t)bodies[bodyIndex].y;
  }
  bodies[bodyIndex].trailIndex = 0;
  bodies[bodyIndex].trailColor = max(4, bodies[bodyIndex].color / 2);
  bodies[bodyIndex].trailIntensity = 1.0f;
}

/**
 * @brief 更新球体轨迹数据
 * @param bodyIndex 球体索引
 */
void updateTrail(int bodyIndex) {
  bodies[bodyIndex].trailIndex = (bodies[bodyIndex].trailIndex + 1) % Body::TRAIL_LENGTH;
  bodies[bodyIndex].trailX[bodies[bodyIndex].trailIndex] = (int8_t)bodies[bodyIndex].x;
  bodies[bodyIndex].trailY[bodies[bodyIndex].trailIndex] = (int8_t)bodies[bodyIndex].y;

  // 基于速度计算轨迹强度
  float speed = fastSqrt(bodies[bodyIndex].vx * bodies[bodyIndex].vx +
    bodies[bodyIndex].vy * bodies[bodyIndex].vy);
  bodies[bodyIndex].trailIntensity = min(1.5f, speed * 3.0f);
}

/**
 * @brief 绘制球体轨迹
 * @param bodyIndex 球体索引
 */
void drawTrail(int bodyIndex) {
  uint8_t baseColor = bodies[bodyIndex].trailColor;
  float intensity = bodies[bodyIndex].trailIntensity;

  for (int i = 0; i < Body::TRAIL_LENGTH; i++) {
    // 计算轨迹点位置（从新到旧）
    int trailPos = (bodies[bodyIndex].trailIndex - i + Body::TRAIL_LENGTH) % Body::TRAIL_LENGTH;

    int x = bodies[bodyIndex].trailX[trailPos];
    int y = bodies[bodyIndex].trailY[trailPos];

    // 边界检查
    if (x < 0 || x >= 96 || y < 0 || y >= 96) continue;

    // 计算轨迹点亮度（年龄衰减）
    float ageRatio = (float)i / Body::TRAIL_LENGTH;
    float attenuation = ageRatio * ageRatio;
    uint8_t brightness = (uint8_t)(baseColor * intensity * (1.0f - attenuation));

    if (brightness < 2) continue;

    // 根据轨迹年龄采用不同绘制策略
    if (i < 4) {
      // 新轨迹：明亮显示
      display.drawPixel(x, y, min((uint8_t)15, brightness));
      if (brightness > 8) {
        display.drawPixel(x + 1, y, brightness / 3);
        display.drawPixel(x - 1, y, brightness / 3);
      }
    } else if (i < 10) {
      // 中等轨迹
      display.drawPixel(x, y, brightness / 2);
    } else {
      // 旧轨迹：间隔绘制
      if (i % 2 == 0 && brightness > 3) {
        display.drawPixel(x, y, brightness / 4);
      }
    }
  }
}

// ============================ 球体绘制 ============================

/**
 * @brief 绘制球形
 * @param x 中心x坐标
 * @param y 中心y坐标
 * @param radius 半径
 * @param baseColor 基础颜色
 */
void drawBall(int x, int y, int radius, uint8_t baseColor) {
  // 快速边界检查
  if (x < -radius || x > 96 + radius || y < -radius || y > 96 + radius) {
    return;
  }

  // 根据半径选择绘制策略
  if (radius <= 2) {
    // 小半径直接绘制
    display.fillCircle(x, y, radius, baseColor);
  } else if (radius <= 4) {
    // 中等半径：简单渐变
    display.drawCircle(x, y, radius, max(3, baseColor / 2));
    display.fillCircle(x, y, radius - 1, baseColor);
  } else {
    // 大半径：多层渐变
    for (int r = radius; r >= 0; r--) {
      uint8_t gray = map(r, 0, radius, baseColor, max(3, baseColor - r));
      display.drawCircle(x, y, r, gray);
      if (r <= radius / 2) break; // 内层使用相同颜色
    }
    display.fillCircle(x, y, radius / 2, baseColor);
  }
}

// ============================ 物理系统函数 ============================

/**
 * @brief 检查位置有效性（避免球体重叠）
 * @param x 待检查x坐标
 * @param y 待检查y坐标
 * @param radius 球体半径
 * @param excludeIndex 排除的球体索引
 * @return true=位置有效, false=位置无效
 */
bool isValidPosition(float x, float y, int radius, int excludeIndex = -1) {
  for (int i = 0; i < NUM_BODIES; i++) {
    if (i == excludeIndex || bodies[i].radius == 0) continue;

    float dx = x - bodies[i].x;
    float dy = y - bodies[i].y;
    float distanceSq = dx * dx + dy * dy;
    float minDistance = radius + bodies[i].radius + 12.0f; // 保持额外间距

    if (distanceSq < minDistance * minDistance) {
      return false;
    }
  }
  return true;
}

/**
 * @brief 预计算球体间距离数据
 */
void precomputeDistances() {
  // 使用快速距离计算
  for (int i = 0; i < NUM_BODIES; i++) {
    for (int j = i + 1; j < NUM_BODIES; j++) {
      float dx = bodies[j].x - bodies[i].x;
      float dy = bodies[j].y - bodies[i].y;
      distanceCache[i][j] = dx * dx + dy * dy; // 存储平方距离
      distanceCache[j][i] = distanceCache[i][j];
    }
  }
}

// ============================ 星空背景函数 ============================

/**
 * @brief 生成新星星
 */
void spawnStar() {
  if (currentStarCount >= MAX_STARS) return;

  for (int i = 0; i < MAX_STARS; i++) {
    if (!stars[i].active) {
      stars[i].x = random(0, 96);
      stars[i].y = random(0, 96);
      stars[i].baseBrightness = random(1, 5); // 基础亮度
      stars[i].speed = random(8, 24);
      stars[i].birthTime = starTimer;
      stars[i].lifetime = random(MIN_LIFETIME / 100, MAX_LIFETIME / 100);
      stars[i].phase = random(0, 628) / 100.0f;
      stars[i].active = true;

      currentStarCount++;
      break;
    }
  }
}

/**
 * @brief 移除过期星星
 */
void removeExpiredStars() {
  for (int i = 0; i < MAX_STARS; i++) {
    if (stars[i].active) {
      uint16_t age = starTimer - stars[i].birthTime;
      if (age > stars[i].lifetime) {
        stars[i].active = false;
        currentStarCount--;
      }
    }
  }
}

/**
 * @brief 动态管理星星生命周期
 */
void manageStars() {
  starTimer++;

  // 移除过期星星
  removeExpiredStars();

  // 动态生成星星
  if (starTimer % (STAR_SPAWN_INTERVAL / TARGET_FPS) == 0) {
    if (currentStarCount < TARGET_STARS) {
      spawnStar();
    }
  }

  // 随机消失
  if (random(1000) == 0 && currentStarCount > TARGET_STARS / 2) {
    for (int i = 0; i < MAX_STARS; i++) {
      if (stars[i].active) {
        stars[i].active = false;
        currentStarCount--;
        break;
      }
    }
  }
}

/**
 * @brief 初始化星空背景
 */
void initializeStars() {
  currentStarCount = 0;
  starTimer = 0;

  // 初始化所有星星为未激活状态
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].active = false;
  }

  // 预生成星星
  for (int i = 0; i < TARGET_STARS / 2; i++) {
    spawnStar();
  }
}

/**
 * @brief 绘制星空背景
 */
void drawStarfield() {
  for (int i = 0; i < MAX_STARS; i++) {
    if (!stars[i].active) continue;

    // 计算生命周期亮度调整
    uint16_t age = starTimer - stars[i].birthTime;
    float ageRatio = (float)age / stars[i].lifetime;
    float lifeBrightness = 1.0f;

    // 生命周期亮度调整
    if (ageRatio < 0.2f) {
      // 淡入阶段（前20%生命周期）
      lifeBrightness = ageRatio / 0.2f;
    } else if (ageRatio > 0.8f) {
      // 淡出阶段（最后20%生命周期）
      lifeBrightness = (1.0f - ageRatio) / 0.2f;
    }

    // 呼吸效果
    float timeFactor = starTimer * 0.05f * (stars[i].speed * 0.01f) + stars[i].phase;
    float brightnessVariation = (sin(timeFactor) + sin(timeFactor * 1.7f)) * 0.5f;
    uint8_t currentBrightness = stars[i].baseBrightness +
      (uint8_t)(brightnessVariation * stars[i].baseBrightness * 0.6f);

    // 应用生命周期亮度调整
    currentBrightness = (uint8_t)(currentBrightness * lifeBrightness);
    currentBrightness = constrain(currentBrightness, 1, 15);

    // 绘制星星（99%显示概率）
    if (random(100) < 99) {
      display.drawPixel(stars[i].x, stars[i].y, currentBrightness);
    }
  }
}

// ============================ 性能监控 ============================
#ifdef DEBUG_PERF
unsigned long frameCount = 0;
unsigned long lastFPSUpdate = 0;
float currentFPS = 0;
#endif

// ============================ 主程序函数 ============================

void setup() {
  Serial.begin(115200);
  delay(100); // 等待串口稳定

  Serial.println("三体运动模拟启动");

  // 初始化显示
  if (!display.begin()) {
    Serial.println("SSD1327初始化失败!");
    while (1) {
      delay(1000);
    }
  }

  // 设置显示参数
  display.setRotation(0);
  display.setTextColor(MONOOLED_WHITE);
  display.setTextSize(1);

  Serial.println("显示初始化成功");

  // 初始化随机数生成器
  randomSeed(esp_random());

  // 配置三体系统
  // 球体1：中等大小
  bodies[0].radius = 4;
  bodies[0].mass = bodies[0].radius * bodies[0].radius * 0.8f;
  bodies[0].color = 10;

  // 球体2：较小
  bodies[1].radius = 3;
  bodies[1].mass = bodies[1].radius * bodies[1].radius * 0.8f;
  bodies[1].color = 10;

  // 球体3：较大
  bodies[2].radius = 5;
  bodies[2].mass = bodies[2].radius * bodies[2].radius * 0.8f;
  bodies[2].color = 10;

  // 初始位置设置
  bool positionsValid = false;
  int attempts = 0;
  const int maxAttempts = 50;

  while (!positionsValid && attempts < maxAttempts) {
    attempts++;

    // 使用三角形布局
    bodies[0].x = random(20, 40);  // 左下
    bodies[0].y = random(60, 80);

    bodies[1].x = random(60, 80);  // 右上
    bodies[1].y = random(20, 40);

    bodies[2].x = random(20, 40);  // 左上
    bodies[2].y = random(20, 40);

    // 检查所有位置是否有效
    positionsValid = true;
    for (int i = 0; i < NUM_BODIES; i++) {
      if (!isValidPosition(bodies[i].x, bodies[i].y, bodies[i].radius, i)) {
        positionsValid = false;
        break;
      }
    }
  }

  // 如果随机放置失败，使用预设位置
  if (!positionsValid) {
    bodies[0].x = 25; bodies[0].y = 70;
    bodies[1].x = 75; bodies[1].y = 25;
    bodies[2].x = 25; bodies[2].y = 25;
  }

  // 设置初始速度（形成混沌系统）
  // 球体1：向右上运动
  bodies[0].vx = random(4, 8) * 0.1f;
  bodies[0].vy = random(-8, -4) * 0.1f;

  // 球体2：向左下运动
  bodies[1].vx = random(-8, -4) * 0.1f;
  bodies[1].vy = random(4, 8) * 0.1f;

  // 球体3：向右下运动
  bodies[2].vx = random(4, 8) * 0.1f;
  bodies[2].vy = random(4, 8) * 0.1f;

  // 初始化轨迹
  for (int i = 0; i < NUM_BODIES; i++) {
    initializeTrail(i);
  }

  // 初始化星空
  initializeStars();

  Serial.print("系统初始化完成，尝试次数: ");
  Serial.println(attempts);
  Serial.println("开始模拟...");
}

void loop() {
  unsigned long frameStart = micros();

  // 清屏
  display.clearDisplay();

  // 1. 管理星空背景
  manageStars();
  drawStarfield();

  // 2. 预计算距离
  precomputeDistances();

  // 3. 计算物理运动
  for (int i = 0; i < NUM_BODIES; i++) {
    float fx = 0.0f, fy = 0.0f;

    for (int j = 0; j < NUM_BODIES; j++) {
      if (i == j) continue;

      float distanceSq = distanceCache[i][j];
      float minDistance = bodies[i].radius + bodies[j].radius + 6.0f;
      float minDistanceSq = minDistance * minDistance;

      // 防止距离过小导致计算不稳定
      distanceSq = max(distanceSq, minDistanceSq);

      float dx = (bodies[j].x - bodies[i].x);
      float dy = (bodies[j].y - bodies[i].y);
      float distance = fastSqrt(distanceSq);

      dx /= distance;
      dy /= distance;

      // 万有引力公式（软化处理）
      float force = G * bodies[i].mass * bodies[j].mass / (distanceSq + 40.0f);
      fx += force * dx;
      fy += force * dy;
    }

    // 更新速度
    bodies[i].vx += fx / bodies[i].mass * dt;
    bodies[i].vy += fy / bodies[i].mass * dt;

    // 速度限制
    float speed = fastSqrt(bodies[i].vx * bodies[i].vx + bodies[i].vy * bodies[i].vy);
    float maxSpeed = 3.0f;
    if (speed > maxSpeed) {
      bodies[i].vx *= maxSpeed / speed;
      bodies[i].vy *= maxSpeed / speed;
    }
  }

  // 4. 更新位置和边界检测
  for (int i = 0; i < NUM_BODIES; i++) {
    bodies[i].x += bodies[i].vx * dt;
    bodies[i].y += bodies[i].vy * dt;

    // 边界处理
    int leftBound = bodies[i].radius;
    int rightBound = 96 - bodies[i].radius;
    int topBound = bodies[i].radius;
    int bottomBound = 96 - bodies[i].radius;

    // X轴边界碰撞
    if (bodies[i].x < leftBound) {
      bodies[i].x = leftBound;
      bodies[i].vx = -bodies[i].vx * DAMPING;
    } else if (bodies[i].x > rightBound) {
      bodies[i].x = rightBound;
      bodies[i].vx = -bodies[i].vx * DAMPING;
    }

    // Y轴边界碰撞
    if (bodies[i].y < topBound) {
      bodies[i].y = topBound;
      bodies[i].vy = -bodies[i].vy * DAMPING;
    } else if (bodies[i].y > bottomBound) {
      bodies[i].y = bottomBound;
      bodies[i].vy = -bodies[i].vy * DAMPING;
    }

    // 更新轨迹
    updateTrail(i);
  }

  // 5. 绘制轨迹
  for (int i = 0; i < NUM_BODIES; i++) {
    drawTrail(i);
  }

  // 6. 绘制球体
  for (int i = 0; i < NUM_BODIES; i++) {
    drawBall(bodies[i].x, bodies[i].y, bodies[i].radius, bodies[i].color);
  }

  // 7. 刷新显示
  display.display();

  // 8. 帧率控制
  unsigned long frameTime = micros() - frameStart;
  unsigned long sleepTime = (frameTime < TARGET_FRAME_TIME) ?
    (TARGET_FRAME_TIME - frameTime) : 0;

  delayMicroseconds(sleepTime);

  // 性能监控
#ifdef DEBUG_PERF
  frameCount++;
  if (millis() - lastFPSUpdate >= 1000) {
    currentFPS = frameCount * 1000.0 / (millis() - lastFPSUpdate);
    Serial.printf("FPS: %.1f, FrameTime: %luμs\n", currentFPS, frameTime);
    frameCount = 0;
    lastFPSUpdate = millis();
  }
#endif
}