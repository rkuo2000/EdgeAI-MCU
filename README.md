# EdgeAI-MCU 作業技術報告

---

## 作業一：YOLOv7 神經網路物件偵測監控系統

### 來源檔案
- **作業說明**：`AI_YOLOv7_Survillience.md`
- **程式碼**：`YOLOv7_Survellience.ino`

### 作業目標
利用 YOLOv7 神經網路模型偵測 person（人）、bicycle（腳踏車）、car（汽車）、motorcycle（機車）、bus（公車）、truck（卡車），並結合 WebSocket Viewer 即時顯示畫面，加上 NTPClient 取得網路時間作為檔案命名，將偵測到的影像存入 SD 卡。

### 程式技術細節

#### 使用的函式庫
| 函式庫 | 用途 |
|--------|------|
| `StreamIO.h` | 影片串流管道 |
| `VideoStream.h` | 攝影機影片擷取 |
| `NNObjectDetection.h` | 神經網路物件偵測 |
| `VideoStreamOverlay.h` | 畫面疊加 OSD 繪圖 |
| `ObjectClassList.h` | 物件類別列表與過濾 |
| `AmebaFatFS.h` | SD 卡檔案系統 |
| `WebSocketViewer.h` | WebSocket 即時檢視 |
| `NTPClient.h` | 網路時間同步 |
| `WiFi.h` / `WiFiUdp.h` | WiFi 連線與 UDP |

#### 硬體設定參數
- **NN 解析度**：576 × 320（`NNWIDTH` / `NNHEIGHT`）
- **影片頻道 0**：FHD (1920×1080), 30fps, H.264
- **NN 頻道 3**：576×320, 10fps, RGB
- **JPEG 頻道 1**：FHD, JPEG 格式

#### 主要全域變數
```cpp
VideoSetting config(VIDEO_FHD, 30, VIDEO_H264, 0);          // 主影片
VideoSetting configNN(NNWIDTH, NNHEIGHT, 10, VIDEO_RGB, 0); // NN 輸入
VideoSetting configJPEG(VIDEO_FHD, CAM_FPS, VIDEO_JPEG, 1); // JPEG 擷圖
NNObjectDetection ObjDet;           // 物件偵測器
StreamIO videoStreamer(1, 1);       // 主串流
StreamIO videoStreamerNN(1, 1);     // NN 串流
WebSocketViewer ws_viewer;          // WebSocket 檢視器
NTPClient timeClient(ntpUDP, "tw.pool.ntp.org", 28800, 60000);
                                    // NTP 用戶端（UTC+8）
```

#### 初始化流程（`setup()`）
1. **WiFi 連線**：連線至指定 SSID，等待連線成功
2. **NTP 初始化**：設定台灣時區（UTC+8），每 60 秒同步
3. **WebSocket 資源載入**：啟用 WebSocket 檢視器
4. **攝影機設定**：設定三個影片頻道（H.264、RGB、JPEG）並初始化
5. **YOLOv7 模型載入**：選用 `DEFAULT_YOLOV7TINY`，註冊回呼函式 `ODPostProcess`
6. **串流管道建立**：`videoStreamer` 連接攝影機 → WebSocket；`videoStreamerNN` 連接 NN 頻道 → 物件偵測器
7. **OSD 疊加啟動**：在 H.264 與 JPEG 頻道上啟用畫面繪圖

#### 主循環（`loop()`）
主循環為空，所有物件偵測處理皆透過**回呼函式** `ODPostProcess` 非同步執行。

#### 回呼處理（`ODPostProcess()`）
1. **取得結果數量**，建立 OSD 位圖
2. **逐一檢查每個偵測物件**：
   - 過濾 `obj_type` 為 0~4、5、7（person/bicycle/car/motorcycle/bus/truck）
   - 若符合則設定 `objects_detected_flag = true`
   - 若 `itemList[obj_type].filter` 允許顯示，則繪製邊界框與標籤文字
3. **更新 OSD 畫面**
4. **若 `objects_detected_flag` 為 true**：
   - 更新 NTP 時間，產生時間戳字串（`年_月_日_時_分_秒`）
   - 若時間在 **0:00 ~ 6:00**（深夜時段），則從 JPEG 頻道擷取影像
   - 以 `ObjDet_時間戳.jpg` 格式存檔至 SD 卡
   - 重設旗標，延遲 5 秒

#### 偵測物件過濾邏輯
| 類別索引 | 物件名稱 | 是否監控 |
|----------|----------|----------|
| 0 | person | ✅ |
| 1 | bicycle | ✅ |
| 2 | car | ✅ |
| 3 | motorcycle | ✅ |
| 4 | airplane | ❌ |
| 5 | bus | ✅ |
| 6 | train | ❌ |
| 7 | truck | ✅ |

### 流程圖說明

```
┌──────────────────────────────────────────┐
│                開始                        │
├──────────────────────────────────────────┤
│  初始化序列埠 (Serial.begin)              │
│                                          │
│  ┌─ WiFi 連線迴圈 ──────────────────┐   │
│  │  WiFi.begin(ssid, pass)          │   │
│  │  等待連線成功                     │   │
│  └──────────────────────────────────┘   │
│                                          │
│  啟動 NTP 時間同步                       │
│  載入 WebSocket 資源                     │
│  設定攝影機三個頻道 (H.264/RGB/JPEG)     │
│  初始化攝影機                           │
│                                          │
│  初始化 WebSocket 檢視器                 │
│  YOLOv7TINY 模型載入與啟動              │
│  建立串流管道 (Camera→WS, Camera→NN)   │
│  啟動 OSD 疊加繪圖                      │
└──────────────────────────────────────────┘
                      │
                      ▼
┌──────────────────────────────────────────┐
│             主循環 loop()                 │
│         （空循環，等待中斷/回呼）          │
└──────────────────────────────────────────┘
                      │
            （非同步回呼觸發）
                      ▼
┌──────────────────────────────────────────┐
│        ODPostProcess(results)            │
├──────────────────────────────────────────┤
│  取得影像寬高                            │
│  建立 OSD 位圖                           │
│                                          │
│  ┌─ 對每個偵測結果 ──────────────┐      │
│  │  檢查 type 是否為目標類別      │      │
│  │  檢查 filter 是否允許顯示     │      │
│  │  計算邊界框座標 (pixel)        │      │
│  │  OSD 繪製矩形 + 標籤文字      │      │
│  └──────────────────────────────┘      │
│                                          │
│  更新 OSD 畫面                           │
│                                          │
│  ┌─ 若有目標物件 ─────────────────┐      │
│  │  更新 NTP 時間                  │      │
│  │  產生時間戳字串                 │      │
│  │  ┌─ 若時間 0~6點 ────────┐    │      │
│  │  │  開啟 SD 卡檔案系統      │    │      │
│  │  │  從 JPEG 頻道擷取影像   │    │      │
│  │  │  寫入 SD 卡              │    │      │
│  │  │  關閉檔案、關閉檔案系統  │    │      │
│  │  └────────────────────────┘    │      │
│  │  重設旗標，延遲 5 秒           │      │
│  └────────────────────────────────┘      │
└──────────────────────────────────────────┘
```

