/*******************************************************
This program was created by the
CodeWizardAVR V3.12 Advanced
Automatic Program Generator
© Copyright 1998-2014 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : Circuit of the WeathRobo robot
Version : 
Date    : 09/14/2016
Author  : Mohammad Samadi Gharajeh
Company : 
Comments: 


Chip type               : ATmega32
Program type            : Application
AVR Core Clock frequency: 8.000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 512
*******************************************************/

#include <mega32.h>
#include <delay.h> 
#include <stdlib.h>
#include <interrupt.h>   
#include <lcd.h>
#include <math.h>

#asm
   .equ __lcd_port=0x18; PORTB
#endasm
 
int Temperature, Temperature_Average, Gas, Gas_Average, WeatherCondition, Light, Light_Average, Reflective_Left, Reflective_Front, Reflective_Right, AmbientBrightness, Speed=255, n=0;
char Str[];
float R[5][5]={{0.1, 0.2, 0.5, 0.8, 0.3}, {0.3, 0.5, 0.7, 0.8, 0.7}, {0.5, 0.5, 0.8, 1.0, 0.9}, {0.5, 0.5, 0.5, 0.9, 0.5}, {0.3, 0.3, 0.4, 0.4, 0.3}};
int WeatherCondition_Universe[5]={0, 25, 50, 75, 100}, Speed_Universe[5]={1, 64, 127, 191, 255};
long int LightIntensity_Universe[5]={0, 25000, 50000, 75000, 100000};
float *Fuzzification_FuzzySet;
float WeatherCondition_FuzzySet[5], LightIntensity_FuzzySet[5], Input_FuzzySet[5], Speed_FuzzySet[5];
int Direction=0; // Left=0, Right=1
int T1=1000; // Time of the 90 degrees rotation
int T2=2000; // Time of the straight forward moving among the two 90 degrees rotations
int T3=500;  // Time of the 45 degrees rotation

