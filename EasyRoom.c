#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <softTone.h>
#include <softPwm.h>
#include <lcd.h>
#include <unistd.h>

typedef unsigned char UCHAR;
typedef unsigned int UINT;

#define SERVO 21

// Button
#define KEY_1 2
#define KEY_2 3
#define KEY_3 4
#define KEY_4 17
#define MAX_KEY_BT_NUM 4

// Text LCD
#define RS 18
#define STRB 23
#define LCD_PIN_D4 8
#define LCD_PIN_D5 7
#define LCD_PIN_D6 24
#define LCD_PIN_D7 25
#define MAX_MENU_NUM 5

// DHT11
#define MAXTIMINGS 83
#define DHTPIN 13

// Buzzer
#define BUZZER_PIN 26

// DC Motor
#define MOTOR_MT_P_PIN 27 // MOTOR포트 MT_P핀 정의
#define MOTOR_MT_N_PIN 22 // MOTOR포트 MT_N핀 정의

#define WINDOW_SIZE 1000 * 3 // 창문 사이즈에 따라 delay시간 조정
#define OPEN_WINDOW 2        // right rotate
#define CLOSE_WINDOW 1       // left rotate

// MCP3208
#define CS_MCP3208 8      // MCP3208핀 정의
#define SPI_CHANNEL 0     // SPI 통신 채널 정의
#define SPI_SPEED 1000000 // SPI 통신 속도 정의
#define DUST_LED 19

#define SERVO_PIN 6

#define DO_LOW 523
#define RE 587
#define MI 659
#define FA 698
#define SOL 784
#define RA 880
#define SI 987
#define DO_HIGH 1046

int dht11_dat[5] = {
    0,
};

// Key1 = 위로, key2 = 확인, key3 = 아래로, key4 = 뒤로가기
const int KeyTable[4] = {KEY_1, KEY_2, KEY_3, KEY_4};
void playSound(int soundOn); // 프로토타입
// 버튼을 입력으로 정수값을 리턴하는 함수
int KeypadRead(void)
{

    int i, nKeypadstate;

    nKeypadstate = 0;

    for (i = 0; i < MAX_KEY_BT_NUM; i++)
    {

        if (!digitalRead(KeyTable[i]))
        {
            nKeypadstate |= (1 << i);
        }
    }
    return nKeypadstate;
}
// 버튼으로 입력받아 알람으로 설정할 시간을 변경하는 함수
char *updateTime(char *timeValue, int nKeypadstate)
{
    static int minuteMode = 0; // minuteMode를 정적 변수로 변경하여 함수 호출 사이에 유지
    int hours, minutes;
    char hourStr[3], minuteStr[3]; // HH값과 MM값 저장할 변수선언

    strncpy(hourStr, timeValue, 2);
    hourStr[2] = '\0';
    hours = atoi(hourStr);

    strncpy(minuteStr, timeValue + 3, 2);
    minuteStr[2] = '\0';
    minutes = atoi(minuteStr);

    switch (nKeypadstate)
    {
    case 1: // 1번버튼 눌렀을 때 minuteMode가 0이면 hours값을 더하고 1이면 minutes값을 더 해준다.
        if (minuteMode == 0)
        {
            hours++;
            if (hours == 24)
            { // hours값이 24가 되면 0
                hours = 0;
            }
        }
        else if (minuteMode == 1)
        {
            minutes++;
            if (minutes == 60)
            { // minutes값이 60이 되면 0
                minutes = 0;
            }
        }
        break;
    case 4: // 4번버튼을 눌렀을 때 minuteMode가 0이면 hours값을 빼고 1이면 minutes값을 빼준다.
        if (minuteMode == 0)
        {
            hours--;
            if (hours == -1)
            { // hours값이 -1이 되면 23
                hours = 23;
            }
        }
        else if (minuteMode == 1)
        {
            minutes--;
            if (minutes == -1)
            { // minutes값이 -1이 되면 59
                minutes = 59;
            }
        }
        break;
    case 2: // 2번 버튼을 눌러 minutesMode를 토글방식으로 변경하여 hours값을 변경할지 minutes값을 변경할지 선택
        minuteMode = !minuteMode;
        break;
    default: // 예상치 못한 입력에 대하여 유효하지 않은 버튼 메세지 출력
        printf("유효하지 않은 버튼입니다.\n");
        break;
    }

    snprintf(timeValue, 6 + 1, "%02d:%02d", hours, minutes); // 버튼을 입력을 통해 변경한 시간값을 timeValue에 저장
    return timeValue;                                        // timeValue리턴
}