### 關鍵技術要點
1. **多頻道影片架構**：同時處理 H.264 串流、RGB NN 輸入、JPEG 截圖三個獨立頻道
2. **StreamIO 管道模式**：使用資料流管道（StreamIO）連接攝影機 → 處理器 → 輸出，而非傳統輪詢
3. **回呼驅動**：NN 偵測結果透過回呼函式非同步傳遞，不阻塞主循環
4. **智慧儲存**：僅在深夜時段（0:00~6:00）自動存檔，減少 SD 卡寫入次數與功耗
5. **即時疊加**：透過 OSD 直接在影片畫面上繪製邊界框與標籤，無需後製

---

## 作業二：AI VibeCoding App

### 來源檔案
- **作業說明**：`AI-VibeCoding-app.md`
- **參考程式範例**：`Camera_2_Lcd_JPEGDEC`、`GenAIVisionTTS`

### 作業目標
在 AMB82-mini 上實作 AI VibeCoding 應用，功能包含**拍照 + AI Vision（視覺辨識）+ TTS（文字轉語音）+ LCD 顯示圖片或文字**。提供四種應用情境選擇。

### 應用方案技術細節

#### 方案一：AI 輔助回收物分類系統
```
Prompt: "請問這個回收物是什麼?請用中文回答"
```
- **技術流程**：攝影機拍攝回收物 → 上傳至 LLM/VLM 進行視覺辨識 → 辨識回收物種類 → TTS 以中文語音回答 → LCD 顯示分類結果
- **應用場景**：智慧回收桶、環保教育裝置
- **關鍵模組**：Camera + VLM API + TTS + LCD + 喇叭

#### 方案二：AI 輔助英語讀字卡造句
```
Prompt: "Just say the word in the picture?"
```
- **技術流程**：攝影機拍攝英文單字卡 → VLM 辨識單字 → LLM 產生例句 → TTS 朗讀單字與例句 → LCD 顯示文字
- **應用場景**：兒童英語學習輔具
- **關鍵模組**：Camera + VLM API + LLM API + TTS + LCD

#### 方案三：AI 看圖說故事
```
Prompt: "看這張圖 請用中文給我一個簡短的故事"
```
- **技術流程**：攝影機拍攝場景或圖片 → 上傳 VLM 分析影像內容 → LLM 生成中文故事 → TTS 朗讀故事 → LCD 顯示圖片與故事文字
- **應用場景**：兒童教育、視障輔助
- **關鍵模組**：Camera + VLM API + LLM API + TTS + LCD + SD 卡儲存

#### 方案四：AI 情緒感知音樂播放器
```
Prompt: "Analyze the emotion (happy, angry, sad, or joyful) of the person in the image..."
```
- **技術流程**：攝影機拍攝人臉 → VLM 分析情緒 → 根據情緒對應播放特定 MP3 歌曲 → TTS 告知情緒與推薦歌曲 → LCD 顯示情緒與曲目
- **情緒與歌曲對應表**：

| 情緒 | 歌曲 |
|------|------|
| Happy | APT.mp3 |
| Angry | BirdsOfAFeather.mp3 |
| Sad | ThePowerOfGoodBye.mp3 |
| Joyful | AstroBunny.mp3 |
| 其他 | gTTS.mp3, IBelieve.mp3, JarOfLove.mp3, LoversMisses.mp3, Stumblin_In.mp3, YUNGBLUD.mp3 |

- **應用場景**：情緒調適、音樂治療、互動藝術裝置
- **關鍵模組**：Camera + VLM API + LLM API + TTS + Audio Output + SD 卡儲存

### 共用技術架構

```
┌────────────────────────────────────────────┐
│                AMB82-mini                    │
│                                              │
│  ┌────────┐   ┌──────────┐   ┌──────────┐  │
│  │ Camera  │──▶│  VLM API  │──▶│  LLM API  │  │
│  │ (拍照)  │   │ (視覺辨識) │   │ (推理生成) │  │
│  └────────┘   └──────────┘   └─────┬────┘  │
│                                     │        │
│  ┌────────┐   ┌──────────┐         │        │
│  │  LCD    │◀──│  TTS API  │◀────────┘        │
│  │ (顯示)  │   │ (文字轉語音)│                  │
│  └────────┘   └─────┬────┘                   │
│                      │                        │
│  ┌────────┐         │                        │
│  │ 喇叭    │◀────────┘                        │
│  │ (語音)  │                                  │
│  └────────┘                                  │
└──────────────────────────────────────────────┘
```

### 流程圖說明

