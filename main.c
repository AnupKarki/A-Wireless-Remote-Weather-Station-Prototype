//*****************************************************************************
//
// MSP432 main.c template - Empty main
//
//****************************************************************************

#include "msp.h"
#include "driverlib.h"
#include "supportFunctions.h"
#include <string.h>
#include <stdbool.h>
#include "bme280Interfacing.h"
#include "i2c.h"
#include "ST7735.h"
#include <stdio.h>

uint8_t U0RXData;
uint8_t U2RXData;
int loadCounter = 0;

int getWeatherAPIData = 0;
int weatherAPIflagCounter = 0;
int humidity_threshold = 60;

bool timeset= 0;
bool alert = 0;
int twoSecondToggle = 0;
bool timeBlinker = 0;
bool stockScreen;
int StockCounter = 0;
bool StockFlag = 0;

RTC_C_Calendar newTime;//to store updated time
RTC_C_Calendar currentTime;//to store updated time

char receivingBuffer[2500];
char timeBuffer[32];
int bufferposition = 0;
char displayBuff [20];

char at_commands [100];
char get_command[300];
char SSID [] = "MySpectrumWiFi46-2G";
//char SSID [] = "4TH_STREET_HOOLIGANS";
//char password [] =  "Noisymoon123";
char password [] = "jollycanoe233";
//char SSID [] = "OnePlus3";
//char password [] =  "bullet1994";
int start = 1;


char deviceId [] = "v2773A2D03447015";
char alertDeviceId [] = "v91DA0C25F9DBADE";


char wundergroundAPIKey[] = "64d6b3bbf5110b2a";
char tempkey [] = "temp_c";
char humiditykey [] = "relative_humidity";
char pressurekey [] = "pressure_in";

char alpha_vantage_key [] = "PNJWOOXW0GQDJ3O2";

SENSOR bme280_sensor;

char temperature[7];
char pressure[7];
char humidity[7];
char light[2];

char tempAPI[7];
char pressureAPI[7];
char humidityAPI[7];

char STK1 [] = "MCI";
char STK2 [] = "DIS";
char STK3 [] = "COKE";
char STK4 [] = "PEP";
char STK5 [] = "AAPL";

char STK1Value [10];
char STK2Value [10];
char STK3Value [10];
char STK4Value [10];
char STK5Value [10];


float humidity_alert;


int weatherAPIFlagCounter = 0;
bool wAPIFlag = 0;





typedef enum  {ConnectionSetUp, SetESPMode, ConnectToRouter, TCPConnect,UpdateData,END,Alert,SendAlertData,Display,WeatherAPIConnect,WeatherAPI,ExtractJSON,DataExtraction,ConnectSpreadSheet,StockConnect,StockData,ExtractStock,StockExtraction} states;
states CurrentState;

const eUSCI_UART_Config uartConfig =
{
        EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
        6,                                      // BRDIV = 26
        8,                                       // UCxBRF = 0
        0x11,                                       // UCxBRS = 0
        EUSCI_A_UART_NO_PARITY,                  // No Parity
        EUSCI_A_UART_LSB_FIRST,                  // MSB First
        EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
        EUSCI_A_UART_MODE,                       // UART mode
        EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  // Low Frequency Mode
};

void DelayWait10ms(uint32_t n){
  Delay1ms(n*10);
}

void display(int row, int col, char *str, int front, int back, int size)
{
    int8_t i = 0;
    for(i = 0; str[i]!= '\0';i++)
    {
        ST7735_DrawCharS(row*8 +6*i*size, col * 6*size , str[i], front, back, size);
    }

}


void EUSCIA2_IRQHandler(void)
{
    uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A2_BASE);
    MAP_UART_clearInterruptFlag(EUSCI_A2_BASE,status);

    if (status & EUSCI_A_IE_RXIE)
    {

        U2RXData = MAP_UART_receiveData(EUSCI_A2_BASE);

        if(bufferposition >= 2500)
            bufferposition = 0;
        receivingBuffer [bufferposition++] = U2RXData;
        while(!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = U2RXData;

    }
}

int checkforOK()
{
    printf("Response: %s",receivingBuffer);
    int charPointer = strstr(receivingBuffer,"OK");
    //lastIndex = bufferposition;

    if (charPointer != NULL)
        return 1;
    else
        return 0;

}