// 온습도 read하는 함수
void read_dht11_dat(int disp)
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, LOW);
    delay(18);

    digitalWrite(DHTPIN, HIGH);
    delayMicroseconds(30);
    pinMode(DHTPIN, INPUT);

    for (i = 0; i < MAXTIMINGS; i++)
    {
        counter = 0;
        while (digitalRead(DHTPIN) == laststate)
        {
            counter++;
            delayMicroseconds(1);
            if (counter == 200)
                break;
        }
        laststate = digitalRead(DHTPIN);

        if (counter == 200)
            break;
        if ((i >= 4) && (i % 2 == 0))
        {
            dht11_dat[j / 8] <<= 1;
            if (counter > 20)
                dht11_dat[j / 8] |= 1;
            j++;
        }
    }
    if ((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xff)))
    {
        // printf("humidity = %d.%d && Temperature = %d.%d *C \n", dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
        lcdClear(disp);
        lcdPosition(disp, 0, 0);
        lcdPrintf(disp, "Temp : %d.%d *C", dht11_dat[2], dht11_dat[3]);
        lcdPosition(disp, 0, 1);
        lcdPrintf(disp, "Hum : %d.%d %%", dht11_dat[0], dht11_dat[1]);
    }
    else
    {
        lcdClear(disp);
        lcdPosition(disp, 0, 0);
        lcdPrintf(disp, "Temp : %d.%d *C", dht11_dat[2], dht11_dat[3]);
        lcdPosition(disp, 0, 1);
        lcdPrintf(disp, "Hum : %d.%d %%", dht11_dat[0], dht11_dat[1]);
    }
}
UINT SevenScale(UCHAR scale)
{
    UINT _ret = 0;
    switch (scale)
    {
    case 0:
        _ret = DO_LOW;
        break;
    case 1:
        _ret = RE;
        break;
    case 2:
        _ret = MI;
        break;
    case 3:
        _ret = FA;
        break;
    case 4:
        _ret = SOL;
        break;
    case 5:
        _ret = RA;
        break;
    case 6:
        _ret = SI;
        break;
    case 7:
        _ret = DO_HIGH;
        break;
    }
    return _ret;
}
void Change_FREQ(UINT freq)
{
    softToneWrite(BUZZER_PIN, freq);
}
void STOP_FREQ(void)
{
    softToneWrite(BUZZER_PIN, 0);
}
void DoorOpen()
{
    // SERVO 작동으로 문 OPEN
    for (int i = 0; i < 7; i += 2)
    {
        Change_FREQ(SevenScale(i));
        delay(300);
        STOP_FREQ();
    }

    printf("Door open!\n"); // Edit Text LCD
    softPwmWrite(SERVO_PIN, 25);

    delay(2000);

    // SERVO 작동으로 문 CLOSE
    softPwmWrite(SERVO_PIN, 15);
    printf("Door close!\n");
    STOP_FREQ();
    for (int i = 6; i >= 0; i -= 2)
    {
        Change_FREQ(SevenScale(i));
        delay(300);
        STOP_FREQ();
    }
    delay(2000);
}
// 설정한 알람시간과 현재시간이 같을 때 알람을 울리는 함수
void SetAlarm(int disp, char *timevalue, char *currentTime, int melody)
{
    // textLCD화면
    lcdClear(disp);
    lcdPosition(disp, 0, 0);
    lcdPuts(disp, "Setting Alarm");
    lcdPosition(disp, 0, 1);
    lcdPuts(disp, timevalue); // 설정한 시간값을 출력해줌

    if (strcmp(currentTime, timevalue) == 0)
    { // 현재시간과 설정한 시간이 같을 때 알람 울림
        playSound(melody);
    }
}
// 매개변수로 들어간 melody값에 따라서 알람 소리 종류를 설정하는 함수
void SetSound(int disp, int melody)
{
    char melodyString[20];               // melody를 저장할 문자열 배열 생성
    sprintf(melodyString, "%d", melody); // 정수 melody를 문자열로 변환하여 melodyString에 저장
    // textLCD화면
    lcdClear(disp);
    lcdPosition(disp, 0, 0);
    lcdPuts(disp, "Alarm Melody");
    lcdPosition(disp, 0, 1);
    lcdPuts(disp, melodyString);
}
void BUZZER_Init(void)
{
    softToneCreate(BUZZER_PIN);
    STOP_FREQ;
}
// 알람을 울리는 함수
void playSound(int soundOn)
{
    softToneCreate(BUZZER_PIN);
    if (soundOn == 1) // 삐삐삐삐
    {
        int melody[5] = {1174, 1174, 1174, 1174, 0}; // softToneWrite()에 들어가 음계를 바꾸는 배열
        int rhythm[5] = {80, 80, 80, 80, 2000};      // delay()에 들어가 리듬을 조절하는 배열

        for (int i = 0; i < 5; i++) // melody와 rhythm의 변화에 따라 buzzer가 울림
        {
            softToneWrite(BUZZER_PIN, melody[i]);
            delay(rhythm[i]);

            for (int j = 0; j < 5; j++) // 음계 간의 구별을 명확하게 하기 위하여 음을 끊어줌
            {
                softToneWrite(BUZZER_PIN, 0);
                delay(10);
            }
        }
    }
    else if (soundOn == 2) // under the sea 1
    {
        int melody[10] = {659, 784, 1046, 1046, 1046, 987, 1174, 1046, 784, 0};
        int rhythm[10] = {170, 320, 250, 530, 330, 180, 440, 450, 400, 2000};

        for (int i = 0; i < 10; i++)
        {
            softToneWrite(BUZZER_PIN, melody[i]);
            delay(rhythm[i]);

            for (int j = 0; j < 10; j++)
            {
                softToneWrite(BUZZER_PIN, 0);
                delay(10);
            }
        }
    }
    else if (soundOn == 3) // under the sea 2
    {
        int melody[9] = {523, 659, 784, 784, 784, 587, 698, 659, 0};
        int rhythm[9] = {260, 470, 230, 470, 300, 480, 460, 300, 2000};

        for (int i = 0; i < 9; i++)
        {
            softToneWrite(BUZZER_PIN, melody[i]);
            delay(rhythm[i]);

            for (int j = 0; j < 9; j++)
            {
                softToneWrite(BUZZER_PIN, 0);
                delay(10);
            }
        }
    }
    else if (soundOn == 4) // La Cucaracha 1
    {
        int melody[6] = {523, 523, 523, 698, 880, 0};
        int rhythm[6] = {250, 270, 310, 430, 400, 2000};

        for (int i = 0; i < 6; i++)
        {
            softToneWrite(BUZZER_PIN, melody[i]);
            delay(rhythm[i]);

            for (int j = 0; j < 6; j++)
            {
                softToneWrite(BUZZER_PIN, 0);
                delay(10);
            }
        }
    }
    else if (soundOn == 5) // La Cucaracha 2
    {
        int melody[8] = {698, 659, 698, 659, 698, 740, 784, 0};
        int rhythm[8] = {280, 400, 210, 290, 270, 300, 300, 2000};

        for (int i = 0; i < 8; i++)
        {
            softToneWrite(BUZZER_PIN, melody[i]);
            delay(rhythm[i]);

            for (int j = 0; j < 8; j++)
            {
                softToneWrite(BUZZER_PIN, 0);
                delay(10);
            }
        }
    }
    else if (soundOn == 6) // airplane
    {
        int melody[26] = {659, 587, 523, 587, 659, 659, 659,
                          587, 587, 587, 659, 659, 659,
                          659, 587, 523, 587, 659, 659, 659,
                          587, 587, 659, 587, 523, 0};
        int rhythm[26] = {520, 180, 470, 430, 470, 470, 470,
                          470, 470, 470, 470, 470, 470,
                          520, 180, 470, 430, 470, 470, 470,
                          470, 470, 470, 470, 400, 2000};

        for (int i = 0; i < 26; i++)
        {
            softToneWrite(BUZZER_PIN, melody[i]);
            delay(rhythm[i]);

            for (int j = 0; j < 26; j++)
            {
                softToneWrite(BUZZER_PIN, 0);
                delay(10);
            }
        }
    }
    STOP_FREQ();
}
// DC모터 정지
void MotorStop()
{
    softPwmWrite(MOTOR_MT_N_PIN, 0);
    softPwmWrite(MOTOR_MT_P_PIN, 0);
}

