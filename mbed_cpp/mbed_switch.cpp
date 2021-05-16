#include "mbed.h"
#include "C12832.h"
#include "LM75B.h"
 
#define AppV 0x01
#define HarV 0x01
#define MaxTemp 125 
#define MinTemp -55
  
C12832 lcd(p5, p7, p6, p8, p11);                                        //configure LCD
LM75B tmp       (p28,p27);                                              //Temperature Sensor
RawSerial xbee(p9,p10);                                                 //Set Serial to XBee
Serial pc(USBTX, USBRX);                                                //Set Serial to PC.
DigitalOut led2 (LED2);                                                 //configure onboard LEDs
DigitalOut led1 (LED1); 
DigitalOut LED (LED4);        
volatile char x,y,z,len,sum;                                            //some useful internal variables
volatile int length, size, temp, check;
char packet[60];
char PacketAddr[10];
char Opacket[60];
char AI[8] = {0x7E,0x00,0x04,0x08,0x21,0x41,0x49,0x4C};                 //create AT command to querey network joined?
char MY[8] = {0x7E,0x00,0x04,0x08,0x21,0x4D,0x59,0x30};                 //create AT command to get my network ID.
char SL[8] = {0x7E,0x00,0x04,0x08,0x21,0x53,0x4C,0x37};                 //create AT command to get 64bit LSB.
char SH[8] = {0x7E,0x00,0x04,0x08,0x21,0x53,0x48,0x3B};                 //create AT command to get 64bit MSB.
char CB[8] = {0x7E,0x00,0x04,0x08,0x21,0x43,0x42,0x51};                 //Create AT command to leave network.
char LNw[2];
char Addr[8];
char pay[101];
char Buffer[101];
char EP, DEP, SEP, seqNum, cmdID, frmType;
char atID[2];
char Clu[2];
char Pro[2];
char Temperature[2];
char Man[3] = {'A','R','M'};                                            //Product manufacturer
char Dev[10] = {'N','X','P','L','P','C','1','7','6','8'};               //Device Name
 
void packet_interupt();                                                 //Create subroutine to deal with incoming packet.
void Psend(char API_packet[]);                                          //Create subroutine to send API commands  
void PBuild(char FrameAddr[], char sEP, char dEP, char Profile[], char Cluster[], char Payload[], char PaySize); //function to create packets to send.
void Builder();                                                         //Get usefull infromation from packets.
void checksum();                                                        //Create the checksum for the packet.
void DevAnnc();                                                         //function to send the device announce command.
void SimpleDesc();                                                      //Function to create the Simple description packet
void ActiveEPReq();                                                     //Function to create the Active end point request.
void ClusterBasic();                                                    //Function to create the Cluster Basic.
void Temp();                                                           //Function to process on off commands.
 