int checkforClosed()
{
    //printf("Response: %s",receivingBuffer);
    int charPointer = strstr(receivingBuffer,"CLOSED");
    //lastIndex = bufferposition;

    if (charPointer != NULL)
        return 1;
    else
        return 0;

}

int checkforError()
{
    //printf("Response: %s",receivingBuffer);
    int charPointer = strstr(receivingBuffer,"ERROR");
    //lastIndex = bufferposition;

    if (charPointer != NULL)
        return 1;
    else
        return 0;

}

int alreadyConnected()
{
    int charPointer = strstr(receivingBuffer,"ALREADY CONNECTED");
    //lastIndex = bufferposition;

    if (charPointer != NULL)
        return 1;
    else
        return 0;
}



char* getTime()
{
    char *timePointer = strstr(receivingBuffer,"+IPD,51:");
    if (timePointer != NULL)
        return timePointer;
    else
        return 0;
}

void updateDateAndTime(){
    char* pointintTotime;
    pointintTotime = getTime();
    if(pointintTotime){
        memcpy(timeBuffer,pointintTotime,32);
        getDateAndTime();
    }

}

void getDateAndTime()
{
    newTime.year = (int)(timeBuffer[15]-48)*10+(int)(timeBuffer[16]-48);
    newTime.month = (int)(timeBuffer[18]-48)*10+(int)(timeBuffer[19]-48);
    newTime.dayOfmonth = (int)(timeBuffer[21]-48)*10+(int)(timeBuffer[22]-48);
    newTime.hours = (int)(timeBuffer[24]-48)*10+(int)(timeBuffer[25]-48);
    newTime.minutes = (int)(timeBuffer[27]-48)*10+(int)(timeBuffer[28]-48);

    if (newTime.hours == 0)
    {
        newTime.hours = 24;
        newTime.dayOfmonth -=1;
    }
    newTime.hours -= 4;
    display(2,9,"Got Time...",ST7735_GREEN,ST7735_BLACK,1);

    MAP_RTC_C_initCalendar(&newTime, RTC_C_FORMAT_BINARY);//passing the entered input in RTC in binary format

    /* Specify an interrupt to assert every minute */
    MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_MINUTECHANGE);
    MAP_RTC_C_startClock();

    timeset = 1;
    ST7735_FillScreen(ST7735_BLACK);
    CurrentState = END;

}


int checkForIPD()
{
    int ipdPointer = strstr(receivingBuffer,"+IPD");
    if (ipdPointer != NULL)
            return 1;
        else
            return 0;

}

int getJsonData()
{
    int dataPointer = strstr(receivingBuffer,"current_observation");
        if (dataPointer != NULL)
        {
            return 1;
        }
        else
            return 0;
}

void getWeatherData()
{

    if(getJsonData)
    {
        setTempAPI();
        setPresAPI();
        setHumidityAPI();

    }


}


void setHumidityAPI()
{
    int startposition = 0;
    int index = 0;
    char *getHumidAPI = strstr(receivingBuffer,humiditykey);
     if (getHumidAPI !=NULL)
      {
         startposition = strlen(humiditykey)+3;
          while (getHumidAPI[startposition+index] != ',')
           {
               humidityAPI[index] = getHumidAPI[startposition+index];
             index++;
            }
           humidityAPI[index-1] = '\0';
           }
}

void setTempAPI()
{
    int startposition = 0;
    int index = 0;
    char *getTempAPI = strstr(receivingBuffer,tempkey);
     if (getTempAPI !=NULL)
      {
         startposition = strlen(tempkey)+2;
           while (getTempAPI[startposition+index] != ',')
           {
               tempAPI[index] = getTempAPI[startposition+index];
             index++;
            }
           tempAPI[index] = '\0';
           }
}

void setPresAPI()
{
    int startposition = 0;
    int index = 0;
    char *getPresAPI = strstr(receivingBuffer,pressurekey);
     if (getPresAPI !=NULL)
      {
         startposition = strlen(pressurekey)+3;
           while (getPresAPI[startposition+index] != ',')
           {
               pressureAPI[index] = getPresAPI[startposition+index];
             index++;
            }
           pressureAPI[index-1] = '\0';
        }

}



