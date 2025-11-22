#include "Common.h"
#include "SSD1327_GFX.h"

// 创建SSD1327显示对象
SSD1327_GFX display(96, 96, &SPI, OLED_DC, OLED_RESET, OLED_CS, 8000000);

// 星空背景参数 - 动态管理
const int MAX_STARS = 100; // 最大星星数量
const int TARGET_STARS = 80; // 目标星星数量
int currentStarCount = 0; // 当前星星数量

struct Star {
  int x, y;           // 位置
  uint8_t baseBrightness; // 基础亮度
  float phase;        // 相位偏移（0-2π）
  float speed;        // 闪烁速度
  unsigned long birthTime; // 星星生成时间
  int lifetime;       // 星星寿命（毫秒）
  bool active;        // 星星是否活跃
};

Star stars[MAX_STARS];

// 星星生成和移除参数
const unsigned long STAR_SPAWN_INTERVAL = 500; // 生成间隔（毫秒）
const unsigned long MIN_LIFETIME = 5000;       // 最小寿命
const unsigned long MAX_LIFETIME = 30000;      // 最大寿命
unsigned long lastSpawnTime = 0;              // 上次生成时间

// 定义球体结构
struct Body {
  float x, y;     // 位置
  float vx, vy;   // 速度
  float mass;     // 质量
  int radius;     // 绘制半径
  uint8_t color;  // 球体颜色（灰度值）

  // 轨迹跟踪相关变量
  static const int TRAIL_LENGTH = 20; // 轨迹长度
  float trailX[TRAIL_LENGTH]; // x坐标轨迹
  float trailY[TRAIL_LENGTH]; // y坐标轨迹
  int trailIndex; // 当前轨迹索引
  uint8_t trailColor; // 轨迹颜色（比球体颜色暗）
};

// 三体系统参数
const int NUM_BODIES = 3;
Body bodies[NUM_BODIES];
const float G = 0.98f;       // 引力常数
const float dt = 1.96f;      // 时间步长
const float DAMPING = 0.98f; // 边界碰撞能量损失
const float MIN_DISTANCE = 24.0f; // 球体间最小初始距离

// 预计算平方和平方根值以提高性能
float precomputedDistances[NUM_BODIES][NUM_BODIES];

// 初始化轨迹数据
void initializeTrail(int bodyIndex) {
  for (int i = 0; i < Body::TRAIL_LENGTH; i++) {
    bodies[bodyIndex].trailX[i] = bodies[bodyIndex].x;
    bodies[bodyIndex].trailY[i] = bodies[bodyIndex].y;
  }
  bodies[bodyIndex].trailIndex = 0;
  // 轨迹颜色比球体颜色暗（约为球体颜色的1/2），并设置最小亮度为4
  bodies[bodyIndex].trailColor = max(4, bodies[bodyIndex].color / 2);
}

void drawTrail(int bodyIndex) {
  // 从最新的轨迹点开始绘制（当前索引的前一个位置）
  for (int i = 0; i < Body::TRAIL_LENGTH; i++) {
    // 计算轨迹点位置：从新到旧（i=0是最新，i=TRAIL_LENGTH-1是最旧）
    int trailPos = (bodies[bodyIndex].trailIndex - i + Body::TRAIL_LENGTH) % Body::TRAIL_LENGTH;

    // 跳过未初始化的轨迹点
    if (bodies[bodyIndex].trailX[trailPos] == 0 &&
      bodies[bodyIndex].trailY[trailPos] == 0) {
      continue;
    }

    // 计算轨迹点的灰度值（越旧的轨迹越暗）
    float ageRatio = (float)i / Body::TRAIL_LENGTH;
    // 使用指数衰减：年龄比例的平方，让衰减更陡峭
    float attenuation = ageRatio * ageRatio;
    uint8_t pointBrightness = bodies[bodyIndex].trailColor * (1.0 - attenuation);

    // 设置最小亮度阈值
    pointBrightness = max(uint8_t(1), pointBrightness);

    int x = (int)bodies[bodyIndex].trailX[trailPos];
    int y = (int)bodies[bodyIndex].trailY[trailPos];

    if (x >= 0 && x < 96 && y >= 0 && y < 96) {
      // 根据轨迹年龄采用不同的绘制策略
      if (i == 0) {
        // 最新的点：不绘制（与球体重合）
        continue;
      } else if (i <= 3) {
        // 较新的轨迹：绘制明亮的小点
        display.drawPixel(x, y, pointBrightness);
        // 添加周围像素增强可见性
        if (pointBrightness > 6) {
          display.drawPixel(x + 1, y, pointBrightness / 2);
          display.drawPixel(x - 1, y, pointBrightness / 2);
        }
      } else if (i <= 8) {
        // 中等年龄的轨迹：绘制单像素点
        display.drawPixel(x, y, pointBrightness);
      } else if (i <= 15) {
        // 较旧的轨迹：降低亮度绘制
        if (pointBrightness > 2) {
          display.drawPixel(x, y, pointBrightness / 2);
        }
      } else {
        // 最旧的轨迹：间隔绘制以节省资源
        if (i % 2 == 0 && pointBrightness > 1) {
          display.drawPixel(x, y, 1);
        }
      }
    }
  }
}