//********************************************************************
int Read_ADC_Temperature()
{
    DDRA=0X00;
    ADMUX=0b11000000;
    ADCSRA.6=1;
    
    delay_ms(10);
        
    ADCSRA=0b11000000;    
    while (ADCSRA.4==0);
    ADCSRA.4=1;
    
    return ADCW;      
}
//********************************************************************
int Read_ADC_Gas()
{
    DDRA=0X00;
    ADMUX=0b11000001;
    ADCSRA.6=1;
    
    delay_ms(10);
        
    ADCSRA=0b11000000;    
    while (ADCSRA.4==0);
    ADCSRA.4=1;
    
    return ADCW;
}
//********************************************************************
int Read_ADC_Light()
{
    DDRA=0X00;
    ADMUX=0b11000010;
    ADCSRA.6=1;
    
    delay_ms(10);
        
    ADCSRA=0b11000000;    
    while (ADCSRA.4==0);
    ADCSRA.4=1;
    
    return ADCW;
}
//********************************************************************
int Read_ADC_Reflective(int ID)
{
    DDRA=0X00;
    if (ID==1)
        ADMUX=0b11000011;
    else if (ID==2)
        ADMUX=0b11000100;
    else if (ID==3)
        ADMUX=0b11000101;
    ADCSRA.6=1;
    
    delay_ms(10);
        
    ADCSRA=0b11000000;    
    while (ADCSRA.4==0);
    ADCSRA.4=1;
    
    return ADCW;
}
//********************************************************************
void Set_LED(int LED_No)
{
    // Set some pins of Port C as output
    DDRC.0=1;
    DDRC.1=1;
    DDRC.6=1; 
    
    // Light an appropriate LED    
    PORTC.0=0;
    PORTC.1=0;
    PORTC.6=0;
        
    if (LED_No==1)
        PORTC.0=1;
    else if (LED_No==2) 
        PORTC.1=1;
    else  
        PORTC.6=1;    
}
//********************************************************************
void Set_AudioAlarm(int State)
{
    // Set Port C Pin 7 as output
    DDRC.7=1; 
    
    // Disable or enable the audio alarm   
    PORTC.7=0;
        
    if (State==1)
        PORTC.7=1;    
}
//********************************************************************
float* Fuzzification_WeatherCondition(int Universe[5], int Center, int Width)
{
    float Set[5];  
    int i;
    for(i=0;i<5;i++){
        if (abs(Center-Universe[i])>(Width/2))
            Set[i]=0;
        else
            Set[i]=1-((float)(2*abs(Center-Universe[i]))/Width);
    }
    
    return Set;
}
//********************************************************************
float* Fuzzification_LightIntensity(long int Universe[5], long int Center, long int Width)
{
    float Set[5];  
    int i;
    for(i=0;i<5;i++){
        if (abs(Center-Universe[i])>(Width/2))
            Set[i]=0;
        else
            Set[i]=1-((float)(2*abs(Center-Universe[i]))/Width);
    }
    
    return Set;
}
//********************************************************************
float Min(float a, float b)
{
    if (a<b)
        return a;
    else
        return b;
}
//********************************************************************
float Max(float a, float b)
{
    if (a>b)
        return a;
    else
        return b;
}
//********************************************************************
int Defuzzification(int Universe[5], float FuzzySet[5])
{
    int i;
    float s1=0, s2=0;
    for(i=0;i<5;i++){
        s1+=FuzzySet[i]*Universe[i];
        s2+=FuzzySet[i];
    } 
    
    return (int)(s1/s2);
}
//********************************************************************
void Motor_Left(int Enable, int Speed)
{
    // Set some pins of Port D as output
    DDRD.2=1;
    DDRD.3=1;
    DDRD.4=1; 
    
    // Disable or enable the motor   
    PORTD.2=Enable;    
    
    // Handle the motor    
    if (Enable==1){        
        PORTD.3=0;
        OCR1BL=Speed;
    }
}
//********************************************************************
void Motor_Right(int Enable, int Speed)
{
    // Set some pins of Port D as output
    DDRD.5=1;
    DDRD.6=1;
    DDRD.7=1; 
    
    // Disable or enable the motor   
    PORTD.7=Enable;    
    
    // Handle the motor    
    if (Enable==1){                
        OCR1AL=Speed;
        PORTD.6=0;
    }
}
//********************************************************************
void main()
{   
    // Variables
    int i, j;
    float m;         
    
    // Enable Global Interrupt
    sei();           
    
    // Timer/Counter 1 initialization
    //*** Clock source: System Clock
    //*** Clock value: 7.813 kHz
    //*** Mode: Fast PWM top=0x00FF
    //*** OC1A output: Non-Inverted PWM
    //*** OC1B output: Non-Inverted PWM
    //*** Noise Canceler: Off
    //*** Input Capture on Falling Edge
    //*** Timer Period: 32.768 ms
    //*** Output Pulse(s):
    //*** OC1A Period: 32.768 ms Width: 0 us
    //*** OC1B Period: 32.768 ms Width: 0 us
    //*** Timer1 Overflow Interrupt: Off
    //*** Input Capture Interrupt: Off
    //*** Compare A Match Interrupt: Off
    //*** Compare B Match Interrupt: Off
    TCCR1A=(1<<COM1A1) | (0<<COM1A0) | (1<<COM1B1) | (0<<COM1B0) | (0<<WGM11) | (1<<WGM10);
    TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (1<<WGM12) | (1<<CS12) | (0<<CS11) | (1<<CS10);
    TCNT1H=0x00;
    TCNT1L=0x00;
    ICR1H=0x00;
    ICR1L=0x00;
    OCR1AH=0x00;
    OCR1AL=0x00;
    OCR1BH=0x00;
    OCR1BL=0x00;

    // Timer(s)/Counter(s) Interrupt(s) initialization
    TIMSK=(0<<OCIE2) | (0<<TOIE2) | (0<<TICIE1) | (0<<OCIE1A) | (0<<OCIE1B) | (0<<TOIE1) | (0<<OCIE0) | (0<<TOIE0);
    
    while (1)
    {  
        // Increase the number of sensing data
        n++; 
            
        // Initialize and clear LCD    
        lcd_init(20); 
        lcd_clear();

        //******************************************************************************
        //********** Sensing Unit (SU)
        
        // Temperature
        Temperature=Read_ADC_Temperature()/4;
        if (Temperature<0)
            Temperature=0;
        if (Temperature>150)
            Temperature=150;
                            
        if (n<=3)
            Temperature_Average=Temperature;
        else
            Temperature_Average=Temperature_Average+(Temperature-Temperature_Average)/n;
        itoa(Temperature_Average, Str);
        
        lcd_puts("Temperature: ");
        lcd_puts(Str);      
        lcd_puts(" ^C");        
           
        // Gas
        Gas=ceil(Read_ADC_Gas()*9.765)+10;
        if (Gas<10)
            Gas=10;
        if (Gas>10000)
            Gas=10000;
                    
        if (n<=3)            
            Gas_Average=Gas;
        else
            Gas_Average=Gas_Average+(Gas-Gas_Average)/n;
        itoa(Gas_Average, Str);        
        
        lcd_puts("\n");
        lcd_puts("Gas: ");
        lcd_puts(Str);
        lcd_puts(" ppm");                          
       
        // Light
        Light=abs(ceil(Read_ADC_Light()*97.75));      
        if (n<=3)
            Light_Average=Light;
        else
            Light_Average=Light_Average+(Light-Light_Average)/n; 
                
        // Reflective
        Reflective_Left=Read_ADC_Reflective(1);         
        Reflective_Front=Read_ADC_Reflective(2);
        Reflective_Right=Read_ADC_Reflective(3);
                     
        //******************************************************************************  
        //********** Robot Alert Unit (RAU)
       
        // Weather condition                             
        WeatherCondition=((float)(0.4*((float)Temperature_Average/150))+(0.6*((float)Gas_Average/10000))+0.05)*100;
        if (WeatherCondition<0)
            WeatherCondition=0;        
        if (WeatherCondition>100)
            WeatherCondition=100;
        
        lcd_puts("\n");
        lcd_puts("********************");
        lcd_puts("WC: ");
        itoa(WeatherCondition, Str);
        lcd_puts(Str);
        lcd_puts(" %");              
                       
        // LED
        if ((WeatherCondition>=0)&&(WeatherCondition<=33))
            Set_LED(1);
        else if ((WeatherCondition>=34)&&(WeatherCondition<=66))
            Set_LED(2);
        else
            Set_LED(3);                     
                        
        // Audio alarm
        AmbientBrightness=0.86+0.001*Light_Average;
        
        if ((WeatherCondition>=67)&&(AmbientBrightness>=70))
            Set_AudioAlarm(1);
        else
            Set_AudioAlarm(0);
                               
        //******************************************************************************  
        //********** Drive & Locomotion Unit (DLU)
                                
        if (n%10==0){
            // Fuzzification                      
            Fuzzification_FuzzySet=Fuzzification_WeatherCondition(WeatherCondition_Universe, WeatherCondition, 101);
            for(i=0;i<5;i++)
                WeatherCondition_FuzzySet[i]=*(Fuzzification_FuzzySet+i);
            
            if (Light_Average<0)
                Light_Average=0;
            if (Light_Average>100000)
                Light_Average=100000;        
            Fuzzification_FuzzySet=Fuzzification_LightIntensity(LightIntensity_Universe, Light_Average, 150000);
            for(i=0;i<5;i++)
                LightIntensity_FuzzySet[i]=*(Fuzzification_FuzzySet+i);
                        
            // Inference engine        
            for(i=0;i<5;i++)
                Input_FuzzySet[i]=Min(WeatherCondition_FuzzySet[i], LightIntensity_FuzzySet[i]);        
            
            for (j=0;j<5;j++){
                m=0;            
                for (i=0;i<5;i++) 
                    m=Max(Min(Input_FuzzySet[i], R[i][j]), m); 
                               
                Speed_FuzzySet[j]=m;
            }
            
            // Defuzzification
            Speed=Defuzzification(Speed_Universe, Speed_FuzzySet);
            
            if (Speed<200)
                Speed=200;                         
        }
                   
        //******************************************************************************        
        //********** Obstacle Detection Unit (ODU)
         
        // Start both motors
        Motor_Left(1, Speed);        
        Motor_Right(1, Speed);
                
        // Scenario 1
        if ((Reflective_Left>=130)&&(Reflective_Front>108)){                    
            Motor_Right(0, Speed);            
            delay_ms(T1);                    
            Motor_Right(1, Speed);            
            delay_ms(T2);                    
            Motor_Right(0, Speed);            
            delay_ms(T1);                   
            Motor_Right(1, Speed);
            //
            Direction=0;                                                
        }
        // Scenario 2
        else if ((Reflective_Front>108)&&(Reflective_Right>=130)){                    
            Motor_Left(0, Speed);            
            delay_ms(T1);                    
            Motor_Left(1, Speed);            
            delay_ms(T2);                    
            Motor_Left(0, Speed);            
            delay_ms(T1);                   
            Motor_Left(1, Speed); 
            //
            Direction=1;
        }                                               
        // Scenario 3
        else if (Reflective_Left>=130){                    
            Motor_Right(0, Speed);                                
            delay_ms(T3);                    
            Motor_Left(0, Speed);
            Motor_Right(1, Speed);                        
            delay_ms(T3);                   
            Motor_Left(1, Speed); 
            //
            Direction=1;
        }                                               
        // Scenario 4
        else if (Reflective_Right>=130){                    
            Motor_Left(0, Speed);                                
            delay_ms(T3);                    
            Motor_Left(1, Speed);            
            Motor_Right(0, Speed);                        
            delay_ms(T3);                   
            Motor_Right(1, Speed); 
            //
            Direction=0;
        }                                               
        else if (Reflective_Front>108){
            // Scenario 5
            if (Direction==0){
                Motor_Left(0, Speed);            
                delay_ms(T1);                    
                Motor_Left(1, Speed);            
                delay_ms(T2);                    
                Motor_Left(0, Speed);            
                delay_ms(T1);                   
                Motor_Left(1, Speed); 
                //
                Direction=1;            
            }
            // Scenario 6
            else{
                Motor_Right(0, Speed);            
                delay_ms(T1);                    
                Motor_Right(1, Speed);            
                delay_ms(T2);                    
                Motor_Right(0, Speed);            
                delay_ms(T1);                   
                Motor_Right(1, Speed);
                // 
                Direction=0;           
            }                    
        }
        
        //******************************************************************************                    
        //********** Wait
        
        delay_ms(1000);        
    }
}