```
┌─────────────────────────────────────┐
│             開始                     │
├─────────────────────────────────────┤
│  初始化 WiFi 連線                    │
│  初始化 Camera                      │
│  初始化 LCD 顯示器                  │
│  初始化 TTS 引擎                    │
│  初始化 SD 卡檔案系統               │
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│     等待使用者觸發（按鈕/語音）     │
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│   Camera 拍攝照片                   │
│   儲存至 SD 卡暫存                  │
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│   LCD 顯示「處理中...」             │
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│   呼叫 VLM API 進行視覺辨識        │
│   （上傳圖片 + Prompt）             │
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│   VLM 回傳辨識結果                  │
│   （視應用而定：回收物/單字/描述/情緒）│
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│   依結果決定後續動作                │
│   ┌─ 回收分類：顯示分類資訊        │
│   ├─ 英語學習：產生例句            │
│   ├─ 看圖說故事：生成故事文字      │
│   └─ 情緒音樂：查表選曲            │
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│   LCD 顯示結果文字/圖片             │
│   呼叫 TTS API 生成語音            │
│   透過喇叭播放語音                 │
│   （情緒音樂則直接播放 MP3）       │
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│   回到等待觸發狀態                  │
└─────────────────────────────────────┘
```

### 關鍵技術要點
1. **多模態 AI 整合**：同時運用 VLM（視覺語言模型）與 LLM（大型語言模型），實現「看見→理解→生成→表達」完整鏈路
2. **串接雲端 API**：AMB82-mini 透過 WiFi 呼叫雲端 LLM/VLM/TTS API，彌補邊緣裝置運算能力不足
3. **本地端多媒體輸出**：結合 LCD 顯示與 Audio 喇叭輸出，實現雙通道（視覺+聽覺）人機互動
4. **Prompt Engineering**：各應用透過精心設計的提示詞（Prompt）引導 AI 產生精確回應
5. **模組化設計**：Camera → AI Vision → TTS → LCD/Audio 的管線架構可靈活組合不同應用

---

## 作業三：MPU6050 慣性感測模組 — 航向角計算

### 來源檔案
- **作業說明**：`IMU_MP6050.md`
- **程式碼**：`MPU6050_DMP6_GetHeading.ino`

### 作業目標
使用 MPU6050 六軸慣性感測模組（加速度計 + 陀螺儀），透過 DMP（Digital Motion Processor）硬體處理器計算航向角（Heading / Yaw），範圍 0°~360°。

### 程式技術細節

#### 使用的函式庫
| 函式庫 | 用途 |
|--------|------|
| `I2Cdev.h` | I2C 裝置通訊抽象層 |
| `MPU6050_6Axis_MotionApps612.h` | MPU6050 DMP 韌體 v6.12 驅動 |
| `Wire.h` | Arduino I2C 匯流排 |

#### 硬體接線
| MPU6050 接腳 | AMB82-mini 接腳 |
|-------------|-----------------|
| VCC | 3.3V |
| GND | GND |
| SDA | SDA (I2C) |
| SCL | SCL (I2C) |
| INT | （本實作未使用中斷腳） |

- I2C 位址：`0x68`（AD0 接地預設值）
- I2C 時脈：400kHz（快速模式）

#### 主要全域變數
```cpp
MPU6050 mpu;                    // MPU6050 物件
bool dmpReady = false;          // DMP 初始化成功旗標
uint8_t devStatus;              // 裝置狀態碼
uint16_t packetSize;            // DMP 封包大小（預設 42 bytes）
uint16_t fifoCount;             // FIFO 緩衝區計數
uint8_t fifoBuffer[128];        // FIFO 資料緩衝

// 姿態資料容器
Quaternion q;                   // 四元數 [w, x, y, z]
VectorFloat gravity;            // 重力向量
float ypr[3];                   // 尤拉角 [yaw, pitch, roll]
int Heading;                    // 航向角 0~360°
```

#### 初始化流程（`setup()`）
1. **I2C 初始化**：`Wire.begin()` 啟動 I2C 匯流排，設定時脈 400kHz
2. **序列埠初始化**：`Serial.begin(115200)`
3. **MPU6050 初始化**：
   - `mpu.initialize()` 初始化感測器
   - `mpu.testConnection()` 測試連線
4. **DMP 初始化**：
   - `mpu.dmpInitialize()` 載入 DMP 韌體
   - 設定自訂陀螺儀偏移量（X:51, Y:8, Z:21）
   - 設定自訂加速度計偏移量（X:1150, Y:-50, Z:1060）
   - 執行 6 次加速度計校準 `CalibrateAccel(6)`
   - 執行 6 次陀螺儀校準 `CalibrateGyro(6)`
   - 啟用 DMP `setDMPEnabled(true)`
   - 取得 DMP 封包大小
5. **設定 LED 輸出腳位**：`pinMode(LED_BUILTIN, OUTPUT)`

#### 主循環（`loop()`）
1. 檢查 `dmpReady` 旗標，若未就緒則返回
2. 從 FIFO 讀取 DMP 資料封包：`mpu.dmpGetCurrentFIFOPacket(fifoBuffer)`
3. 呼叫 `GetHeading(&Heading)` 計算航向角
4. 透過序列埠輸出 `Yaw: <數值>`
5. 切換 LED 狀態作為活動指示

#### 航向角計算函式（`GetHeading()`）
1. 讀取中斷狀態：`mpu.getIntStatus()`
2. 檢查 FIFO 溢位（`0x10` 旗標或計數=1024），若溢位則重設 FIFO
3. 檢查 DMP 資料就緒（`0x02` 旗標）：
   - 等待 FIFO 資料達到封包大小
   - 讀取 FIFO 資料到緩衝區
   - 更新 FIFO 計數
   - **四元數解算**：`mpu.dmpGetQuaternion(&q, fifoBuffer)`
   - **重力向量提取**：`mpu.dmpGetGravity(&gravity, &q)`
   - **尤拉角轉換**：`mpu.dmpGetYawPitchRoll(ypr, &q, &&gravity)`
   - **航向角換算**：`Heading = ypr[0] * 180/π + 180`（將 -180°~180° 映射至 0°~360°）

### 角度計算原理

```
DMP FIFO 資料
     │
     ▼
┌─────────────────────┐
│   dmpGetQuaternion   │──▶ 四元數 q[w, x, y, z]
└─────────┬───────────┘
          ▼
┌─────────────────────┐
│   dmpGetGravity      │──▶ 重力向量 gravity[x, y, z]
└─────────┬───────────┘
          ▼
┌─────────────────────┐
│  dmpGetYawPitchRoll  │──▶ 尤拉角 ypr[yaw, pitch, roll]（弧度）
└─────────┬───────────┘
          ▼
┌─────────────────────┐
│   yaw * 180/π + 180  │──▶ 航向角 0°~360°
└─────────────────────┘
```