// 轨迹更新逻辑，确保轨迹颜色动态调整
void updateTrail(int bodyIndex) {
  bodies[bodyIndex].trailIndex = (bodies[bodyIndex].trailIndex + 1) % Body::TRAIL_LENGTH;
  bodies[bodyIndex].trailX[bodies[bodyIndex].trailIndex] = bodies[bodyIndex].x;
  bodies[bodyIndex].trailY[bodies[bodyIndex].trailIndex] = bodies[bodyIndex].y;

  // 根据速度动态调整轨迹基准亮度，让快速运动的物体轨迹更明显
  float speed = sqrt(bodies[bodyIndex].vx * bodies[bodyIndex].vx +
    bodies[bodyIndex].vy * bodies[bodyIndex].vy);
  uint8_t speedBrightness = min(uint8_t(15), uint8_t(speed * 8 + 4));
  bodies[bodyIndex].trailColor = max(uint8_t(bodies[bodyIndex].color / 2), speedBrightness);
}

// 绘制渐变球形函数
void drawGradientBall(int x, int y, int radius, uint8_t baseColor) {
  // 边界检查，避免绘制屏幕外内容
  if (x + radius < 0 || x - radius > 95 || y + radius < 0 || y - radius > 95) {
    return;
  }

  // 根据半径大小选择绘制细节
  if (radius <= 3) {
    // 小半径直接绘制实心圆
    display.fillCircle(x, y, radius, baseColor);
  } else {
    // 大半径绘制渐变效果
    for (int r = radius; r >= 0; r--) {
      uint8_t gray = map(r, 0, radius, baseColor, max(5, baseColor - 8));
      display.drawCircle(x, y, r, gray);
    }
    // 填充中心区域
    display.fillCircle(x, y, radius / 2, baseColor);
  }
}

// 检查位置是否有效（与其他球体保持足够距离）
bool isValidPosition(float x, float y, int radius, int excludeIndex = -1) {
  for (int i = 0; i < NUM_BODIES; i++) {
    if (i == excludeIndex) continue;
    if (bodies[i].radius == 0) continue; // 未初始化的球体

    float dx = x - bodies[i].x;
    float dy = y - bodies[i].y;
    float distance = sqrt(dx * dx + dy * dy);
    float minAcceptable = radius + bodies[i].radius + 16; // 保持额外间距

    if (distance < minAcceptable) {
      return false;
    }
  }
  return true;
}

// 预计算距离相关数据
void precomputeDistances() {
  for (int i = 0; i < NUM_BODIES; i++) {
    for (int j = i + 1; j < NUM_BODIES; j++) {
      float dx = bodies[j].x - bodies[i].x;
      float dy = bodies[j].y - bodies[i].y;
      precomputedDistances[i][j] = dx * dx + dy * dy; // 存储平方距离
      precomputedDistances[j][i] = precomputedDistances[i][j];
    }
  }
}

