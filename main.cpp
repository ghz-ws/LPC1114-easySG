#include "mbed.h"
BufferedSerial pc(P1_7, P1_6);  //TX, RX
SPI spi(P0_9, P0_8, P0_6);    //mosi, miso, sclk
DigitalOut LE1(P1_8);
DigitalOut LE2(P1_9);

//uart read
int read();
const char delimiter='\r';
const int buf_size=10;
char buf[buf_size];
int number;

//MCLK freq.
const int mclk=67108864;
int freq1=1000,freq2=1000;
int pha1=0,pha2=0;
int freq_reg1,pha_reg1,freq_reg2,pha_reg2;
int lsb1,msb1,phaf1,lsb2,msb2,phaf2;
int state=0;
int i,k;
int pow10;
int val;

int main(){
    thread_sleep_for(2000);
    printf("Booting... Use CR code!!\r\n");
    spi.format(16,2);   //spi mode setting. 2byte transfer, mode 2
    LE1=1;              //cs init.
    LE2=1;              //cs init.
    while(true) {
        printf("Ready!! 1=CH1 Freq, 2=CH1 Phase, 3=CH2 Freq, 4=CH2 Phase\r\n");
        number=read();//read state
        if(number==1){
            if(buf[0]-48==1){
                printf("CH1 Frequency?\r\n");
                state=1;
            }else if(buf[0]-48==2){
                printf("CH1 Phase?\r\n");
                state=2;
            }else if(buf[0]-48==3){
                printf("CH2 Frequency?\r\n");
                state=3;
            }else if(buf[0]-48==4){
                printf("CH2 Phase?\r\n");
                state=4;
            }else{
                printf("ERR!!\r\n");
                state=0;
            }
        }else{
            printf("ERR!!\r\n");
            state=0;
        }
        
        //state switch
        if(state!=0){
            number=read();
            val=0;
            for(i=0;i<number;++i){
                pow10=1;
                for(k=0;k<number-i-1;++k){
                    pow10=10*pow10;
                }
                val=val+(buf[i]-48)*pow10;
            }
            if(state==1){
                freq1=val;
            }else if(state==2){
                pha1=val;
            }else if(state==3){
                freq2=val;
            }else if(state==4){
                pha2=val;
            }
            
            //range check
            if(freq1>mclk/2){
                printf("CH1 Frequency is Out of range.\r\n");
                freq1=1000;
            }
            if(freq2>mclk/2){
                printf("CH2 Frequency is Out of range.\r\n");
                freq2=1000;
            }
            if(pha1>=360){
                printf("CH1 Phase is Out of range.\r\n");
                pha1=0;
            }
            if(pha2>=360){
                printf("CH2 Phase is Out of range.\r\n");
                pha2=0;
            }
            
            //SPI write
            freq_reg1=268435456/mclk*freq1;
            pha_reg1=4096*pha1/360;
            lsb1=(freq_reg1&0x3FFF)+0x4000;
            msb1=(freq_reg1>>14)+0x4000;
            phaf1=pha_reg1+0xC000;
            freq_reg2=268435456/mclk*freq2;
            pha_reg2=4096*pha2/360;
            lsb2=(freq_reg2&0x3FFF)+0x4000;
            msb2=(freq_reg2>>14)+0x4000;
            phaf2=pha_reg2+0xC000;
            printf("CH1 Freq=%d Hz,Phase=%d deg.\r\nCH2 Freq=%d Hz,Phase=%d deg.\r\n\r\n",freq1,pha1,freq2,pha2);
            
            //ch1 write
            LE1=0;
            spi.write(0x2100);
            spi.write(lsb1);
            spi.write(msb1);
            spi.write(phaf1);
            LE1=1;
            
            //ch2 write
            LE2=0;
            spi.write(0x2100);
            spi.write(lsb2);
            spi.write(msb2);
            spi.write(phaf2);
            LE2=1;
            
            //ch1&ch2 reset
            LE1=0;
            LE2=0;
            spi.write(0x2000);
            LE1=1;
            LE2=1;
        }
    }
}

//uart delimiter read func.
int read(){
    int i;
    char local[1];
    for (i=0;i<buf_size;++i){
        pc.read(local,1);
        buf[i]=local[0];
        if(local[0]==delimiter)break;
    }
    return i;
}