void setStock1(){
    int startposition = 0;
    int index = 0;
    char *getStock1 = strstr(receivingBuffer,STK1);
    if (getStock1 !=NULL)
    {
        startposition = strlen(STK1)+2;
        while (getStock1[startposition+index] != ',')
        {
            STK1Value[index] = getStock1[startposition+index];
            index++;
        }
        STK1Value[index] = '\0';
    }
}

void setStock2(){
    int startposition = 0;
    int index = 0;
    char *getStock2 = strstr(receivingBuffer,STK2);
    if (getStock2 !=NULL)
    {
        startposition = strlen(STK2)+2;
        while (getStock2[startposition+index] != ',')
        {
            STK2Value[index] = getStock2[startposition+index];
            index++;
        }
        STK2Value[index] = '\0';
    }
}

void setStock3(){
    int startposition = 0;
    int index = 0;
    char *getStock3 = strstr(receivingBuffer,STK3);
    if (getStock3 !=NULL)
    {
        startposition = strlen(STK3)+2;
        while (getStock3[startposition+index] != ',')
        {
            STK3Value[index] = getStock3[startposition+index];
            index++;
        }
        STK3Value[index] = '\0';
    }
}

void setStock4(){
    int startposition = 0;
    int index = 0;
    char *getStock4 = strstr(receivingBuffer,STK4);
    if (getStock4 !=NULL)
    {
        startposition = strlen(STK4)+2;
        while (getStock4[startposition+index] != ',')
        {
            STK4Value[index] = getStock4[startposition+index];
            index++;
        }
        STK4Value[index] = '\0';
    }
}

void setStock5(){
    int startposition = 0;
    int index = 0;
    char *getStock5 = strstr(receivingBuffer,STK5);
    if (getStock5 !=NULL)
    {
        startposition = strlen(STK5)+2;
        while (getStock5[startposition+index] != ',')
        {
            STK5Value[index] = getStock5[startposition+index];
            index++;
        }
        STK5Value[index] = '\0';
    }
}

void getStockData()
{
    setStock1();
    setStock2();
    setStock3();
    setStock4();
    setStock5();
}




void send_to_ESP8266(char *sendStr)
{   int i;
    //printf(" Sending: %s",sendStr);
    for(i=0;i<strlen(sendStr);i++)
        MAP_UART_transmitData(EUSCI_A2_BASE,sendStr[i]);
}

void resetBuffer()
{
    memset(receivingBuffer,0,2500);
    bufferposition = 0;
}



void displayData(){
    sprintf(displayBuff,"%.2d/%.2d/%.2d", currentTime.year, currentTime.month, currentTime.dayOfmonth);
    display(1,0,displayBuff,ST7735_GREEN,ST7735_BLACK,1);

    if (timeBlinker)
        sprintf(displayBuff,"%.2d %.2d", currentTime.hours, currentTime.minutes);
    else
        sprintf(displayBuff,"%.2d:%.2d", currentTime.hours, currentTime.minutes);

    display(10,0,displayBuff,ST7735_GREEN,ST7735_BLACK,1);

    display(3,3,LightCondition, ST7735_GREEN,ST7735_BLACK,1);

    display(2,5,"Sensor/API",ST7735_WHITE,ST7735_BLACK,1);
    display(0,7,"Temp: ", ST7735_CYAN,ST7735_BLACK,1);
    sprintf(displayBuff,"%s/%s C", temperature,tempAPI);
    display(4,7,displayBuff, ST7735_GREEN,ST7735_BLACK,1);
    display(0,9,"Pres: ",ST7735_CYAN,ST7735_BLACK,1);
    sprintf(displayBuff,"%s/%s inHg", pressure,pressureAPI);
    display(4,9,displayBuff, ST7735_GREEN,ST7735_BLACK,1);
    display(0,11,"Humd: ",ST7735_CYAN,ST7735_BLACK,1);
    sprintf(displayBuff,"%s/%s %%rh", humidity,humidityAPI);
    display(4,11,displayBuff, ST7735_GREEN,ST7735_BLACK,1);


    display(4,15,"STOCK",ST7735_WHITE,ST7735_BLACK,1);

    sprintf(displayBuff,"   %s:%s ", STK1,STK1Value);
    display(0,17,displayBuff, ST7735_GREEN,ST7735_BLACK,1);

    sprintf(displayBuff,"   %s:%s ", STK2,STK2Value);
    display(0,19,displayBuff, ST7735_GREEN,ST7735_BLACK,1);

    sprintf(displayBuff,"   %s:%s ", STK3,STK3Value);
    display(0,21,displayBuff, ST7735_GREEN,ST7735_BLACK,1);

    sprintf(displayBuff,"   %s:%s ", STK4,STK4Value);
    display(0,23,displayBuff, ST7735_GREEN,ST7735_BLACK,1);

    sprintf(displayBuff,"   %s:%s ", STK5,STK5Value);
    display(0,25,displayBuff, ST7735_GREEN,ST7735_BLACK,1);


}