int main(){
    lcd.cls();                                                          //clear lcd screen.
    lcd.locate(0,1);                                                    //locate the cursor.
    lcd.printf("         Joining network...");                          //Print to lcd screen
    memset(packet, 0, sizeof(packet));                                  //Clear array
    
    xbee.attach(&packet_interupt, Serial::RxIrq);                       //attach interupt when recieving serial information.
    
    while(packet[5] != 0){                                              //While Xbee has not joined a network
        wait(1);                                                        //Wait 0.1 second.
        Psend(AI);                                                      //Send network joined status again.
        while(check == 0)
        {}                                                              //Read data.
    }
    lcd.cls();                                                          //Clear lcd screen.
    lcd.locate(0,1);                                                    //Locate the cursor.
    lcd.printf("        Connected");                                    //Print to lcd screen.
    
    check = 0;
    Psend(MY);                                                          //Send AT command to get the 16bit network address.
    while(check == 0)                                                   //Wait for packet.
    {}
    LNw[0] = packet[5];                                                 //Transfer 16 bit address payload.
    LNw[1] = packet[6];                                                 //Transfer 16 bit address payload.
    
    check = 0;
    Psend(SH);                                                          //Send AT command to get the 64bit address MSB.
    while(check == 0)                                                   //Wait for packet.
    {}
    Addr[0] = packet[5];                                                //Transfer 64 bit address payload.
    Addr[1] = packet[6];                                                //Transfer 64 bit address payload.
    Addr[2] = packet[7];                                                //Transfer 64 bit address payload.
    Addr[3] = packet[8];                                                //Transfer 64 bit address payload.
    
    check = 0;
    Psend(SL);                                                          //Send AT command to get the 64bit address LSB.
    while (check == 0)                                                  //Wait for packet.
    {}
    Addr[4] = packet[5];                                                //Transfer 64 bit address payload.
    Addr[5] = packet[6];                                                //Transfer 64 bit address payload.
    Addr[6] = packet[7];                                                //Transfer 64 bit address payload.
    Addr[7] = packet[8];                                                //Transfer 64 bit address payload.
    DevAnnc();                                                          //Send Device Announce.
    
while(1) {   
    lcd.locate(0,21);                                                   //Locate LCD cursor.
    lcd.printf("Packets recieved = %d", z);                             //Print to LCD display.
    check = 0;
    while(check == 0)                                                   //Wait for packet.
    {}
    if (packet[16] == 0x00){                                            //If packet profile = 16.
        if (packet[14] == 0x04){                                        //If packet cluster = 14.
            lcd.locate(0,11);                                           //Locate LCD cursor.
            lcd.printf("       simple dec");    
            SimpleDesc();                                               //Process Simple Desciption request.
        }
        else if (packet[14] ==0x05){                                    //If packet cluster = 15.
            lcd.locate(0,11);                                           //Locate LCD display
            lcd.printf("       Active EP req");
            ActiveEPReq();                                              //Process active EP Request.
        }
        else{
            lcd.locate(0,11);                                           //Locate LCD cursor.
            lcd.printf("     Unknown Packet");                          //Print to lcd screen.
            }
        }
    else{                                                               //Else if packet profile is not = 00.
        if (packet[14] == 0x00){                                        //If packet Cluster = 00.
            lcd.locate(0,11);                                           //Locate LCD cursor.
            lcd.printf("       Cluster Basic");
            ClusterBasic();                                             //Process Cluster Basic request.
        }
        else if (packet[14] == 0x02){                                   //Else if packet profile is not = 06.
            lcd.locate(0,11);                                           //Locate LCD cursor.
            lcd.printf("        Temperature    ");     
            Temp();                                                    //Process On/Off.
        }
        else{
            lcd.locate(0,11);                                           //Locate the cursor.
            lcd.printf("     Unknown Packet");                          //Print to lcd screen.
            }
        }
    }                                                                   //end of while(1)
}
 