// 生成新星星
void spawnStar() {
  if (currentStarCount >= MAX_STARS) return;

  // 寻找空闲的星星槽位
  for (int i = 0; i < MAX_STARS; i++) {
    if (!stars[i].active) {
      stars[i].x = random(0, 96);
      stars[i].y = random(0, 96);
      stars[i].baseBrightness = random(1, 5);
      stars[i].phase = random(0, 628) / 100.0;
      stars[i].speed = random(5, 20) / 100.0;
      stars[i].birthTime = millis();
      stars[i].lifetime = random(MIN_LIFETIME, MAX_LIFETIME);
      stars[i].active = true;

      currentStarCount++;
      break;
    }
  }
}

// 移除过期星星
void removeExpiredStars() {
  unsigned long currentTime = millis();

  for (int i = 0; i < MAX_STARS; i++) {
    if (stars[i].active && (currentTime - stars[i].birthTime > stars[i].lifetime)) {
      stars[i].active = false;
      currentStarCount--;
    }
  }
}

// 动态管理星星数量
void manageStars() {
  unsigned long currentTime = millis();

  // 移除过期星星
  removeExpiredStars();

  // 定期生成新星星
  if (currentTime - lastSpawnTime > STAR_SPAWN_INTERVAL) {
    // 如果星星数量不足，生成新星星
    if (currentStarCount < TARGET_STARS) {
      spawnStar();
    }
    lastSpawnTime = currentTime;
  }

  // 随机移除星星（模拟星星自然消失）
  if (random(1000) < 2 && currentStarCount > TARGET_STARS / 2) { // 0.2%的概率每帧移除一颗星星
    for (int i = 0; i < MAX_STARS; i++) {
      if (stars[i].active) {
        stars[i].active = false;
        currentStarCount--;
        break;
      }
    }
  }
}

// 初始化星空背景
void initializeStars() {
  randomSeed(analogRead(0));
  currentStarCount = 0;

  // 初始化所有星星为未激活状态
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].active = false;
  }

  // 生成初始星星
  for (int i = 0; i < TARGET_STARS; i++) {
    spawnStar();
  }
}

