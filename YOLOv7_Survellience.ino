/*
 Example guide:
 https://ameba-doc-arduino-sdk.readthedocs-hosted.com/en/latest/ameba_pro2/amb82-mini/Example_Guides/Neural%20Network/Object%20Detection%20results%20save%20on%20SD%20Card.html

 NN Model Selection
 Select Neural Network(NN) task and models using .modelSelect(nntask, objdetmodel, facedetmodel, facerecogmodel).
 Replace with NA_MODEL if they are not necessary for your selected NN Task.

 NN task
 =======
 OBJECT_DETECTION/ FACE_DETECTION/ FACE_RECOGNITION

 Models
 =======
 YOLOv3 model         DEFAULT_YOLOV3TINY   / CUSTOMIZED_YOLOV3TINY
 YOLOv4 model         DEFAULT_YOLOV4TINY   / CUSTOMIZED_YOLOV4TINY
 YOLOv7 model         DEFAULT_YOLOV7TINY   / CUSTOMIZED_YOLOV7TINY
 SCRFD model          DEFAULT_SCRFD        / CUSTOMIZED_SCRFD
 MobileFaceNet model  DEFAULT_MOBILEFACENET/ CUSTOMIZED_MOBILEFACENET
 No model             NA_MODEL
 */

#include "StreamIO.h"
#include "VideoStream.h"
#include "NNObjectDetection.h"
#include "VideoStreamOverlay.h"
#include "ObjectClassList.h"
#include "AmebaFatFS.h"
#include "WebSocketViewer.h"
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define CHANNEL     0
#define CHANNELJPEG 1
#define CHANNELNN   3

// Lower resolution for NN processing
#define NNWIDTH  576
#define NNHEIGHT 320

uint32_t img_addr = 0;
uint32_t img_len = 0;
uint32_t count = 0;

AmebaFatFS fs;

VideoSetting config(VIDEO_FHD, 30, VIDEO_H264, 0);
VideoSetting configNN(NNWIDTH, NNHEIGHT, 10, VIDEO_RGB, 0);
VideoSetting configJPEG(VIDEO_FHD, CAM_FPS, VIDEO_JPEG, 1);
NNObjectDetection ObjDet;
StreamIO videoStreamer(1, 1);
StreamIO videoStreamerNN(1, 1);
WebSocketViewer ws_viewer;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "tw.pool.ntp.org", 28800, 60000);

char path[128];
char ssid[] = "TCFSTWIFI.ALL";    // your network SSID (name)
char pass[] = "035623116";        // your network password
int status = WL_IDLE_STATUS;

IPAddress ip;
//int rtsp_portnum;
bool objects_detected_flag = false;

void setup()
{
    Serial.begin(115200);

    // attempt to connect to Wifi network:
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);

        // wait 2 seconds for connection:
        delay(2000);
    }
    ip = WiFi.localIP();

    timeClient.begin();

    ws_viewer.loadWebResources(0);
    config.enableWebsocketViewer();

    // Configure camera video channels with video format information
    // Adjust the bitrate based on your WiFi network quality
    config.setBitrate(2 * 1024 * 1024);    // Recommend to use 2Mbps for RTSP streaming to prevent network congestion
    Camera.configVideoChannel(CHANNEL, config);
    Camera.configVideoChannel(CHANNELNN, configNN);
    Camera.configVideoChannel(CHANNELJPEG, configJPEG);
    Camera.videoInit();

    ws_viewer.init();
    // Configure RTSP with corresponding video format information
    //rtsp.configVideo(config);
    //rtsp.begin();
    //rtsp_portnum = rtsp.getPort();

    // Configure object detection with corresponding video format information
    // Select Neural Network(NN) task and models
    ObjDet.configVideo(configNN);
    ObjDet.setResultCallback(ODPostProcess);
    ObjDet.modelSelect(OBJECT_DETECTION, DEFAULT_YOLOV7TINY, NA_MODEL, NA_MODEL);
    ObjDet.begin();

    // Configure StreamIO object to stream data from video channel to RTSP
    videoStreamer.registerInput(Camera.getStream(CHANNEL));
    videoStreamer.registerOutput(ws_viewer);
    if (videoStreamer.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }

    // Start data stream from video channel
    Camera.channelBegin(CHANNEL);
    Camera.channelBegin(CHANNELJPEG);

    // Configure StreamIO object to stream data from RGB video channel to object detection
    videoStreamerNN.registerInput(Camera.getStream(CHANNELNN));
    videoStreamerNN.setStackSize();
    videoStreamerNN.setTaskPriority();
    videoStreamerNN.registerOutput(ObjDet);
    if (videoStreamerNN.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }

    // Start video channel for NN
    Camera.channelBegin(CHANNELNN);
    ws_viewer.begin();

    // Start OSD drawing on RTSP video channel
    // OSD.configVideo(CHANNEL, config);
    // OSD.begin();

    // Start OSD drawing on JPEG video channel
    OSD.configVideo(CHANNEL, config);
    OSD.configVideo(CHANNELJPEG, configJPEG);
    OSD.begin();
}

