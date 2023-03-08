#include <SPI.h>
#include <uRTCLib.h>
#include <SD.h>
#include <stdio.h>
#include <math.h>
#include <LiquidCrystal_I2C.h> 

#define         MQ_PIN                       34     // chân nhận tín từ cảm biến MQ2
#define         RL_VALUE                     5      // giá trị điện trở tải trên module MQ2 (kilo_ohm)
#define         RS_RO_RATIO_CLEAN_AIR        9.83   // hằng số tỷ lệ Rs/R0 ( chi tiết trong datasheet)

#define         SAMPLE_TIMES                 20     // số lần đọc trong 1 lần lấy mẫu
#define         RO_SAMPLE_INTERVAL           500    // khoảng thời gian giữa 2 lần lấy mẫu đối với tìm Ro (mili giây)                                              
#define         READ_SAMPLE_INTERVAL         50     // khoảng thời gian giữa 2 lần lấy mẫu đối với đọc giá trị adc (mili giây)


#define         GAS_LPG                      0     // Khí dễ cháy
#define         GAS_METHANE                  1     // Khí metan
#define         GAS_SMOKE                    2     // Khói
#define         GAS_ALCOHOL                  3     // Khí ancol
#define         GAS_PROPANE                  4     // Khí C3H8

LiquidCrystal_I2C lcd(0x27, 16, 2);                   // Cấu hình LCD

uRTCLib rtc(0x68);

/*
  Biểu đồ trong datasheet là biểu đồ theo tỷ lệ logarit.
  https://www.mouser.com/datasheet/2/321/605-00008-MQ-2-Datasheet-370464.pdf
  Vì thế nếu xét theo tỷ lệ tuyến tính, thì sự thay đổi 
  nồng độ khí dẫn đến sự thay đổi tỷ lệ trở kháng theo 
  cấp số nhân. Dữ liệu cho thấy nồng độ khí dao động từ
  200 ppm đến 10000 ppm. Với mỗi đường khác nhau lại thể
  hiện cho loại khí khác nhau. Đối với loại khí mà dự
  án cần khảo sát thì gần như đường đó là đường thẳng 
  tuyến tính.

  Vì vậy chúng tôi có thể mô hình hóa nó dưới dạng công thức:

    Rs/R0 = a * Concentrations_of_gases + b

  Đặt: 

    x = Concentrations_of_gases
    y = Rs/R0
  
  Vì đây là đồ thì log-log nên công thức sẽ trở thành:

    log(y) = a * log(x) + b

  Từ đó với mỗi loại khí khác nhau, từ 2 điểm trên đồ thị
  chúng ta có thể xác định được 2 tham số a và b đặc trưng 
  cho từng loại khí.
  
  GAS         |      a        |     b     |
  H2          -0.47305447     1.412572126
  LPG         -0.454838059    1.25063406      (C4H10)
  Methane     -0.372003751    1.349158571     (CH4)
  CO          -0.33975668     1.512022272
  Alcohol     -0.373311285    1.310286169     (C2H5OH)
  Smoke       -0.44340257     1.617856412
  Propane     -0.461038681    1.290828982     (C3H8)         
  
*/
float           LPGCurve[2]       =  {-0.454838059   , 1.25063406};   
float           MethaneCurve[2]   =  {-0.372003751   , 1.349158571};    
float           COCurve[2]        =  {-0.33975668    , 1.512022272};    
float           AlcoholCurve[2]   =  {-0.373311285   , 1.310286169};
float           SmokeCurve[2]     =  {-0.44340257    , 1.617856412};
float           PropaneCurve[2]   =  {-0.461038681   , 1.290828982};

float           Ro;                                 // sensor resistance at 1000ppm of H2 in the clean air.

void setup()
{
  Serial.begin(115200);                             //UART
  delay(120000)                                     // Làm nóng cảm biến trong 2 phút.
  Serial.print("Calibrating...\n");                                
  Serial.print("Calibration is done...\n"); 
  Serial.print("Ro=");
  Serial.print(Ro);
  Serial.print("kohm");
  Serial.print("\n");

  URTCLIB_WIRE.begin();      // Khởi tạo bộ đếm thời gian thực

  rtc.set(0, 51, 13, 1, 24, 2, 23);

  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0,0);
  lcd.print("     We Air     ");
  delay(1500);
  lcd.print("Calibrating..");
  delay(500);
  lcd.print("Calibrating...");
  delay(500);
  lcd.print("Calibrating....");

  Ro = RoCalibration(MQ_PIN); 

  lcd.print("...........DONE"); 
  delay(1000);
}