// 绘制星空背景
void drawStarfield() {
  unsigned long currentTime = millis();

  for (int i = 0; i < MAX_STARS; i++) {
    if (!stars[i].active) continue;

    // 计算星星年龄比例（用于淡入淡出效果）
    float ageRatio = (float)(currentTime - stars[i].birthTime) / stars[i].lifetime;

    // 生命周期亮度调整：淡入和淡出
    float lifeBrightness = 1.0;
    if (ageRatio < 0.1) {
      // 淡入阶段（前10%生命周期）
      lifeBrightness = ageRatio / 0.1;
    } else if (ageRatio > 0.9) {
      // 淡出阶段（最后10%生命周期）
      lifeBrightness = (1.0 - ageRatio) / 0.1;
    }

    // 呼吸效果
    float timeFactor = currentTime * 0.001 * stars[i].speed + stars[i].phase;
    float brightnessVariation = sin(timeFactor);
    uint8_t currentBrightness = stars[i].baseBrightness +
      (uint8_t)(brightnessVariation * stars[i].baseBrightness * 0.7);

    // 应用生命周期亮度调整
    currentBrightness = (uint8_t)(currentBrightness * lifeBrightness);
    currentBrightness = constrain(currentBrightness, 1, 15);

    // 随机闪烁效果
    if (random(1000) < 995) { // 99.5%的概率显示
      display.drawPixel(stars[i].x, stars[i].y, currentBrightness);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // 初始化显示
  if (!display.begin()) {
    Serial.println("SSD1327初始化失败!");
    while (1);
  }
  Serial.println("三体运动模拟启动");

  // 初始化三个球体
  randomSeed(analogRead(0));

  // 球体1：中等大小 - 放置在左下区域
  bodies[0].radius = 4;
  bodies[0].mass = bodies[0].radius * bodies[0].radius * 0.6f;
  bodies[0].color = 12;

  // 球体2：较小 - 放置在右上区域  
  bodies[1].radius = 3;
  bodies[1].mass = bodies[1].radius * bodies[1].radius * 0.6f;
  bodies[1].color = 10;

  // 球体3：较大 - 放置在左上区域
  bodies[2].radius = 5;
  bodies[2].mass = bodies[2].radius * bodies[2].radius * 0.6f;
  bodies[2].color = 14;

  // 设置初始位置，确保彼此远离
  bool positionsValid = false;
  int attempts = 0;

  while (!positionsValid && attempts < 100) {
    attempts++;

    // 尝试放置球体1（左下区域）
    bodies[0].x = random(20, 40);
    bodies[0].y = random(60, 80);

    // 尝试放置球体2（右上区域）
    bodies[1].x = random(60, 80);
    bodies[1].y = random(20, 40);

    // 尝试放置球体3（左上区域）
    bodies[2].x = random(20, 40);
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

  if (!positionsValid) {
    // 如果随机放置失败，使用预设的分散位置
    bodies[0].x = 30; bodies[0].y = 70; // 左下
    bodies[1].x = 70; bodies[1].y = 30; // 右上  
    bodies[2].x = 30; bodies[2].y = 30; // 左上
  }

  // 设置初始速度 - 让它们有向外运动的趋势
  // 球体1：向右上运动
  bodies[0].vx = random(3, 6) * 0.1f;
  bodies[0].vy = random(-6, -3) * 0.1f;

  // 球体2：向左下运动
  bodies[1].vx = random(-6, -3) * 0.1f;
  bodies[1].vy = random(3, 6) * 0.1f;

  // 球体3：向右下运动
  bodies[2].vx = random(3, 6) * 0.1f;
  bodies[2].vy = random(3, 6) * 0.1f;

  // 初始化轨迹数据
  for (int i = 0; i < NUM_BODIES; i++) {
    initializeTrail(i);
  }

  // 初始化星空背景
  initializeStars();

  Serial.print("初始化尝试次数: ");
  Serial.println(attempts);
}

void loop() {
  unsigned long startTime = micros();
  display.clearDisplay();

  // 管理星星生命周期（生成和移除）
  manageStars();

  // 绘制星空背景
  drawStarfield();

  // 预计算距离数据
  precomputeDistances();

  // 1. 计算引力并更新运动状态
  for (int i = 0; i < NUM_BODIES; i++) {
    float fx = 0, fy = 0;

    for (int j = 0; j < NUM_BODIES; j++) {
      if (i == j) continue;

      float distanceSq = precomputedDistances[i][j];
      float minDistance = bodies[i].radius + bodies[j].radius + 8.0f; // 增加最小距离
      float minDistanceSq = minDistance * minDistance;

      // 防止距离过小导致计算不稳定
      distanceSq = max(distanceSq, minDistanceSq);

      float distance = sqrt(distanceSq);
      float dx = (bodies[j].x - bodies[i].x) / distance;
      float dy = (bodies[j].y - bodies[i].y) / distance;

      // 万有引力公式 - 使用软化处理避免过强引力
      float force = G * bodies[i].mass * bodies[j].mass / (distanceSq + 50.0f);
      fx += force * dx;
      fy += force * dy;
    }

    // 更新速度
    bodies[i].vx += fx / bodies[i].mass * dt;
    bodies[i].vy += fy / bodies[i].mass * dt;

    // 限制最大速度避免运动过快
    float speed = sqrt(bodies[i].vx * bodies[i].vx + bodies[i].vy * bodies[i].vy);
    float maxSpeed = 2.5f;
    if (speed > maxSpeed) {
      bodies[i].vx = bodies[i].vx * maxSpeed / speed;
      bodies[i].vy = bodies[i].vy * maxSpeed / speed;
    }
  }

  // 2. 更新位置并处理边界碰撞
  for (int i = 0; i < NUM_BODIES; i++) {
    bodies[i].x += bodies[i].vx * dt;
    bodies[i].y += bodies[i].vy * dt;

    // 边界碰撞检测（考虑球体半径）
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

  // 3. 绘制轨迹（先绘制轨迹，确保在球体下方）
  for (int i = 0; i < NUM_BODIES; i++) {
    drawTrail(i);
  }

  // 4. 绘制球体
  for (int i = 0; i < NUM_BODIES; i++) {
    drawGradientBall(bodies[i].x, bodies[i].y, bodies[i].radius, bodies[i].color);
  }

  display.display();

  // 动态调整延迟以保持稳定帧率
  unsigned long frameTime = micros() - startTime;
  unsigned long targetFrameTime = 40000; // ~25 FPS
  if (frameTime < targetFrameTime) {
    delayMicroseconds(targetFrameTime - frameTime);
  }
}