### 流程圖說明

```
┌─────────────────────────────────────┐
│              開始                    │
├─────────────────────────────────────┤
│  I2C 初始化 (Wire.begin, 400kHz)    │
│  序列埠初始化 (115200)              │
│  MPU6050 initialize()               │
│  測試連線 testConnection()          │
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│  dmpInitialize() 載入 DMP 韌體      │
│  設定陀螺儀/加速度計偏移量          │
│  CalibrateAccel(6)                  │
│  CalibrateGyro(6)                   │
└──────────┬──────────────────────────┘
           ▼
     ┌─────┴─────┐
     │devStatus==0│
     ├─────┬──────┤
     │ 是  │  否  │
     └──┬──┴──────┘
        ▼          ▼
┌──────────┐  ┌──────────┐
│setDMP    │  │ 輸出錯誤 │
│Enabled() │  │  代碼     │
│dmpReady= │  │ dmpReady │
│true      │  │ =false   │
└────┬─────┘  └────┬─────┘
     │              │
     ▼              ▼
┌─────────────────────────────────────┐
│           主循環 loop()              │
├─────────────────────────────────────┤
│  if (!dmpReady) return              │
│  if (dmpGetCurrentFIFOPacket)       │
│   ├─ GetHeading(&Heading)           │
│   ├─ Serial.print(Yaw)             │
│   └─ digitalWrite(LED, toggle)      │
└─────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────┐
│       GetHeading(int *Heading)      │
├─────────────────────────────────────┤
│  讀取中斷狀態 getIntStatus()        │
│  取得 FIFO 計數 getFIFOCount()      │
│                                    │
│  ┌─ 檢查溢位 ────────────┐        │
│  │  (0x10 或 1024)        │        │
│  │  若溢位 → resetFIFO() │        │
│  └────────────────────────┘        │
│                                    │
│  ┌─ 檢查資料就緒 ─────────┐        │
│  │  (0x02 旗標)             │        │
│  │  等待 FIFO 資料充足     │        │
│  │  讀取 FIFO bytes        │        │
│  │  dmpGetQuaternion()     │        │
│  │  dmpGetGravity()        │        │
│  │  dmpGetYawPitchRoll()   │        │
│  │  Heading = yaw+180      │        │
│  └────────────────────────┘        │
└─────────────────────────────────────┘
```

### 關鍵技術要點
1. **DMP 硬體加速**：MPU6050 內建 DMP（Digital Motion Processor）專門處理慣性感測融合演算法，減輕主控 CPU 負擔
2. **四元數 → 尤拉角**：透過四元數避免 Gimbal Lock（萬向鎖），再轉換為直觀的 Yaw/Pitch/Roll
3. **偏移量校準**：每個感測器個體差異需設定自訂偏移量，並執行多次校準以獲得精確資料
4. **FIFO 佇列管理**：DMP 使用硬體 FIFO 儲存姿態資料，需妥善處理溢位與讀取時序
5. **角度映射**：原始 yaw 範圍 -180°~180°，透過 +180 轉換為 0°~360° 直觀表示

---

## 作業四：VL53L0X 紅外線測距感測器 + TFT LCD 顯示

### 來源檔案
- **作業說明**：`IRranger_TFT.md`
- **參考程式**：`AmebaWire / VL53L0X / Continuous` 與 `AmebaSPI / LCD_Screen_ILI9341_TFT`

### 作業目標
使用 VL53L0X 紅外線雷射測距感測器測量距離，並將測量結果顯示於 ILI9341 TFT LCD 螢幕上。需修改 VL53L0X 程式庫以使用 I2C1 匯流排（SDA1, SCL1）。

### 程式技術細節

#### 使用的函式庫
| 函式庫 | 用途 |
|--------|------|
| `AmebaWire.h` | AMB82-mini I2C 通訊（替代標準 Wire） |
| `VL53L0X.h` | ST 雷射測距感測器驅動 |
| `AmebaSPI.h` | AMB82-mini SPI 通訊 |
| `LCD_Screen_ILI9341_TFT.h` | ILI9341 TFT LCD 驅動 |

#### 硬體接線

**VL53L0X 紅外線測距模組（I2C1 匯流排）**
| VL53L0X 接腳 | AMB82-mini 接腳 |
|-------------|-----------------|
| VCC | 3.3V |
| GND | GND |
| SDA | SDA1 (I2C1) |
| SCL | SCL1 (I2C1) |

**ILI9341 TFT LCD（SPI 匯流排）**
| ILI9341 接腳 | AMB82-mini 接腳 |
|--------------|-----------------|
| VCC | 3.3V / 5V |
| GND | GND |
| CS | SPI CS |
| MOSI | SPI MOSI |
| SCK | SPI SCK |
| DC | GPIO |
| RST | GPIO |
| LED | 3.3V（背光）|

#### I2C1 匯流排配置

關鍵修改點：VL53L0X 預設使用 I2C0（`Wire`），需修改為 I2C1（`Wire1`）。

```cpp
// 在 setup() 中初始化 I2C1
Wire1.begin();        // 使用 SDA1/SCL1 腳位

// 修改 VL53L0X.cpp 程式庫檔案
// 將所有 Wire. 改為 Wire1.
// 檔案路徑：
// packages\realtek\hardware\AmebaPro2\4.1.1\
//   libraries\Wire\src\VL53L0X_IR_libraries\VL53L0X.cpp
```

#### 初始化流程
1. **I2C1 初始化**：`Wire1.begin()` 啟動 I2C1 匯流排
2. **VL53L0X 初始化**：
   - 設定感測器位址（預設 0x29）
   - 初始化感測器 `sensor.init()`
   - 設定連續測距模式 `sensor.startContinuous()`
3. **SPI 初始化**：啟動 SPI 通訊
4. **ILI9341 初始化**：
   - 初始化 LCD 控制器
   - 設定顯示方向與解析度（240×320）
   - 清屏