void main(void)
{
	
        WDTCTL = WDTPW | WDTHOLD;           // Stop watchdog timer

            /* Selecting P6.4 for Reset*/
                     MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P6, GPIO_PIN4);
            //         MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN4);

            clockInit48MHzXTL();

            MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);

            /* Select Port 6 for I2C - Set Pin 4, 5 to input Primary Module Function,
                     *   (UCB1SIMO/UCB1SDA, UCB1SOMI/UCB1SCL).
                     */
             MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6,
                             GPIO_PIN4 + GPIO_PIN5, GPIO_PRIMARY_MODULE_FUNCTION);

             /* Enabling the FPU for floating point operation */
                 MAP_FPU_enableModule();
                 MAP_FPU_enableLazyStacking();


            /* Selecting P1.2 and P1.3 in UART mode. */
            MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
                    GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

            /* Configuring UART Module */
            MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);

            /* Enable UART module */
            MAP_UART_enableModule(EUSCI_A0_BASE);

            UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
            Interrupt_enableInterrupt(INT_EUSCIA0);

            /* Configuring SysTick to trigger at 48000 (MCLK is 48 MHz so this will trigger every 1 ms) */
                    MAP_SysTick_enableModule();
                    MAP_SysTick_setPeriod(48 * 1000 * 1);
                    MAP_SysTick_enableInterrupt();  // use a simple interrupt service routine to keep track of time intervals
                //    MAP_Interrupt_enableMaster();

           /* Selecting P3.2 and P3.3 in UART mode. */
                MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P3,
                        GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
                /* Configuring UART Module */
                MAP_UART_initModule(EUSCI_A2_BASE, &uartConfig);

                /* Enable UART module */
                MAP_UART_enableModule(EUSCI_A2_BASE);

                UART_enableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
                Interrupt_enableInterrupt(INT_EUSCIA2);


                /* Specify an interrupt to assert every minute */
                MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_MINUTECHANGE);

                /* Enable interrupt for RTC Ready Status, which asserts when the RTC
                 * Calendar registers are ready to read.
                 * Also, enable interrupts for the Calendar alarm and Calendar event. */
                MAP_RTC_C_clearInterruptFlag(
                        RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
                                | RTC_C_CLOCK_ALARM_INTERRUPT);
                MAP_RTC_C_enableInterrupt(
                        RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
                                | RTC_C_CLOCK_ALARM_INTERRUPT);

            MAP_Interrupt_enableInterrupt(INT_RTC_C);
            MAP_Interrupt_enableMaster();

            adcInit();
            bme280Init();
            ST7735_InitR(INITR_REDTAB);//enabling LCD


            CurrentState = ConnectionSetUp;
            display(2,9,"Connecting...",ST7735_GREEN,ST7735_BLACK,1);

            while(1)
            {
                switch(CurrentState){

                    case ConnectionSetUp:
                         {
                          send_to_ESP8266("AT\r\n");
                          delayNms(50);
                          if(checkforOK())
                          {
                              send_to_ESP8266("AT+RST\r\n");
                              CurrentState = SetESPMode;
                              delayNms(50);

                          }
                          resetBuffer();
                          break;
                         }

                    case SetESPMode:
                    {
                        send_to_ESP8266("AT+CWMODE=1\r\n");

                        delayNms(50);
                        if(checkforOK())
                          {
                              CurrentState = ConnectToRouter;

                              delayNms(50);
                          }
                        resetBuffer();

                          break;
                    }
                    case ConnectToRouter:
                    {
                        sprintf(at_commands,"AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,password);
                        send_to_ESP8266(at_commands);
                        delayNms(2000);
                        if(checkforOK())
                        {
                            CurrentState = TCPConnect;
                            delayNms(50);
                            display(2,9,"Connected To",ST7735_GREEN,ST7735_BLACK,1);
                            display(2,11,SSID,ST7735_GREEN,ST7735_BLACK,1);

                        }

                        resetBuffer();
                        break;

                    }

                    case TCPConnect:
                    {
                        while(!timeset){
                            display(2,9,"Getting Time ...",ST7735_GREEN,ST7735_BLACK,1);
                            display(2,11,"                             ",ST7735_GREEN,ST7735_BLACK,1);
                            send_to_ESP8266("AT+CIPSTART=\"TCP\",\"time.nist.gov\",13\r\n");
                            delayNms(2000);

                            if (alreadyConnected())
                            {
                                send_to_ESP8266("AT+CIPCLOSE\r\n");
                            }
                            if(checkforClosed())
                            {
                                updateDateAndTime();
                            }
                            else
                            {
                                delayNms(3000);
                            }

                    }
                        resetBuffer();
                        break;
                    }

                    case ConnectSpreadSheet:
                    {
                        send_to_ESP8266("AT+CIPSTART=\"TCP\",\"api.pushingbox.com\",80\r\n");
                        delayNms(2000);
                        if(checkforOK())
                        {
                            CurrentState = UpdateData;
                            delayNms(50);

                        }
                        resetBuffer();
                        break;
                    }

                    case UpdateData:
                    {

                        sprintf(get_command,"GET /pushingbox?devid=%s&humidityData=%s&celData=%s&hicData=%s&hifData=%s HTTP/1.1\r\nHost: api.pushingbox.com\r\nUser-Agent: ESP8266/1.0\r\nConnection:close \r\n\r\n",deviceId,humidity,temperature,pressure,light);
                        sprintf(at_commands,"AT+CIPSEND=%d\r\n",strlen(get_command));
                        send_to_ESP8266(at_commands);
                        delayNms(100);
                        send_to_ESP8266(get_command);
                        delayNms(2000);

                        CurrentState = END;
                        resetBuffer();
                        break;

                    }



                    case WeatherAPIConnect:
                    {
                        send_to_ESP8266("AT+CIPSTART=\"TCP\",\"api.wunderground.com\",80\r\n");
                        delayNms(2000);
                        if(checkforOK())
                        {
                            CurrentState = WeatherAPI;
                            delayNms(50);

                        }
                        resetBuffer();
                        break;

                    }

                    case WeatherAPI:
                    {
                        sprintf(get_command,"GET http://api.wunderground.com/api/%s/conditions/q/MI/Grand_Rapids.json HTTP/1.1\r\nHost:api.wunderground.com\r\nConnection: close\r\n\r\n",wundergroundAPIKey);
                        sprintf(at_commands,"AT+CIPSEND=%d\r\n",strlen(get_command));
                        send_to_ESP8266(at_commands);
                        delayNms(100);
                        send_to_ESP8266(get_command);
                        delayNms(2000);

                        CurrentState = ExtractJSON;
                        break;

                    }

                    case ExtractJSON:
                    {
                        if (checkforClosed())
                        {
                            if (checkForIPD)
                            {
                                CurrentState = DataExtraction;
                            }
                        }
                        break;
                    }

                    case DataExtraction:
                    {
                        getWeatherData();
                        delayNms(1000);
                        resetBuffer();
                        CurrentState = END;

                        break;
                    }


                    case Alert:
                    {
                        send_to_ESP8266("AT+CIPSTART=\"TCP\",\"api.pushingbox.com\",80\r\n");
                        delayNms(2000);
                        if(checkforOK())
                        {
                         CurrentState = SendAlertData;
                         delayNms(50);
                        }
                         resetBuffer();
                        break;
                    }

                    case SendAlertData:
                    {

                        sprintf(get_command,"GET /pushingbox?devid=%s&value=%s HTTP/1.1\r\nHost: api.pushingbox.com\r\nUser-Agent: ESP8266Alerter/1.0\r\nConnection:close \r\n\r\n",alertDeviceId,humidity);
                        sprintf(at_commands,"AT+CIPSEND=%d\r\n",strlen(get_command));
                        send_to_ESP8266(at_commands);
                        delayNms(100);
                        send_to_ESP8266(get_command);
                        delayNms(2000);
                        alert = 0;
                        resetBuffer();
                        CurrentState = END;
                        break;
                  }

                    case StockConnect:
                    {
                        send_to_ESP8266("AT+CIPSTART=\"TCP\",\"download.finance.yahoo.com\",80\r\n");
                        delayNms(2000);
                        if(checkforOK())
                        {
                        CurrentState = StockData;
                        delayNms(50);

                        }
                        resetBuffer();
                        break;
                    }

                    case StockData:
                    {

                        //sprintf(get_command,"GET http://api.wunderground.com/api/%s/conditions/q/MI/Grand_Rapids.json HTTP/1.1\r\nHost:api.wunderground.com\r\nConnection: close\r\n\r\n",wundergroundAPIKey);

                        sprintf(get_command, "GET /d/quotes.csv?s=%s+%s+%s+%s+%s&f=sl1d1t1c1 HTTP/1.1\r\nHost:download.finance.yahoo.com\r\nConnection: close\r\n\r\n",STK1,STK2,STK3,STK4,STK5);
                        sprintf(at_commands,"AT+CIPSEND=%d\r\n",strlen(get_command));
                        send_to_ESP8266(at_commands);
                        delayNms(100);
                        send_to_ESP8266(get_command);
                        delayNms(3000);


                        CurrentState = ExtractStock;
                        break;

                    }

                    case ExtractStock:
                    {
                        if (checkforClosed())
                         {
                            if (checkForIPD)
                            {
                            CurrentState = StockExtraction;
                            }
                        }
                        break;

                    }
                    case StockExtraction:
                    {
                        getStockData();
                        resetBuffer();
                        delayNms(2000);

                        CurrentState = END;
                        break;
                    }
                    case END:
                    {
                        getSensorValues(&temperature,&pressure,&humidity,&light,&humidity_alert);
                        if (humidity_alert > humidity_threshold)
                        {
                            alert = 1;
                            CurrentState = Alert;
                        }
                        else
                            alert = 0;

                        if(wAPIFlag)
                        {

                            CurrentState = WeatherAPIConnect;
                            wAPIFlag = 0;

                        }
                       // if (StockFlag)
                       // {
                       //     CurrentState  = StockConnect;
                       //     StockFlag = 0;

                       // }

                        if (start == 0)
                            displayData();
                        if (start){
                            display(2,9,"Connecting...",ST7735_GREEN,ST7735_BLACK,1);
                            if (start == 1){
                                display(2,9,"Getting Weather..",ST7735_GREEN,ST7735_BLACK,1);
                                CurrentState = WeatherAPIConnect;

                            }
                            if (start == 2)
                            {
                           //     display(2,9,"Getting Stock..",ST7735_GREEN,ST7735_BLACK,1);
                           //     CurrentState = StockConnect;
                            }
                            start++;
                            if (start == 3)
                            {
                                start = 0;
                                ST7735_FillScreen(ST7735_BLACK);
                            }
                        }

                        break;
                }


                    default:
                        break;

                }
            }
}