void loop()
{
    // Do nothing
}

// User callback function for post processing of object detection results
void ODPostProcess(std::vector<ObjectDetectionResult> results)
{
    uint16_t im_h = config.height();
    uint16_t im_w = config.width();

    printf("Total number of objects detected = %d\r\n", ObjDet.getResultCount());
    OSD.createBitmap(CHANNEL);
    OSD.createBitmap(CHANNELJPEG);

    if (ObjDet.getResultCount() > 0) {
        for (int i = 0; i < ObjDet.getResultCount(); i++) {
            int obj_type = results[i].type();
            if (obj_type<4 || obj_type==5 || obj_type==7) // person, bicycle, car, motorbike, bus, truck
                objects_detected_flag = true; 
            if (itemList[obj_type].filter) {    // check if item should be ignored
                ObjectDetectionResult item = results[i];
                // Result coordinates are floats ranging from 0.00 to 1.00
                // Multiply with RTSP resolution to get coordinates in pixels
                int xmin = (int)(item.xMin() * im_w);
                int xmax = (int)(item.xMax() * im_w);
                int ymin = (int)(item.yMin() * im_h);
                int ymax = (int)(item.yMax() * im_h);

                // Draw boundary box
                printf("Item %d %s:\t%d %d %d %d\n\r", i, itemList[obj_type].objectName, xmin, xmax, ymin, ymax);
                OSD.drawRect(CHANNEL, xmin, ymin, xmax, ymax, 3, OSD_COLOR_WHITE);
                OSD.drawRect(CHANNELJPEG, xmin, ymin, xmax, ymax, 3, OSD_COLOR_WHITE);

                // Print identification text
                char text_str[20];
                snprintf(text_str, sizeof(text_str), "%s %d", itemList[obj_type].objectName, item.score());
                OSD.drawText(CHANNEL, xmin, ymin - OSD.getTextHeight(CHANNEL), text_str, OSD_COLOR_CYAN);
                OSD.drawText(CHANNELJPEG, xmin, ymin - OSD.getTextHeight(CHANNEL), text_str, OSD_COLOR_CYAN);
            }
        }
    }
    OSD.update(CHANNEL);
    OSD.update(CHANNELJPEG);

    if (objects_detected_flag) {

        timeClient.update();
        uint16_t year = (uint16_t)timeClient.getYear();
        uint16_t month = (uint16_t)timeClient.getMonth();
        uint16_t date = (uint16_t)timeClient.getMonthDay();
        uint16_t hour = (uint16_t)timeClient.getHours();
        uint16_t minute = (uint16_t)timeClient.getMinutes();
        uint16_t second = (uint16_t)timeClient.getSeconds();
        String timestamp = String(year)+"_"+String(month)+"_"+String(date)+"_"+String(hour)+"_"+String(minute)+"_"+String(second);

        if(hour>=0 && hour<=6) {
            fs.begin();
            File file = fs.open(String(fs.getRootPath()) + "ObjDet_" + timestamp + ".jpg");            
            delay(1000);
            Camera.getImage(CHANNELJPEG, &img_addr, &img_len);
            file.write((uint8_t *)img_addr, img_len);
            printf("Saved %s\r\n", file.name());
            file.close();
            fs.end();
            count++;
        }
        objects_detected_flag = false;
        delay(5000);
    }
}