// DC모터 컨트롤(창문 열리고 닫힘)
void MotorControl(UCHAR rotate)
{
    int speed = 10;
    if (rotate == OPEN_WINDOW)
    {
        digitalWrite(MOTOR_MT_N_PIN, LOW);
        softPwmWrite(MOTOR_MT_P_PIN, speed);
    }
    else if (rotate == CLOSE_WINDOW)
    {
        digitalWrite(MOTOR_MT_P_PIN, LOW);
        softPwmWrite(MOTOR_MT_N_PIN, speed);
    }
}

// A/D Converter Read
int ReadMcp3208ADC(unsigned char adcChannel)
{
    unsigned char buff[3];
    int nAdcValue = 0;
    buff[0] = 0x06 | ((adcChannel & 0x07) >> 2);
    buff[1] = ((adcChannel & 0x07) << 6);
    buff[2] = 0x00;
    digitalWrite(CS_MCP3208, 0);
    wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);
    buff[1] = 0x0F & buff[1];
    nAdcValue = (buff[1] << 8) | buff[2];
    digitalWrite(CS_MCP3208, 1);
    return nAdcValue;
}

// 윈도우 상태 출력 메뉴
void SetWindow(int disp, int windowState, int autoState)
{

    lcdClear(disp);
    lcdPosition(disp, 0, 0);

    if (windowState == 0)
        lcdPuts(disp, "Window is close");
    else if (windowState == 1)
        lcdPuts(disp, "Window is opens");

    lcdPosition(disp, 0, 1);
    if (autoState == 0)
        lcdPuts(disp, "Auto mode : Off");
    else if (autoState == 1)
        lcdPuts(disp, "Auto mode : On");
}