/********************************* Packet interupt ***************************************
When a new Xbee packet is revieved it is sent to this function to be processed.
 
*****************************************************************************************/
void packet_interupt(){                                                 //packet interupt begins when incoming data sensed on the rx line.
 
if (xbee.readable())                                                    //If data is available
    x=xbee.getc();                                                      //Get data
if (x==0x7E){                                                           //Test for start of Frame.
    led2 = 1;                                                           //Set indicator LED2 to on.
    z ++;                                                               //Increment packet counter.
    memset(packet, 0, sizeof(packet));                                  //Clear packet buffer.
    while(xbee.readable() == 0);                                        //If data is available.
        x = xbee.getc();                                                //Get data MSB length.
    while(xbee.readable() == 0);                                        //If data is available.
        len = xbee.getc();                                              //Get data LSB length.
        x = 0;                                                          //Counter to 0.
        sum = 0;                                                        //Set checksum calculator to 0.
        while(x < (len + 1)){                                           //Fill packet variable with entire API packet.
            while (xbee.readable() == 0);                               //If data is available.
            packet[x] = xbee.getc();                                    //Get data and place it in the data array.
            sum = (sum + packet[x]);                                    //Add value to checksum calculation.
            x++;
            }    
        sum = (0xFF - ( sum & 0xFF));                                   //Finish calculating the checksum.
        if (packet[x] == sum ){                                         //If the checksum and the calculated checksum are equal.
            if (packet[0] == 0x91){                                     //If packet ID = 0x91
                Builder();                                              //Extract packet information
                check = 1;
            }
            else if(packet[0] == 0x88){                                  //Else if packet ID = 0x88.
                check = 1;
            }
            else
                memset(packet, 0, sizeof(packet));                      //Clear array
        }
        else{
            memset(packet, 0, sizeof(packet));                          //Clear array
        }
    led2 = 0;                                                           //Set indicator LED2 to off.
    }   
}
/************************************* Builder ******************************************
Extract all the important inforamtion from the packet and assoicate it to the relevant variable.
*****************************************************************************************/
void Builder()
{
    x = 0;                                                              //Set the counter to 0.
    while (x < 10){         //send the command
        PacketAddr[x] = packet[(x+1)];                                  //Copy the packets 16 and 64bit address.
        x ++;
    }
    DEP = packet[11];                                                   //Get the source endpoint.
    SEP = packet[12];                                                   //Get the destination endpoint.
    Clu[0] = packet[13];                                                //Get Cluster ID MSB.
    Clu[1] = packet[14];                                                //Get Cluster ID LSB.
    Pro[0] = packet[15];                                                //Get profile ID MSB.
    Pro[1] = packet[16];                                                //Get profile ID LSB.
    x = 0;                                                              //Set counter to 0.
    while ((x+18) < len){
        pay[x] = packet[(x+18)];                                        //Copy packet payload.
        x++;
    }
}
/************************************ PBuild ******************************************
Organise the information from other functions into the correct packet format. 
 
*****************************************************************************************/
void PBuild(char FrameAddr[], char sEP, char dEP, char Profile[], char Cluster[], char Payload[],char PaySize){
    
    size = 0;                                                           //Set size to 0.
    x = 0;                                                              //Set counter to 0.
    memset(Opacket, 0, sizeof(Opacket));                                //Clear output buffer.
 
    size = 20 +(PaySize);                                               //Find out the size of the out going packet.
 
    Opacket[0] = 0x7E;                                                  //Packet start bit.
    Opacket[1] = 0x00;                                                  //Packet length MSB.
    Opacket[2] = size;                                                  //Packet length LSB.
    Opacket[3] = 0x11;                                                  //Packet Type
    Opacket[4] = 0x00;                                                  //No response needed.
    Opacket[5] = FrameAddr[0];                                          //64 bit address High
    Opacket[6] = FrameAddr[1];  
    Opacket[7] = FrameAddr[2];
    Opacket[8] = FrameAddr[3];
    Opacket[9] = FrameAddr[4];                                          //64 bit address Low
    Opacket[10] = FrameAddr[5];
    Opacket[11] = FrameAddr[6];
    Opacket[12] = FrameAddr[7];
    Opacket[13] = FrameAddr[8];                                         //16 bit PAN address
    Opacket[14] = FrameAddr[9];
    Opacket[15] = sEP;                                                  //Source Endpoint
    Opacket[16] = dEP;                                                  //Destinaton Endpoint
    Opacket[17] = Cluster[0];                                           //Cluster ID
    Opacket[18] = Cluster[1];
    Opacket[19] = Profile[0];                                           //Profile ID
    Opacket[20] = Profile[1];
    Opacket[21] = 0x00;                                                 //Broadcast radius
    Opacket[22] = 0x00;                                                 //Trasmit option bitfield
    while (x < PaySize){
        Opacket[(23 + x)] = Payload[x];                                 //Copy packet payload
        x ++;
    }
    
    checksum();                                                         //Create the check sum.
    Psend(Opacket);                                                     //Send the packet.
}
/************************************ Checksum ******************************************
Create the checksum for outgoing packets.
 
*****************************************************************************************/
void checksum(){
    
    y = 0;                                                              //Set Alternatie counter to 0
    sum = 0;
    while (y < (Opacket[2])){                                           //Create the checksum.
        sum = (sum + Opacket[y + 3]);                           
        y++;
    }
    Opacket[23 + x] = (0xFF - ( sum & 0xFF));
}
/************************************* Psend ********************************************
Send the packet out serially to the Xbee module.
 
*****************************************************************************************/
void Psend(char API_packet[]){
    
    led1 = 1;                                                           //Set onboard LED1 on for diagnostics
    length = (API_packet[2] + 4);                                       //Calaculate packet length using packet length identifier and add the start byte, 2 byte length and checksum.
    x = 0;
    while (x < length){                                                 //Send the command
        xbee.putc(API_packet[x]);
        x ++;
    }
    led1 = 0;                                                           //Set diagnostics LED1 to off
}
/********************************* Device Announce **************************************
Send a Device announce packet when joining or re joining and network.
For more informaion see ZigBee spec page 109 
*****************************************************************************************/
void DevAnnc(){
    
    char AnncFrame[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFE};
    char AnncPayload[12];                                               //Create char array for payload.
    char DevClu[2] = {0x00, 0x13};                                      //Create char array for Cluster ID.
    char DevPro[2] = {0x00, 0x00};                                      //Create char array for Profile ID.
    AnncPayload[0] = 0x22;                                              //Set seqence ID to 0x22.
    AnncPayload[1] = LNw[1];                                            //Copy 16bit PAN address little endian.
    AnncPayload[2] = LNw[0];
    AnncPayload[3] = Addr[7];                                           //Copy 64bit address little endian.
    AnncPayload[4] = Addr[6];
    AnncPayload[5] = Addr[5];
    AnncPayload[6] = Addr[4];
    AnncPayload[7] = Addr[3];
    AnncPayload[8] = Addr[2];
    AnncPayload[9] = Addr[1];
    AnncPayload[10] = Addr[0]; 
    AnncPayload[11] = 0x8C;                                             //Set device parameters.
 
    PBuild(AnncFrame,0x00,0x00,DevPro,DevClu, AnncPayload, 12);         //Send packet to builder.
}
/******************************** Simple Description ************************************
Send a response to a Simple Description Request.
For more informaion see ZigBee spec page 159. 
*****************************************************************************************/
void SimpleDesc(){
    
    EP = pay[3];                                                        //Get endpoint from payload.
    char DescClu[2] = {0x80,0x04};                                      //Create char array for cluster ID.
    char DescPro[2] = {0x00,0x00};                                      //Create char array for profile ID.
 
    if( EP == 0x76){                                                    //If Endpoint = 38.
        memset(Buffer, 0, sizeof(Buffer));                              //Clear array                                                                                
        Buffer[0] = pay[0];                                             //Set Transcation Seq number to match inbound packets seq number
        Buffer[1] = 0x00;                                               //Status $00 = success Table 2.93 on page 159 of ZBSpec 
        Buffer[2] = pay[1];                                             //Set Network address little endian order
        Buffer[3] = pay[2]; 
        Buffer[4] = 0x0E;                                               //Length in bytes of the Simple Descriptor to Follow
        Buffer[5] = 0x76;                                               //Endpoint of the simple descriptor Table 2.38 on page 88 of ZBSpec
        Buffer[6] = 0x04;                                               //Application Profile ID 2 Bytes Little endian. $0104 = Home Automation Profile
        Buffer[7] = 0x01;
        Buffer[8] = 0x02;                                               //Device type 2 Bytes Little endian, $0002 = On/Off Output see page 42 of ZigBee Home Automation Profile
        Buffer[9] = 0x00;                                                             
        Buffer[10] = 0x00;                                              //App Dev Version 4bits + reserved 4bits  
        Buffer[11] = 0x02;                                              //Input cluster count in this case we only have $02 input clusters
        Buffer[12] = 0x00;                                              //Input cluster list 2 bytes each little endian. $0000 = Basic Cluster 
        Buffer[13] = 0x00;
        Buffer[14] = 0x02;                                              //Output cluster 2 bytes each little endian. $0006 = On / Off Cluster
        Buffer[15] = 0x04;
        Buffer[16] = 0x00;                                              //Output cluster list. No output cluster.
        PBuild(PacketAddr,0x00,0x00,DescPro,DescClu, Buffer, 0x11);
    }
    
    else{
        memset(Buffer, 0, sizeof(Buffer));                              //Clear array.                                                                               
        Buffer[0] = pay[0];                                             //Set Transcation Seq number to match inbound packets seq number.
        Buffer[1] = 0x82;                                               //Status $82 = Invalid_EP page 212 of ZigBee Specification.  
        Buffer[2] = pay[1];                                             //Set Network address little endian order.
        Buffer[3] = pay[2];
        Buffer[4] = 0x00;                                               //Length in bytes of simple descriptor to follow.
        PBuild(PacketAddr,0x00,0x00,DescPro,DescClu, Buffer, 0x05); 
    }
}
/******************************** Active EP Request ************************************
Send a response to and Active EP Request.
For more informaion see ZigBee spec page 161. 
*****************************************************************************************/
void ActiveEPReq(){
    
    char EPReqClu[2] = {0x80, 0x05};                                    //Create char array for Cluster.
    char EPReqPro[2] = {0x00, 0x00};                                    //Create char array for Profile.
    memset(Buffer, 0, sizeof(Buffer));                                  //Clear array.                                                                            
    Buffer[0] = pay[0];                                                 //Set Transcation Seq number to match inbound packets seq number.
    Buffer[1] = 0x00;                                                   //Status $00 = success Table 2.93 on page 159 of ZBSpec.
    Buffer[2] = pay[1];                                                 //Set Network address little endian order.
    Buffer[3] = pay[2];
    Buffer[4] = 0x01;                                                   //Active end point count in this case 1.
    Buffer[5] = 0x76;                                                   //Endpoint 38.
    PBuild(PacketAddr,0x00,0x00,EPReqPro,EPReqClu, Buffer, 0x06); 
}
/********************************** Cluster Basic ***************************************
Process a Basic Cluster request.
For more informaion see ZigBee cluster library page 78 
*****************************************************************************************/
void ClusterBasic(){
    
    cmdID = 0;                                                          //Set command ID to 0.
    seqNum = 0;                                                         //Set seqence number to 0.
    memset(Buffer, 0, sizeof(Buffer));                                  //Clear array.
    memset(atID, 0, sizeof(atID));                                      //Clear array.
    
    seqNum = pay[1];                                                    //Copy seqence number from payload.
    cmdID = pay[2];                                                     //Copy command ID from payload.
    atID[0] = pay[3];                                                   //Copy attribute ID LSB from payload.
    atID[1] = pay[4];                                                   //Copy attribute ID MSB from payload.
    
    if ((cmdID == 0x00) && (atID[0] == 0x01) && (atID[1] == 0x00)){     //If Application version is requested.
        Buffer[0] = 0x18;                                               //Frame control direction is server to client.
        Buffer[1] = seqNum;                                             //Reply with seqence number from request.
        Buffer[2] = 0x01;                                               //Command indetifier = 1, Read attribute response.
        Buffer[3] = 0x01;                                               //Attribute Identfier (2 bytes) field being reported.
        Buffer[4] = 0x00;       
        Buffer[5] = 0x00;                                               //status 00 success.
        Buffer[6] = 0x20;                                               //Attribute data type 0x20 = unsigned 8 bit integer.
        Buffer[7] = AppV;                                               //Application version.
        PBuild(PacketAddr,SEP,DEP,Pro,Clu, Buffer, 0x08);               //Send data to packet builder.
        } 
        
    if ((cmdID == 0x00) && (atID[0] == 0x03) && (atID[1] == 0x00)){     //If Hardware version is requeted.
        Buffer[0] = 0x18;                                               //Frame control direction is server to client.
        Buffer[1] = seqNum;                                             //Reply with seqence number from request.
        Buffer[2] = 0x01;                                               //Command indetifier = 1, Read attribute response.
        Buffer[3] = 0x03;                                               //Attribute Identfier (2 bytes) field being reported.
        Buffer[4] = 0x00;
        Buffer[5] = 0x00;                                               //status 00 success.
        Buffer[6] = 0x20;                                               //Attribute data type 0x20 = unsigned 8 bit integer.
        Buffer[7] = HarV;                                               //Hardware version.
        PBuild(PacketAddr,SEP,DEP,Pro,Clu, Buffer, 0x08);               //Send data to packet builder.
        }
 
    if ((cmdID == 0x00) && (atID[0] == 0x04) && (atID[1] == 0x00)){     //If device Manufacturer is requested
        Buffer[0] = 0x18;                                               //Frame control direction is server to client.
        Buffer[1] = seqNum;                                             //Reply with seqence number from request.
        Buffer[2] = 0x01;                                               //Command indetifier = 1, Read attribute response.
        Buffer[3] = 0x04;                                               //Attribute Identfier (2 bytes) field being reported.
        Buffer[4] = 0x00;
        Buffer[5] = 0x00;                                               //status 00 success.
        Buffer[6] = 0x42;                                               //Attribute data type 0x42 = character string.
        Buffer[7] = sizeof(Man);                                        //Size of string to follow.
        x = 0;
        while( x < (sizeof(Man))){                                      //Send sting byte by byte.
            Buffer[(x+8)] = Man[x];
            x++;
            }
        PBuild(PacketAddr,SEP,DEP,Pro,Clu, Buffer, (0x08 + Buffer[7])); //Send data to packet builder.
        }
        
    if ((cmdID == 0x00) && (atID[0] == 0x05) && (atID[1] == 0x00)){     //If Device devoloper is requested.
        Buffer[0] = 0x18;                                               //Frame control direction is server to client.
        Buffer[1] = seqNum;                                             //Reply with seqence number from request.
        Buffer[2] = 0x01;                                               //Command indetifier = 1, Read attribute response.
        Buffer[3] = 0x05;                                               //Attribute Identfier (2 bytes) field being reported.
        Buffer[4] = 0x00;
        Buffer[5] = 0x00;                                               //status 00 success.
        Buffer[6] = 0x42;                                               //Attribute data type 0x42 = character string.
        Buffer[7] = sizeof(Dev) ;                                       //Size of string to follow.
        x = 0;
        while( x < (sizeof(Dev))){                                      //Send sting byte by byte.
            Buffer[(x+8)] = Dev[x];
            x++;
            }
        PBuild(PacketAddr,SEP,DEP,Pro,Clu, Buffer, (0x08 + Buffer[7])); //Send data to packet builder.
        } 
}
/******************************** Switch Cluster ************************************
Process and respond to a Switch cluster command.
For more informaion see ZigBee Cluster library page 125. 
*****************************************************************************************/
 