5. **主循環**：
   - 每 50ms 讀取一次距離 `sensor.readRangeContinuousMillimeters()`
   - 格式化距離字串（公釐 / 公分）
   - 更新 LCD 顯示

### 流程圖說明

```
┌──────────────────────────────────────┐
│                開始                    │
├──────────────────────────────────────┤
│  初始化 I2C1 匯流排 (Wire1.begin)    │
│  初始化 SPI 匯流排                    │
│                                      │
│  ┌─ VL53L0X 初始化 ───────────┐     │
│  │  sensor.init()              │     │
│  │  sensor.startContinuous()   │     │
│  └────────────────────────────┘     │
│                                      │
│  ┌─ ILI9341 LCD 初始化 ───────┐     │
│  │  LCD 控制器初始化            │     │
│  │  設定解析度 240×320         │     │
│  │  清屏                       │     │
│  │  顯示標題文字               │     │
│  └────────────────────────────┘     │
└──────────────────┬───────────────────┘
                   ▼
┌──────────────────────────────────────┐
│             主循環 loop()             │
├──────────────────────────────────────┤
│  ┌─ 每 50ms ─────────────────┐      │
│  │  讀取 VL53L0X 距離值       │      │
│  │  (readRangeContinuousMm)   │      │
│  │                            │      │
│  │  轉換為公分 (mm / 10)     │      │
│  │                            │      │
│  │  ┌─ 距離判斷 ──────┐     │      │
│  │  │ > 100cm → 綠色  │     │      │
│  │  │ 50~100cm → 黃色 │     │      │
│  │  │ < 50cm   → 紅色  │     │      │
│  │  └────────────────┘     │      │
│  │                            │      │
│  │  更新 LCD 顯示距離與顏色   │      │
│  └────────────────────────────┘      │
└──────────────────────────────────────┘
```

### 關鍵技術要點
1. **I2C1 匯流排切換**：AMB82-mini 提供兩組 I2C 匯流排，需修改 VL53L0X 程式庫原始碼將 `Wire` 取代為 `Wire1` 以使用 SDA1/SCL1
2. **ToF 雷射測距**：VL53L0X 採用 Time-of-Flight 技術，測距範圍 30mm~1200mm，精度 ±3%
3. **連續測量模式**：`startContinuous()` 啟動自動連續測距，內部以 50ms 間隔更新
4. **距離可視化**：根據距離遠近改變 LCD 顯示顏色（綠/黃/紅），提供直觀警示
5. **雙匯流排架構**：I2C1 用於感測器讀取，SPI 用於 LCD 更新，兩者獨立運作不互相干擾

---

## 作業五：Vibe Coding — SD 卡 HTML 網頁伺服器

### 來源檔案
- **作業說明**：`VibeCoding.md`
- **程式碼**：`VibeCoding_ReadHTMLFile.ino`

### 作業目標
在 Google AI Studio 或 ChatGPT 中自行設計 HTML 應用程式，將 HTML 檔案存入 AMB82-mini 的 SD 卡，透過 AMB82-mini 架設的 Wi-Fi 網頁伺服器提供手機瀏覽器存取。實現「Vibe Coding」— 用自然語言生成程式碼並部署於邊緣裝置。

### 程式技術細節

#### 使用的函式庫
| 函式庫 | 用途 |
|--------|------|
| `WiFi.h` | Wi-Fi 連線與 TCP 伺服器 |
| `AmebaFatFS.h` | SD 卡 FAT 檔案系統 |

#### 硬體設定
- **Wi-Fi 模式**：STA（Station），連線至現有無線網路
- **伺服器埠**：80（HTTP）
- **SD 卡 HTML 檔案**：`app-vibe_coding.html`

#### 主要全域變數
```cpp
char ssid[] = "Network_SSID";              // 網路 SSID（範例值）
char pass[] = "Password";                   // 網路密碼
int status = WL_IDLE_STATUS;                // Wi-Fi 狀態
WiFiServer server(80);                      // HTTP 伺服器（埠 80）
char filename_Web_test[] = "app-vibe_coding.html";  // SD 卡 HTML 檔名
AmebaFatFS fs;                              // FAT 檔案系統物件
```

> **注意**：作業中 SSID 與密碼為範例值 `Network_SSID` / `Password`，實際使用需修改為正確的無線網路憑證。

#### 初始化流程（`setup()`）
1. **序列埠初始化**：`Serial.begin(115200)`
2. **Wi-Fi 連線**：
   - 持續嘗試連線直到成功
   - `WiFi.begin(ssid, pass)` 連線至 WPA/WPA2 網路
   - 每次嘗試間隔 10 秒
3. **啟動 HTTP 伺服器**：`server.begin()` 開始監聽埠 80
4. **列印連線狀態**：`printWifiStatus()` 顯示 SSID、IP 位址、訊號強度

#### 主循環（`loop()`）
1. **監聽客戶端連線**：`server.available()` 等待瀏覽器連線
2. **HTTP 請求解析**：
   - 逐字元讀取 HTTP 請求
   - 以 `\n` 判斷行尾
   - 空白行表示 HTTP 請求結束
3. **HTTP 回應**：
   ```
   HTTP/1.1 200 OK
   Content-type:text/html
   ```
4. **SD 卡 HTML 檔案讀取**：
   - 掛載 SD 卡檔案系統 `fs.begin()`
   - 合成完整路徑 `fs.getRootPath() + filename_Web_test`
   - 開啟 HTML 檔案
   - 逐 byte 讀取並寫入 HTTP 回應串流
   - 關閉檔案與檔案系統
5. **關閉連線**

### 流程圖說明