// 창문을 열기 위한 함수
void OpenWindow(int disp1)
{
    lcdClear(disp1);
    lcdPosition(disp1, 0, 0);
    lcdPuts(disp1, "Window opening");
    lcdPosition(disp1, 0, 1);
    lcdPuts(disp1, "is processing...");
    MotorControl(OPEN_WINDOW);
    delay(WINDOW_SIZE);
    MotorStop();
}

// 창문을 닫기 위한 함수
void CloseWindow(int disp1)
{
    lcdClear(disp1);
    lcdPosition(disp1, 0, 0);
    lcdPuts(disp1, "Window closing");
    lcdPosition(disp1, 0, 1);
    lcdPuts(disp1, "is processing...");
    MotorControl(CLOSE_WINDOW);
    delay(WINDOW_SIZE);
    MotorStop();
}

// 블루투스로 RSSI값을 가져오는 함수
int getRSSIValue(const char *deviceAddress)
{
    char command[100];
    int rssiValue = 0;

    // hcitool 명령으로 RSSI 값을 얻기
    snprintf(command, sizeof(command), "sudo hcitool rssi %s", deviceAddress);

    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error executing command.\n");
        return rssiValue;
    }

    // RSSI PRINT
    if (fscanf(fp, "RSSI return value: %d\n", &rssiValue) != 1)
    {
        fprintf(stderr, "Error reading RSSI value.\n");
    }

    pclose(fp);

    return rssiValue;
}
int main(void)
{

    if (wiringPiSetupGpio() == -1)
    {
        return 1;
    }

    if (wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1)
    {
        fprintf(stdout, "wiringPiSPISetup Failed: %s\n", strerror(errno));
        return 1;
    }
    if (softPwmCreate(SERVO_PIN, 0, 200) != 0)
    {
        fprintf(stderr, "Error setting up software PWM \n");
        return 1;
    }

    int nKeypadstate = 0; // 입력받은 버튼의 값을 저장할 변수
    int alarmMelody = 1;  // 알람 소리종류 1~6 중 초기값 1로 설정

    // 시간관련 변수 설정
    time_t currentTime;          // 현재 시간을 담을 변수
    char timeValue[6] = "00:00"; // textLCD화면 상에 알람을 설정할 때 띄울 시간 초깃값
    char buffer[6];              // hh:mm 형식의 현재시간 문자열을 저장할 공간
    struct tm *timeinfo;         //

    // textLCD 메뉴 구성에 필요한 변수 설정
    int mainMenu = 1;         // 1=메인메뉴 o 0=메인메뉴 x
    int timeMenu = 0;         // 1=시간메뉴 o 0=시간메뉴 x
    int scrollCount = 0;      // 메인 메뉴의 스크롤 값
    int alarmScrollCount = 0; // 알람설정 메뉴의 스크롤 값
    int timeSelect = 0;       // 1=알람설정메뉴 o 0=알람설정메뉴 x
    int alarmSelect = 0;      // 1=알람 시간 설정 메뉴 o 0=알람 시간 설정 메뉴 x
    int soundSelect = 0;      // 1=알람 소리 설정 메뉴 o 0=알람 소리 설정 메뉴 x
    int temphumSelect = 0;    // 1=온습도메뉴 o 0=온습도메뉴 x

    // 창문 조작을 위한 변수
    int windowSelect = 0;
    int smokelChannel = 2;
    int smokeValue = 0; // 가스 값
    int windowState = 0;
    int autoState = 1;
    int dustChannel = 3;
    float Vo_Val = 0;
    float Voltage = 0;
    float dustDensity = 0;           // 미세 먼지 값(밀도)
    int samplingTime = 280;          // 0.28ms
    int delayTime = 40;              // 0.32 - 0.28 = 0.04ms
    int offTime = 9680;              // 10 - 0.32 = 9.68ms
    int smokeComparisonValue = 1200; // 창문 열고 닫히는 가스값의 기준값
    int dustComparisionValue = 450;  // 창문 열고 닫히는 미세먼지값의 기준값
    // ex) 가스농도가 smokeComparisionValue값보다 높다면 창문이 열리고 낮다면 창문이 닫힘

    // textLCD 설정
    int disp1;
    disp1 = lcdInit(2, 16, 4, RS, STRB, LCD_PIN_D4, LCD_PIN_D5, LCD_PIN_D6, LCD_PIN_D7, 0, 0, 0, 0);

    // Bluetooth 주소 설정
    int motorActivated = 0;
    const char *iphoneAddress = "70:B3:06:1B:33:1F";

    // Motor, MCP3208 pinmode선언
    pinMode(MOTOR_MT_N_PIN, OUTPUT);
    pinMode(MOTOR_MT_P_PIN, OUTPUT);
    pinMode(CS_MCP3208, OUTPUT);
    pinMode(DUST_LED, OUTPUT);
    softPwmCreate(MOTOR_MT_N_PIN, 0, 100);
    softPwmCreate(MOTOR_MT_P_PIN, 0, 100);

    softPwmCreate(SERVO_PIN, 0, 200);

    // Button사용을 위해 Key를 활성화 하기 위해 HIGH값으로 설정
    digitalWrite(KEY_1, HIGH);
    digitalWrite(KEY_2, HIGH);
    digitalWrite(KEY_3, HIGH);
    digitalWrite(KEY_4, HIGH);

    while (1)
    {
        nKeypadstate = KeypadRead();

        time(&currentTime);                                  // 현재 시간을 가져오는 함수(time.h 헤더파일에서 가져옴)
        timeinfo = localtime(&currentTime);                  // 현재시간 timeinfo에 저장
        strftime(buffer, sizeof(buffer), "%H:%M", timeinfo); // timeinfo의 값을 문자열 형태로 buffer에 저장

        // ADC 채널에 입력되는 값을 변환 (미세먼지 센서)
        digitalWrite(DUST_LED, LOW); // IRED on
        delayMicroseconds(samplingTime);
        Vo_Val = ReadMcp3208ADC(dustChannel); // sensor read
        delayMicroseconds(delayTime);
        digitalWrite(DUST_LED, HIGH); // IRED off
        delayMicroseconds(offTime);
        Voltage = (Vo_Val * 3.3 / 1024.0) / 3.0;
        dustDensity = ((0.172 * (Voltage)) - 0.0999) * 1000;

        // ADC 채널에 입력되는 값을 변환 (가스 센서)
        smokeValue = ReadMcp3208ADC(smokelChannel);

        // SMOKE값과 DUST값이 잘 들어가는지 테스트
        // printf("smoke : %d, dust : %f\n", smokeValue, dustDensity);

        if (autoState == 1) // 창문의 Auto모드가 On일때, 즉 창문 열림닫힘이 자동일때 실행
        {
            // smokeValue값과 dustDensity값에 따라 자동으로 창문 조절
            if ((smokeValue >= smokeComparisonValue) || (dustDensity >= dustComparisionValue))
            {
                if (windowState == 0) // smokeValue값이나 dustDensity값이 일정수치보다 크고 창문이 닫혀있으면 창문 열림
                {
                    OpenWindow(disp1); // 창문 열림 함수 호출
                    windowState = 1;   // 창문 열림 상태 1로 정의
                }
            }
            else if ((smokeValue < smokeComparisonValue) && (dustDensity < dustComparisionValue))
            {
                if (windowState == 1) // smokeValue값과 dustDensity값이 일정수치보다 작으면 창문이 열려있을때 창문 닫힘
                {
                    CloseWindow(disp1);
                    windowState = 0;
                }
            }
        }

        // 아이폰과 라즈베리파이의 RSSI 값을 읽어옴
        int rssi = getRSSIValue(iphoneAddress);

        // RSSI 값을 출력
        // fprintf(stderr, "Received RSSI: %d\n", rssi);

        // RSSI 값에 따라 서보 모터 제어
        if (rssi >= -5 && rssi <= -1 && rssi != 0)
        {
            DoorOpen();
        }
        // 메뉴화면에서 버튼3 누를 시 아래로 이동
        if (mainMenu == 1 && timeSelect == 0 && windowSelect == 0 && nKeypadstate == 4)
        {
            scrollCount++;
            delay(100);
        }
        // 메뉴화면에서 버튼1 누를 시 위로 이동
        else if (mainMenu == 1 && timeSelect == 0 && windowSelect == 0 && nKeypadstate == 1)
        {
            if (scrollCount == 0)
            {
                scrollCount = 4;
            }
            else
            {
                scrollCount--;
            }
            delay(100);
        }
        // 메뉴화면에서 시간 메뉴 선택시 시간 화면으로 이동
        else if (mainMenu == 1 && timeSelect == 0 && nKeypadstate == 2 && scrollCount % MAX_MENU_NUM == 0)
        {
            mainMenu = 0;   // 메인메뉴 off
            timeSelect = 0; // 알람설정 on
            timeMenu = 1;   // 시간메뉴 on
        }
        // 시간 화면에서 뒤로가기시 메인메뉴로 이동
        else if (timeMenu == 1 && nKeypadstate == 8)
        {
            mainMenu = 1;   // 메인메뉴 on
            timeSelect = 0; // 알람설정 on
            timeMenu = 0;
        }
        else if (mainMenu == 1 && temphumSelect == 0 && nKeypadstate == 2 && scrollCount % MAX_MENU_NUM == 1)
        {
            mainMenu = 0;      // 메인메뉴 off
            temphumSelect = 1; // 온습도 화면 on
        }
        // 메뉴화면에서 알람메뉴 선택시 알람화면으로 이동
        else if (mainMenu == 1 && timeSelect == 0 && nKeypadstate == 2 && scrollCount % MAX_MENU_NUM == 2)
        {
            mainMenu = 0;   // 메인메뉴 off
            timeSelect = 1; // 알람설정 on
        }
        // 알람 설정 화면의 스크롤 설정(알람 설정 화면 안에서도 알람 시간 설정과 알람 소리 설정 두 개의 메뉴가 있으므로 스크롤 필요)
        else if (mainMenu == 0 && timeSelect == 1 && alarmSelect == 0 && soundSelect == 0 && nKeypadstate == 4)
        {
            alarmScrollCount++;
            delay(100);
        }
        else if (mainMenu == 0 && timeSelect == 1 && alarmSelect == 0 && soundSelect == 0 && nKeypadstate == 1)
        {
            alarmScrollCount--;
            delay(100);
        }
        // 알람설정화면에서 뒤로가기 버튼 누를 시 메인메뉴로 이동
        else if (mainMenu == 0 && timeSelect == 1 && nKeypadstate == 8)
        {
            timeSelect = 0;  // 알람설정 off
            mainMenu = 1;    // 메인메뉴 on
            alarmSelect = 0; // 알람 시간설정 off
        }
        // 알람 시간 설정 메뉴로 이동
        else if (mainMenu == 0 && timeSelect == 1 && alarmScrollCount % 2 == 0 && alarmSelect == 0 && soundSelect == 0 && nKeypadstate == 2)
        {
            alarmSelect = 1;
        }
        // 알람 소리 설정 메뉴로 이동
        else if (mainMenu == 0 && timeSelect == 1 && alarmScrollCount % 2 == 1 && alarmSelect == 0 && soundSelect == 0 && nKeypadstate == 2)
        {
            soundSelect = 1;
        }
        else if (mainMenu == 1 && windowSelect == 0 && nKeypadstate == 2 && scrollCount % MAX_MENU_NUM == 3)
        {
            mainMenu = 0;     // 메인메뉴 off
            windowSelect = 1; // 창문설정 on
        }
        // Text LCD 메뉴화면

        // 메인메뉴
        if (mainMenu == 1 && scrollCount % MAX_MENU_NUM == 0) // 시간
        {
            lcdClear(disp1);
            lcdPosition(disp1, 0, 0);
            lcdPuts(disp1, "1. Time <");
            lcdPosition(disp1, 0, 1);
            lcdPuts(disp1, "2. Temp,Humid");
        }
        else if (mainMenu == 1 && scrollCount % MAX_MENU_NUM == 1) // 온도,습도
        {
            lcdClear(disp1);
            lcdPosition(disp1, 0, 0);
            lcdPuts(disp1, "2. Temp,Humid <");
            lcdPosition(disp1, 0, 1);
            lcdPuts(disp1, "3. Alarm");
        }
        else if (mainMenu == 1 && scrollCount % MAX_MENU_NUM == 2) // 알람설정
        {
            lcdClear(disp1);
            lcdPosition(disp1, 0, 0);
            lcdPuts(disp1, "3. Alarm <");
            lcdPosition(disp1, 0, 1);
            lcdPuts(disp1, "4. Window");
        }
        else if (mainMenu == 1 && scrollCount % MAX_MENU_NUM == 3) // 창문
        {
            lcdClear(disp1);
            lcdPosition(disp1, 0, 0);
            lcdPuts(disp1, "4. Window <");
            lcdPosition(disp1, 0, 1);
            lcdPuts(disp1, "5. Door");
        }
        else if (mainMenu == 1 && scrollCount % MAX_MENU_NUM == 4) // 문
        {
            lcdClear(disp1);
            lcdPosition(disp1, 0, 0);
            lcdPuts(disp1, "5. Door <");
            lcdPosition(disp1, 0, 1);
            lcdPuts(disp1, "1. Time");
        }
        // time메뉴 선택시 현재시간을 보여주는 textLCD 출력
        if (timeMenu == 1)
        {
            lcdClear(disp1);
            lcdPosition(disp1, 0, 0);
            lcdPuts(disp1, "Current Time");
            lcdPosition(disp1, 0, 1);
            lcdPuts(disp1, buffer); // buffer에는 현재시간이 담겨있기 때문에 buffer를 출력
        }
        // 알람설정메뉴 스크롤 기능
        if (timeSelect == 1 && alarmScrollCount % 2 == 0 && alarmSelect == 0 && soundSelect == 0)
        {
            lcdClear(disp1);
            lcdPosition(disp1, 0, 0);
            lcdPuts(disp1, "Alarm Setting <");
            lcdPosition(disp1, 0, 1);
            lcdPuts(disp1, "Sound Setting");
        }
        else if (timeSelect == 1 && alarmScrollCount % 2 == 1 && alarmSelect == 0 && soundSelect == 0)
        {
            lcdClear(disp1);
            lcdPosition(disp1, 0, 0);
            lcdPuts(disp1, "Alarm Setting");
            lcdPosition(disp1, 0, 1);
            lcdPuts(disp1, "Sound Setting <");
        }
        // 알람 시간 설정
        else if (alarmSelect == 1)
        {
            SetAlarm(disp1, timeValue, buffer, alarmMelody);                                     // 알람 시간 설정 함수에 설정한 시간, 현재시간, 알람소리 전달
            if (alarmSelect == 1 && nKeypadstate == 1 || nKeypadstate == 2 || nKeypadstate == 4) // 알람 시간 설정메뉴에서 버튼입력에 따라서
            {
                updateTime(timeValue, nKeypadstate);             // 버튼입력값을 전달하여 timeValue의 값을 업데이트하여
                SetAlarm(disp1, timeValue, buffer, alarmMelody); // 업데이트된 timeValue값을 출력하고 buffer에 담기 현재시간과 같다면
            }                                                    // alarmMelody에 해당하는 알람 소리를 울림
            else if (alarmSelect == 1 && nKeypadstate == 8)
            { // 알람 시간 설정메뉴에서 버튼입력이 8(4번째버튼)일 때 뒤로가기(알람 설정 메뉴로 이동)
                mainMenu = 0;
                alarmSelect = 0;
                soundSelect = 0;
                timeSelect = 1;
            }
            delay(100);
        }
        // 알람 소리 설정
        else if (soundSelect == 1)
        {
            SetSound(disp1, alarmMelody); // 알람 소리 설정 메뉴에 들어왔을 때 초기값을 textLCD에 띄움
            if (soundSelect == 1 && nKeypadstate == 1)
            { // 1번 버튼 입력시 alarmMelody값을 더 해주고
                alarmMelody++;
                if (alarmMelody == 7)
                { // 알람 소리가 6가지 이므로 alarmMelody가 7일대 1로 변경
                    alarmMelody = 1;
                }
                else if (alarmMelody == 0)
                {
                    // 마찬가지로 알람 소리가 6가지 이므로 alarmMelody가 0일대 6으로 변경
                    alarmMelody = 6;
                }
                SetSound(disp1, alarmMelody); // 변경된 alarmMelody를 textLCD에 띄움
            }
            else if (soundSelect == 1 && nKeypadstate == 4)
            { // 4번 버튼 입력시 alarmMelody값을 빼줌

                alarmMelody--;
                if (alarmMelody == 0)
                {
                    alarmMelody = 1;
                }
                SetSound(disp1, alarmMelody);
            }
            else if (soundSelect == 1 && nKeypadstate == 2)
            { // 2번 버튼 입력시 현재 선택한 알람소리 재생
                playSound(alarmMelody);
            }
            else if (soundSelect == 1 && nKeypadstate == 8)
            { // 8번 버튼 입력시 뒤로가기(알람설정메뉴로 이동)
                mainMenu = 0;
                alarmSelect = 0;
                soundSelect = 0;
                timeSelect = 1;
            }
        }
        if (mainMenu == 0 && temphumSelect == 1 && scrollCount % MAX_MENU_NUM == 1)
        {
            read_dht11_dat(disp1); // lcd에 온습도 출력 함수 호출
            delay(1000);           // 1초마다 온습도 갱신
            if (nKeypadstate == 8) // 뒤로가기
            {
                mainMenu = 1;
                temphumSelect = 0;
            }
        }
        if (mainMenu == 0 && windowSelect == 1 && scrollCount % MAX_MENU_NUM == 3)
        {

            if (nKeypadstate == 1 || nKeypadstate == 4) // 위아래 버튼중 하나를 누를때 autoState를조종
            {
                if (autoState == 0)
                    autoState = 1; // auto mode off => auto mode on
                else if (autoState == 1)
                    autoState = 0; // auto mode off => auto mode on
                delay(100);
            }
            SetWindow(disp1, windowState, autoState);

            if (autoState == 0) // auto mode가 0(수동)일 때, 창문을 수동 조작
            {
                if (nKeypadstate == 2) // 확인버튼을 누를 때, 창문 조작
                {
                    if (windowState == 0) // 창문이 닫힘 상태이면 창문이 열림
                    {
                        OpenWindow(disp1);
                        windowState = 1; // 창문이 열렸기 때문에 windowState를 1로 설정
                    }
                    else if (windowState == 1) // 창문이 열림 상태이면 창문이 닫힘
                    {
                        CloseWindow(disp1);
                        windowState = 0; // 창문이 닫혔기 때문에 windowState를 0로 설정
                    }
                }
            }
            if (nKeypadstate == 8) // 뒤로가기
            {
                mainMenu = 1;
                windowSelect = 0;
            }
        }
        if (mainMenu == 1 && scrollCount % MAX_MENU_NUM == 4 && nKeypadstate == 2)
        {
            DoorOpen();
        }
        delay(100);
    }
    return 0;
}