void RTC_C_IRQHandler(void)
{
    uint32_t status;

    status = MAP_RTC_C_getEnabledInterruptStatus();
    MAP_RTC_C_clearInterruptFlag(status);

    if (status & RTC_C_CLOCK_READ_READY_INTERRUPT)//taking interrupt in each second
    {
        MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
        currentTime = MAP_RTC_C_getCalendarTime();//storing the updated value

        timeBlinker = !timeBlinker;


        //flag1 = 1;// goes into while loop
       // trendsPutVal = 1 ;
    }

    if (status & RTC_C_TIME_EVENT_INTERRUPT)
    {//taking interrupt in each minute
      //  trendsPutVal =1 ;
       // __no_operation();
        twoSecondToggle++;
        weatherAPIFlagCounter++;

        if(twoSecondToggle == 2)
        {
            twoSecondToggle = 0;
            CurrentState = ConnectSpreadSheet;
        }

        if (weatherAPIFlagCounter == 3)
        {
            wAPIFlag = 1;

        }
        StockCounter ++;
        if (StockCounter == 25)
         {
            StockCounter = 0;
            CurrentState = StockConnect;

         }


    }

    if (status & RTC_C_CLOCK_ALARM_INTERRUPT)//interrupts when time reaches alarm time
    {

        __no_operation();
    }

}