```
┌────────────────────────────────────────┐
│                開始                      │
├────────────────────────────────────────┤
│  初始化序列埠 (115200)                  │
│                                        │
│  ┌─ Wi-Fi 連線迴圈 ────────────┐      │
│  │  WiFi.begin(ssid, pass)      │      │
│  │  等待 10 秒                  │      │
│  │  直到連線成功                │      │
│  └──────────────────────────────┘      │
│                                        │
│  server.begin() 啟動 HTTP 伺服器       │
│  列印 Wi-Fi 狀態 (SSID, IP, RSSI)      │
└────────────────┬───────────────────────┘
                 ▼
┌────────────────────────────────────────┐
│            主循環 loop()                │
├────────────────────────────────────────┤
│  client = server.available()           │
│                                        │
│  ┌─ 若有客戶端連線 ──────────┐        │
│  │  逐步讀取 HTTP 請求        │        │
│  │  以 currentLine 累積字元  │        │
│  │                            │        │
│  │  ┌─ 空白行判斷 ────┐     │        │
│  │  │ HTTP 請求結束     │     │        │
│  │  │ 回送 200 OK      │     │        │
│  │  │ Content-type:html│     │        │
│  │  │                  │     │        │
│  │  │ 掛載 SD 卡       │     │        │
│  │  │ 開啟 HTML 檔案   │     │        │
│  │  │ 逐 byte 寫入回應 │     │        │
│  │  │ 關閉檔案         │     │        │
│  │  │ 卸載檔案系統     │     │        │
│  │  └──────────────────┘     │        │
│  │                            │        │
│  │  關閉客戶端連線            │        │
│  └────────────────────────────┘        │
└────────────────────────────────────────┘
```

### 關鍵技術要點
1. **Vibe Coding 流程**：使用 AI（Google AI Studio / ChatGPT）以自然語言生成 HTML 應用，不需手寫程式碼
2. **SD 卡靜態託管**：HTML 檔案存放於 microSD 卡，AMB82-mini 作為輕量級靜態網頁伺服器
3. **逐 byte 串流**：為節省記憶體，不將整個 HTML 檔案載入 RAM，而是從 SD 卡逐 byte 讀取並直接寫入 HTTP 回應
4. **跨裝置存取**：任何連接相同 Wi-Fi 網路的裝置（手機、平板、筆電）皆可透過瀏覽器存取
5. **低資源佔用**：不需 SD 卡快取或動態內容生成，AMB82-mini 可同時處理多個應用

---

## 作業六：Visual Assistant 視覺輔助應用