void Temp()
{
    cmdID = 0;                                                          //Set command ID to 0.
    seqNum = 0;                                                         //Set seqence number to 0.
    frmType = 0;                                                        //Set frame type to 0.
    memset(Buffer, 0, sizeof(Buffer));                                  //Clear array 
    memset(atID, 0, sizeof(atID));                                      //Clear array 
    
    frmType = pay[0];                                                   //Get frame type from payload.
    frmType = frmType & 0x01;                                           //Bitwise & with 0x03 to make sure you are looking at the first 2 bits.
    seqNum = pay[1];                                                    //Get Seqence Number for payload.
    cmdID = pay[2];                                                     //Get command ID from payload.
    atID[0] = pay[3];                                                   //Get attribute ID MSB.
    atID[1] = pay[4]; 
    
    if ((frmType == 0x00) && (cmdID == 0x00) && (atID[0] == 0x00)){     //Read current temperature.
        temp = tmp.read();
        temp = temp - 8;
        Temperature[0] = (temp >> 8) & 0xFF;
        Temperature[1] = temp & 0xFF;                                  
    } 
    
    if ((frmType == 0x00) && (cmdID == 0x00) && (atID[0] == 0x01)){     //Read minimum temperature.
        Temperature[0] = (MinTemp >> 8) & 0xFF;
        Temperature[1] = MinTemp & 0xFF;        
    }
     
    if ((frmType == 0x00) && (cmdID == 0x00) && (atID[0] == 0x02)){     //Read maximum temperature.
        Temperature[0] = (MaxTemp >> 8) & 0xFF;
        Temperature[1] = MaxTemp & 0xFF;                                
    } 
 
    Buffer[0] = 0x18;                                                   //Frame control direction is server to client.
    Buffer[1] = seqNum;                                                 //Reply with seqence number from request.
    Buffer[2] = 0x01;                                                   //Command indetifier = 1, Read attribute response.
    Buffer[3] = atID[0];                                                //Attribute Identfier (2 bytes) field being reported.
    Buffer[4] = atID[1];
    Buffer[5] = 0x00;                                                   //Status 00 = success.
    Buffer[6] = 0x29;                                                   //Attribute data type 29 = signed 16-bit interger.
    Buffer[7] = Temperature[1];                                         //MSB temperature.
    Buffer[8] = Temperature[0];                                         //LSB temperature.
    PBuild(PacketAddr,SEP,DEP,Pro,Clu, Buffer, 0x09);                   //Send to packet builder.    
}