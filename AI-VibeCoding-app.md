Homework: AI VibeCoding App for AMB82-mini
實作以上任一個應用(或自創應用)，功能包含拍照+AI Vision + TTS + LCD顯示圖片或文字

Code Examples:
* Camera_2_Lcd_JPEGDEC (Camera to LCD with JPEG decoder)
* GenAIVisionTTS (Camera to LLM + TTS)

Delivery of Applications:
1. AI 輔助回收物分類系統 
```
String prompt_msg = "請問這個回收物是什麼?請用中文回答";
```
2. AI輔助英語讀字卡造句
```
String prompt_msg_img = "Just say the word in the picture?";
```
3. AI看圖說故事
```
String prompt_msg = "看這張圖 請用中文給我一個簡短的故事";
```
4. AI情緒感知音樂播放器
```
String prompt_msg = "Analyze the emotion (happy, angry, sad, or joyful) of the person in the image. Based on the detected emotion, recommend a suitable song filename from the following list. Respond in the exact format: 'Emotion: [emotion], Song: [filename.mp3]'. Emotion mapping: Happy: APT.mp3. Angry: BirdsOfAFeather.mp3. Sad: ThePowerOfGoodBye.mp3. Joyful: AstroBunny.mp3. Other songs available: gTTS.mp3, IBelieve.mp3, JarOfLove.mp3, LoversMisses.mp3, Stumblin_In.mp3, YUNGBLUD.mp3.";
```