void loop()
{
  rtc.refresh();
  
  Serial.printf("%d/%d/%d [%d:%d:%d]",rtc.year(), rtc.month(), rtc.day(), rtc.hour(), rtc.minute(), rtc.second());
  
  float _readRs      =    readRs(MQ_PIN);

   int LPGppm        =    getGas(_readRs/Ro,  GAS_LPG);
   int Methaneppm    =    getGas(_readRs/Ro,  GAS_METHANE);
   int SMOKEppm      =    getGas(_readRs/Ro,  GAS_SMOKE);
   int Alcoholppm    =    getGas(_readRs/Ro,  GAS_ALCOHOL);
   int Propaneppm    =    getGas(_readRs/Ro,  GAS_PROPANE);

  Serial.printf("LPG: %dppm; PRO: %dppm; SMK: %dppm\n",LPGppm, Propaneppm, SMOKEppm);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(rtc.year());
  lcd.print("/");
  lcd.print(rtc.month());
  lcd.print("/");
  lcd.print(rtc.day());

  lcd.setCursor(0,1);
  lcd.print("LPG: "); lcd.print(LPGppm);

  delay(1000);

}

/****************************** RsCalculator ****************************************
 * Tìm giá trị trở kháng của MQ2
 * 
 * Công thức: Rs = (VCC * RL) / Vout - RL
 * 
*************************************************************************************/ 
float RsCalculator(int raw_adc)
{
  if (raw_adc == 0) raw_adc++;
  float rs = (float)RL_VALUE * 4095 / raw_adc - RL_VALUE;
  return rs;
}

/***************************** RoCalibration ****************************************
 *  Tìm giá trị Ro của cảm biến Mq2
 * 
 *  Số lần lấy mẫu:      SAMPLE_TIMES
 *  Thời gian lấy mẫu:   RO_SAMPLE_INTERVAL
************************************************************************************/ 
float RoCalibration(int mq_pin)
{
  int i;
  float val=0;

  for (i=0; i < SAMPLE_TIMES; i++) {           
    val += RsCalculator(analogRead(mq_pin));
    delay(RO_SAMPLE_INTERVAL);
  }
  val = val / SAMPLE_TIMES;                   

  val = val / RS_RO_RATIO_CLEAN_AIR;                        

  return val; 
}
/**********************************  readRawData ************************************
 * Đọc giá trị Rs trả về bởi cảm biến MQ2. 
 * 
 * Số lần lấy mẫu :   SAMPLE_TIMES
 * Thời gian lấy mẫu: READ_SAMPLE_INTERVAL
 * 
*************************************************************************************/ 
float readRs(int mq_pin)
{
  int i;
  float rs=0;

  for (i=0; i < SAMPLE_TIMES; i++) {
    rs += RsCalculator(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }

  rs = rs/SAMPLE_TIMES;

  return rs;  
}

/************************************  getGas ***************************************
 * gas_id = {GAS_LPG, GAS_METHANE, GAS_SMOKE}
 * 
 * Chúng tôi lựa chọn hiển thị cho người dùng 3 loại khí đặc trưng bao gồm:
 * 
 *  Khí dễ cháy
 *  Khí CO 
 *  Khói
 * 
*************************************************************************************/ 
int getGas(float rs_ro_ratio, int gas_id)
{
  if ( gas_id == GAS_LPG ) {
     return ppmCalculator(rs_ro_ratio,LPGCurve);
  } else if ( gas_id == GAS_METHANE ) {
     return ppmCalculator(rs_ro_ratio,MethaneCurve);
  } else if ( gas_id == GAS_SMOKE ) {
     return ppmCalculator(rs_ro_ratio,SmokeCurve);
  } else if ( gas_id == GAS_ALCOHOL ) {
     return ppmCalculator(rs_ro_ratio,AlcoholCurve);
  } else if ( gas_id == GAS_PROPANE ) {
     return ppmCalculator(rs_ro_ratio,PropaneCurve);
  }   

  return 0;
}

/*****************************  ppmCalculator **********************************
 *                        
 *                        x = 10 ^ {[log(y) - b] / a}
 * 
 *                        x = Concentrations_of_gases
 *                        y = Rs/R0
 * 
********************************************************************************/ 
int  ppmCalculator(float rs_ro_ratio, float *pcurve)
{
  return (pow(10,( ((log(rs_ro_ratio)/log(10) - pcurve[1])) / pcurve[0])));
}