### 來源檔案
- **作業說明**：`Visual_Assistant.md`
- **交付網址**：[app-Visual_Assistant](https://github.com/rkuo2000/app-visual_assistant)

### 作業目標
複製並修改 Visual Assistant 網頁應用程式，使其可透過 GitHub Pages 部署執行，並簡化使用介面以適合盲人使用，同時支援響應式網頁設計以在手機上執行。

### 技術細節

#### 專案來源
- **原始倉庫**：`https://github.com/rkuo2025/app-visual_assistant`
- **交付倉庫**：`https://github.com/rkuo2000/app-visual_assistant`
- **部署網址**：`https://rkuo2025.github.io/app-visual_assistant`

#### 核心功能
Visual Assistant 是一款以視覺障礙者為目標使用者的網頁應用，透過手機相機拍攝畫面，結合 AI 視覺辨識 API，以語音方式描述拍攝內容。

#### 技術架構
```
┌──────────────────────────────────┐
│         使用者手機瀏覽器           │
├──────────────────────────────────┤
│  ┌────────────────────────┐      │
│  │   響應式 UI (RWD)      │      │
│  │  - 大字體 / 高對比     │      │
│  │  - 語音操作引導        │      │
│  │  - 觸控手勢支援        │      │
│  └────────┬───────────────┘      │
│           │                       │
│  ┌────────▼───────────────┐      │
│  │  MediaPipe / Camera    │      │
│  │  (手機相機擷取畫面)    │      │
│  └────────┬───────────────┘      │
│           │                       │
│  ┌────────▼───────────────┐      │
│  │  AI Vision API         │      │
│  │  (VLM 影像辨識)        │      │
│  └────────┬───────────────┘      │
│           │                       │
│  ┌────────▼───────────────┐      │
│  │  SpeechSynthesis       │      │
│  │  (Web TTS 語音朗讀)    │      │
│  └────────────────────────┘      │
└──────────────────────────────────┘
```

#### 響應式設計要點
| 裝置 | 佈局 | 字體 | 操作方式 |
|------|------|------|----------|
| 手機（直式） | 單欄全屏 | 加大字體（≥20px） | 觸控 + 語音 |
| 手機（橫式） | 單欄 | 大字体 | 觸控 |
| 平板 | 雙欄（預覽+結果） | 標準 | 觸控 + 鍵盤 |
| 桌機 | 雙欄 | 標準 | 鍵盤 + 滑鼠 |

#### 無障礙設計（Accessibility）
1. **高對比度色彩**：文字與背景對比度符合 WCAG AA 標準
2. **螢幕閱讀器支援**：使用 ARIA 標籤與語義化 HTML
3. **語音輸出**：透過 Web Speech API（SpeechSynthesis）將辨識結果轉為語音
4. **大按鈕**：觸控目標 ≥ 48×48dp
5. **簡化流程**：開啟即用，最少步驟即可完成拍照→辨識→語音

### 流程圖說明

```
┌─────────────────────────────────────┐
│          使用者開啟網頁               │
├─────────────────────────────────────┤
│  載入 PWA 應用程式主畫面             │
│  檢查裝置相機權限                    │
│  若未授權 → 請求相機權限            │
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│        主畫面（單鍵操作）            │
│  ┌─────────────────────────┐        │
│  │  [ 拍照辨識 ]  (大按鈕) │        │
│  │  或點擊畫面任意處       │        │
│  └─────────────────────────┘        │
└──────────┬──────────────────────────┘
           ▼ （觸發拍照）
┌─────────────────────────────────────┐
│  啟動手機相機拍攝畫面               │
│  顯示「正在辨識...」進度提示        │
│  （可選震動回饋）                   │
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│  呼叫 AI Vision API                 │
│  上傳影像進行 VLM 分析              │
│  等待 API 回傳辨識結果              │
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│  以大字體顯示辨識結果文字           │
│  自動啟動 SpeechSynthesis 朗讀     │
│  支援點擊結果重複朗讀              │
└──────────┬──────────────────────────┘
           ▼
┌─────────────────────────────────────┐
│  回到主畫面，等待下一次拍照        │
└─────────────────────────────────────┘
```

### 關鍵技術要點
1. **PWA 架構**：可安裝為手機應用，支援離線快取與背景執行
2. **Web Speech API**：使用瀏覽器內建 TTS，不需額外 API 金鑰
3. **GitHub Pages 部署**：靜態網頁直接託管於 GitHub Pages，零伺服器成本
4. **RWD 響應式設計**：CSS Media Query 與 viewport 設定確保所有裝置可用
5. **無障礙優先**：以視障者需求為核心設計 UI/UX，而非一般使用者為基礎再行調適

---

## 作業七：Web Server 雙 LED 控制

### 來源檔案
- **作業說明**：`WebServer_ControlLEDx2.md`
- **程式碼**：`WebServer_ControlLED.ino`

### 作業目標
將原始單一 LED 控制網頁伺服器程式上傳至 ChatGPT 或 Gemini，要求 AI 修改為支援兩個按鈕分別控制 LED_B（內建 LED）與 LED_G（GPIO 27）的雙 LED 控制版本。

### 程式技術細節

#### 使用的函式庫
| 函式庫 | 用途 |
|--------|------|
| `WiFi.h` | Wi-Fi 連線與 TCP 伺服器 |

#### 硬體接線
| 元件 | AMB82-mini 接腳 |
|------|-----------------|
| LED_B（藍色） | LED_BUILTIN（內建） |
| LED_G（綠色） | GPIO 27 |

#### 主要全域變數
```cpp
char ssid[] = "TCFSTWIFI.ALL";        // Wi-Fi SSID
char pass[] = "035623116";            // Wi-Fi 密碼
int status = WL_IDLE_STATUS;           // 連線狀態
WiFiServer server(80);                 // HTTP 伺服器

#define LED_PIN_LED_B LED_BUILTIN      // 藍色 LED（內建）
#define LED_PIN_LED_G 27               // 綠色 LED（GPIO 27）
String buttonState_B = "off";          // LED_B 狀態
String buttonState_G = "off";          // LED_G 狀態
```

#### 初始化流程（`setup()`）
1. **序列埠初始化**：`Serial.begin(115200)`
2. **GPIO 設定**：設定 `LED_PIN_LED_B` 與 `LED_PIN_LED_G` 為輸出模式
3. **Wi-Fi 連線**：等待連線至指定 SSID
4. **伺服器啟動**：`server.begin()`
5. **列印狀態**：顯示 SSID、IP 位址、訊號強度

#### 主循環（`loop()`）
1. **監聽客戶端**：`server.available()`
2. **HTTP 請求解析**：
   - 逐字元讀取 HTTP 請求
   - `\n` 判斷行尾
   - 空白行表示請求結束，準備回應
3. **HTML 頁面生成**（內嵌於程式碼）：
   - DOCTYPE html 宣告
   - viewport meta 設定（RWD）
   - CSS 樣式（綠色按鈕 = ON，灰色按鈕 = OFF，圓角設計）
   - 顯示目前 LED 狀態
   - 根據狀態顯示 ON/OFF 超連結按鈕
   - **（待修改）** 目前版本僅支援單一 LED 控制
4. **HTTP 請求處理**：
   - `GET /H` → 點亮 LED
   - `GET /L` → 熄滅 LED

> **注意**：現有程式碼為原始版本，僅支援單一 LED 控制。作業要求透過 AI 修改為雙按鈕版本，分別控制 LED_B 與 LED_G。

### 流程圖說明

```
┌────────────────────────────────────────┐
│                開始                      │
├────────────────────────────────────────┤
│  初始化序列埠 (115200)                  │
│  設定 LED_B(LED_BUILTIN) 為輸出        │
│  設定 LED_G(GPIO 27) 為輸出            │
│                                        │
│  ┌─ Wi-Fi 連線迴圈 ───────────┐       │
│  │  WiFi.begin(ssid, pass)     │       │
│  │  等待 10 秒                 │       │
│  │  直到連線成功               │       │
│  └─────────────────────────────┘       │
│                                        │
│  server.begin() 啟動 HTTP 伺服器       │
│  列印 Wi-Fi 狀態                       │
└────────────────┬───────────────────────┘
                 ▼
┌────────────────────────────────────────┐
│            主循環 loop()                │
├────────────────────────────────────────┤
│  client = server.available()           │
│                                        │
│  ┌─ 若有客戶端連線 ──────────┐        │
│  │  讀取 HTTP 請求            │        │
│  │  以 currentLine 累積      │        │
│  │                            │        │
│  │  ┌─ 空白行判斷 ────┐     │        │
│  │  │ 回傳 HTML 頁面    │     │        │
│  │  │ (內嵌 CSS 樣式)   │     │        │
│  │  │ LED 狀態按鈕      │     │        │
│  │  └──────────────────┘     │        │
│  │                            │        │
│  │  ┌─ 請求處理 ──────┐     │        │
│  │  │ GET /H → LED ON │     │        │
│  │  │ GET /L → LED OFF│     │        │
│  │  └──────────────────┘     │        │
│  │                            │        │
│  │  關閉客戶端連線            │        │
│  └────────────────────────────┘        │
└────────────────────────────────────────┘
```

### AI 修改版本預期成果

透過 ChatGPT 或 Gemini 修改後，預期新增功能：

1. **雙 LED 狀態變數**：獨立追蹤 `buttonState_B` 與 `buttonState_G`
2. **雙按鈕網頁 UI**：頁面分為 LED_B 與 LED_G 兩個控制區塊
3. **四條路由**：
   - `GET /HB` → LED_B 開啟
   - `GET /LB` → LED_B 關閉
   - `GET /HG` → LED_G 開啟
   - `GET /LG` → LED_G 關閉
4. **狀態指示**：兩個 LED 各自獨立顯示 ON/OFF 狀態

### 關鍵技術要點
1. **AI 輔助開發**：展現使用 ChatGPT/Gemini 進行程式碼修改的 AI-assisted programming 流程
2. **嵌入式 HTTP 伺服器**：AMB82-mini 直接作為 Web Server，不需額外雲端伺服器
3. **動態 HTML 生成**：透過 `client.println()` 逐行輸出 HTML，根據 LED 狀態動態變更內容
4. **RWD 支援**：透過 viewport meta 與 CSS 確保手機瀏覽器正確顯示
5. **邊緣控制**：使用者可從任何瀏覽器直接控制 AMB82-mini 的 GPIO 輸出

---

## 作業八：Web Server DHT11 溫濕度感測

### 來源檔案
- **作業說明**：`WebServer_DHT11.md`
- **程式碼**：`WebServer_DHT11.ino`

### 作業目標
修改網頁伺服器程式，從 DHT11 溫濕度感測器讀取溫度與濕度數據，並在 HTTP 回應中顯示。

### 程式技術細節

#### 使用的函式庫
| 函式庫 | 用途 |
|--------|------|
| `WiFi.h` | Wi-Fi 連線與 TCP 伺服器 |
| `DHT.h` | DHT11 溫濕度感測器驅動 |

#### 硬體接線
| DHT11 接腳 | AMB82-mini 接腳 |
|-----------|-----------------|
| VCC | 3.3V / 5V |
| GND | GND |
| DATA | GPIO 8 |
| NC | 不接 |

**重要**：DHT11 的 DATA 腳需外接 4.7kΩ~10kΩ 上拉電阻至 VCC。

#### 主要全域變數
```cpp
#define DHTPIN 8                   // DHT11 資料腳位
#define DHTTYPE DHT11              // 感測器類型
DHT dht(DHTPIN, DHTTYPE);         // DHT 物件

char ssid[] = "TCFSTWIFI.ALL";    // Wi-Fi SSID
char pass[] = "035623116";        // Wi-Fi 密碼
WiFiServer server(80);             // HTTP 伺服器
```

#### 初始化流程（`setup()`）
1. **序列埠初始化**：`Serial.begin(115200)`，等待序列埠連線
2. **Wi-Fi 連線**：
   - 持續嘗試連線至指定 SSID
   - 每次嘗試間隔 10 秒
3. **DHT11 初始化**：`dht.begin()` 啟動感測器
4. **HTTP 伺服器啟動**：`server.begin()`（使用非阻塞模式）
5. **列印狀態**：顯示 SSID、IP 位址、訊號強度

#### 主循環（`loop()`）
1. **監聽客戶端**：`server.available()`
2. **讀取感測器資料**：
   - `dht.readHumidity()` 讀取濕度（%）
   - `dht.readTemperature()` 讀取溫度（°C）
   - *注意：感測器資料可能延遲 2 秒更新*
3. **HTTP 請求解析**：逐字元讀取，以 `\n` 判斷行尾
4. **HTTP 回應**（收到空白行時）：
   ```
   HTTP/1.1 200 OK
   Content-Type: text/html
   Connection: close
   Content-Length: <長度>

   Temperature: 25.5C <br />
   Humidity:    60.0%  <br />
   </body></html>
   ```
5. **非阻塞模式**：不呼叫 `client.stop()`，由解構子自動關閉
6. **使用者程式碼**：回應傳送後可繼續執行其他任務，每 5 秒循環

### 流程圖說明

```
┌────────────────────────────────────────┐
│                開始                      │
├────────────────────────────────────────┤
│  初始化序列埠 (115200)                  │
│                                        │
│  ┌─ Wi-Fi 連線迴圈 ───────────┐       │
│  │  WiFi.begin(ssid, pass)     │       │
│  │  等待 10 秒                 │       │
│  │  直到連線成功               │       │
│  └─────────────────────────────┘       │
│                                        │
│  dht.begin() 初始化 DHT11 感測器       │
│  server.begin() 啟動 HTTP 伺服器       │
│  列印 Wi-Fi 狀態                       │
└────────────────┬───────────────────────┘
                 ▼
┌────────────────────────────────────────┐
│            主循環 loop()                │
├────────────────────────────────────────┤
│  client = server.available()           │
│                                        │
│  ┌─ 若有客戶端連線 ──────────┐        │
│  │  讀取 DHT11 溫濕度         │        │
│  │  (readHumidity /           │        │
│  │   readTemperature)        │        │
│  │                            │        │
│  │  逐步讀取 HTTP 請求        │        │
│  │  累積至 currentLine        │        │
│  │                            │        │
│  │  ┌─ 空白行判斷 ────┐     │        │
│  │  │ 組合 HTML 回應    │     │        │
│  │  │ Temperature: x C  │     │        │
│  │  │ Humidity:    y%   │     │        │
│  │  │ 設定 Content-    │     │        │
│  │  │ Length 標頭      │     │        │
│  │  │ 發送 HTTP 回應   │     │        │
│  │  └──────────────────┘     │        │
│  │                            │        │
│  │  延遲 1ms (等待傳送完成)   │        │
│  └────────────────────────────┘        │
│                                        │
│  使用者程式碼區段                      │
│  Serial.println("User code...")        │
│  delay(5000)                           │
└────────────────────────────────────────┘
```

### HTTP 回應結構

```
HTTP/1.1 200 OK
Content-Type: text/html
Connection: close
Content-Length: 46

Temperature: 25.50C <br />
Humidity:    60.00%  <br />
</body></html>
```

**Content-Length 計算**：動態計算 HTML 內容長度，確保 HTTP 協定正確。

### 關鍵技術要點
1. **Content-Length 動態計算**：使用 `String(htmlContent.length())` 確保 HTTP 回應長度精確，避免瀏覽器等待逾時
2. **非阻塞伺服器模式**：AMB82-mini 的 WiFiServer 支援非阻塞模式，`client.stop()` 不需要手動呼叫
3. **感測器延遲處理**：DHT11 為慢速感測器（取樣率 1Hz），每次讀取可能回傳 2 秒前的舊資料
4. **字串串接回應**：使用 `String` 類別的 `+` 運算子組合 HTML 內容，簡潔易懂
5. **內嵌溫濕度監控**：無需額外的 IoT 平台，直接透過網頁瀏覽器即可查看即時環境數據

---








