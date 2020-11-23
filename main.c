#include <stdio.h>
#include <string.h>
#include <pitches.h>	// Nuoteille

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/i2c/I2CCC26XX.h>

/* Board Header files */
#include "Board.h"

#include "wireless/comm_lib.h"
#include "sensors/mpu9250.h"

float ax, ay, az, gx, gy, gz;
/* Task */
#define STACKSIZE 2048
char labTaskStack[STACKSIZE];
char commTaskStack[STACKSIZE];
char displayTaskStack[STACKSIZE];
int suunta;


/* ALUSTUS*/

//tilakoneen tilat
enum state { TAUKO=1, READ_SENSOR, UPDATE, NEW_MSG, MENU };
enum state myState = MENU ;

//nappi ja ledi
static PIN_Handle buttonHandle;
static PIN_State buttonState;

/* KOMMENTOI ULOS; JOS OTAT LEDIN KÄYTTÖÖN
static PIN_Handle ledHandle;
static PIN_State ledState;
*/

//mpu
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};
//lisää mpu
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};

// Melodia
int melody[] = {
  NOTE_FS5, NOTE_FS5, NOTE_D5, NOTE_B4, NOTE_B4, NOTE_E5,
  NOTE_E5, NOTE_E5, NOTE_GS5, NOTE_GS5, NOTE_A5, NOTE_B5,
  NOTE_A5, NOTE_A5, NOTE_A5, NOTE_E5, NOTE_D5, NOTE_FS5,
  NOTE_FS5, NOTE_FS5, NOTE_E5, NOTE_E5, NOTE_FS5, NOTE_E5
};

//nuottejen kestot:
int durations[] = {
  8, 8, 8, 4, 4, 4,
  4, 5, 8, 8, 8, 8,
  8, 8, 8, 4, 4, 4,
  4, 5, 8, 8, 8, 8
};

int songLength = sizeof(melody)/sizeof(melody[0]);
int thisNote = 0;

void loop() {
	if (myState == MENU){
		// Iterate through both arrays
  // Notice how the iteration variable thisNote is created in the parenthesis
  // The for loop stops when it is equal to the size of the melody array
		for (thisNote < songLength, thisNote++);
		int duration = 1000/ durations[thisNote];
		tone(8, melody[thisNote], duration);
		// pause between notes
		//int pause = duration * 1.3;
		//delay(pause);
  }
}

//Nappi configuraatio
PIN_Config buttonConfig[] = {
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE, // Nelj�n vakion TAI-operaatio
   PIN_TERMINATE // Taulukko lopetetaan aina t�ll� vakiolla
};

/*
//Ledi koniguraatio KOMMENTOI TÄMÄKIN ULOS, JOS OTAT LEDIN KÄYTTÖÖN
PIN_Config ledConfig[] = {
   Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE // Taulukko lopetetaan aina t�ll� vakiolla
};
*/

//buzzerille TÄNNE KOKO AUDIO

//Napille taski
void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
   if(myState == TAUKO) {
      myState = READ_SENSOR;
   }
   else if(myState == MENU) {
    myState = READ_SENSOR;
   }
   else if(myState == UPDATE) {
        myState = TAUKO;
   }
}


//Näytölle taski
Void displayTask(UArg arg0, UArg arg1) {
	//testi printtaus
    System_printf("pilluako \n");
            System_flush();

    Display_Params params;
    Display_Params_init(&params);
    params.lineClearMode = DISPLAY_CLEAR_NONE;

    Display_Handle displayHandle = Display_open(Display_Type_LCD, &params);
    tContext *pContext = DisplayExt_getGrlibContext(displayHandle);

    while (1) {
    	//tässä menu, josta peli alkaa
        if (myState == MENU){
            Display_print0(displayHandle, 5, 1, "PAINA YLINTA");
            Display_print0(displayHandle, 6, 1, "NAPPIA ");
            Display_print0(displayHandle, 7, 1, "ALOITTAAKSESI");
            Display_print0(displayHandle, 8, 1, "PELI");
        }
        //jos peli alkaa, niin sensori alkaa lukemaan ja näytttö tyhjenee
        else if (myState == READ_SENSOR){
    	    Display_clear(displayHandle);
    	}

        //Task_sleep(1 * 1000000/Clock_tickPeriod);

        //Pikselinuolet, jos saa labTask:ssa tiedon että on ylitetty liikkene kynnysraja
        else if ((myState == UPDATE ) && (suunta == 1)){
    	    GrLineDraw(pContext,90,48,30,48);
    	    GrLineDraw(pContext,90,48,60,20);      //VASEN
    	    GrLineDraw(pContext,90,48,60,76);
    	    GrFlush(pContext);
    		}
        else if ((myState == UPDATE) && (suunta == 2)){
            GrLineDraw(pContext,6,48,66,48);
            GrLineDraw(pContext,6,48,36,76);       // OIKEA
            GrLineDraw(pContext,6,48,36,20);
            GrFlush(pContext);
    	}
        else if ((myState == UPDATE) && (suunta == 3)){
            GrLineDraw(pContext,48,6,48,66);
            GrLineDraw(pContext,48,6,76,36);       // ALAS
            GrLineDraw(pContext,48,6,20,36);
            GrFlush(pContext);
        }
        else if ((myState == UPDATE) && (suunta == 4)){
            GrLineDraw(pContext,48,90,48,30);
            GrLineDraw(pContext,48,90,20,60);      // YLÖS
            GrLineDraw(pContext,48,90,76,60);
            GrFlush(pContext);
        }
    }

    	   // Once per second
    //Task_sleep(1 * 1000000/Clock_tickPeriod);
    }


//mpu:lle
Void labTaskFxn(UArg arg0, UArg arg1) {

    float ax, ay, az, gx, gy, gz;
    char message[80];


	I2C_Handle i2cMPU; // INTERFACE FOR MPU9250 SENSOR
	I2C_Params i2cMPUParams;
    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL) {
        System_abort("Error Initializing I2CMPU\n");
    }
    // JTKJ: Tehtävä 2. Sensorin alustus kirjastofunktiolla ennen datan lukemista
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);
    // Tietoa, kalibroituuko MPU
	Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();

    System_printf("MPU9250: Setup and calibration...\n");
	System_flush();

	mpu9250_setup(&i2cMPU);

	System_printf("MPU9250: Setup and calibration OK\n");
	System_flush();

    I2C_close(i2cMPU);

    while (1) {

        if (myState == READ_SENSOR){
        i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
	    if (i2cMPU == NULL) {
	        System_abort("Error Initializing I2CMPU\n");
	    }
	    // Haetaan kiihtyvyyden ja gy
		mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);


	    I2C_close(i2cMPU);

	    if (ay < -0.3){
	    	suunta = 3;
	    	myState = UPDATE;
	    }
	    if (ay > 0.3){
	    	suunta = 4;
	    	myState = UPDATE;

	    }
	    if (ax < -0.3){
	        suunta = 1;
	    	myState = UPDATE;

	    }
	    if (ax > 0.3){
	    	suunta = 2;
	    	myState = UPDATE;
	    }
	    else{
		sprintf(message,"%f,  %f  \n", ax, ay);
		System_printf(message);
		System_flush();
}
}
	    // WAIT 1s

    	Task_sleep(100000 / Clock_tickPeriod);
	// MPU9250 POWER OFF
	// Because of loop forever, code never goes here



}

      PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);
    	// JTKJ: Tehtävä 3. Tulosta sensorin arvo merkkijonoon ja kirjoita se ruudulle
		// JTKJ: Exercise 3. Store the sensor value as char array and print it to the display
}
    	// JTKJ: Tehtävä 4. Lähetä CSV-muotoinen merkkijono UARTilla
		// JTKJ: Exercise 4. Send CSV string with UART


/* Communication Task */
/*Void commTaskFxn(UArg arg0, UArg arg1) {

   char payload[16]; // viestipuskuri
   uint16_t senderAddr;

    // Radio to receive mode
	int32_t result = StartReceive6LoWPAN();
	if(result != true) {
		System_abort("Wireless receive mode failed");
	}

    while (1) {



        if (GetRXFlag() == true && myState == TAUKO) {
             // Tilasiirtymä IDLE -> NEW_MSG
            myState = NEW_MSG;

            // Tilan toiminnallisuus
            memset(payload,0,16);
            Receive6LoWPAN(&senderAddr, payload, 16);
            System_printf(payload);
            System_flush();
    		// Handled the received message..

            myState = TAUKO;
        }
    }
}

*/
Int main(void) {

    // Task variables
	Task_Handle labTaskHandle;
	Task_Params labTaskParams;

	//Task_Handle commTask;
	//Task_Params commTaskParams;

    Task_Handle displayTaskHandle;
    Task_Params displayTaskParams;

    // Initialize board
    Board_initGeneral();
    Board_initI2C();
    //Init6LoWPAN();



    //Nappi
/*    PIN_Config buttonConfig[] = {
       Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE, // Nelj�n vakion TAI-operaatio
       PIN_TERMINATE // Taulukko lopetetaan aina t�ll� vakiolla
    };

    // Ledipinni
    PIN_Config ledConfig[] = {
       Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
       PIN_TERMINATE // Taulukko lopetetaan aina t�ll� vakiolla
    };




    // Ledi k�ytt��n ohjelmassa
    ledHandle = PIN_open( &ledState, ledConfig );
    if(!ledHandle) {
       System_abort("Error initializing LED pin\n");
    }

    // Painonappi k�ytt��n ohjelmassa
    buttonHandle = PIN_open(&buttonState, buttonConfig);
    if(!buttonHandle) {
       System_abort("Error initializing button pin\n");
    }


    // JTKJ: Teht�v� 1. Rekister�i painonapille keskeytyksen k�sittelij�funktio
    // JTKJ: Exercise 1. Register the interrupt handler for the button
    // Painonapille keskeytyksen k�sittellij�
    if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
       System_abort("Error registering button callback function");
    }

*/

buttonHandle = PIN_open(&buttonState, buttonConfig);
if(!buttonHandle) {
   System_abort("Error initializing button pin\n");
}

// Painonapille keskeytyksen käsittellijä
if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
   System_abort("Error registering button callback function");
}



    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
    	System_abort("Pin open failed!");
    }

    /* Task */
    Task_Params_init(&labTaskParams);
    labTaskParams.stackSize = STACKSIZE;
    labTaskParams.stack = &labTaskStack;
    labTaskParams.priority=2;

    labTaskHandle = Task_create( (Task_FuncPtr)labTaskFxn, &labTaskParams, NULL);
    if (labTaskHandle == NULL) {
    	System_abort("Task create failed!");
    }

    /* Communication Task */

  //  Init6LoWPAN(); // This function call before use!

   /* Task_Params_init(&commTaskParams);
    commTaskParams.stackSize = STACKSIZE;
    commTaskParams.stack = &commTaskStack;
    commTaskParams.priority=1;

    commTask = Task_create(commTaskFxn, &commTaskParams, NULL);
    if (commTask == NULL) {
    	System_abort("Task create failed!");
    } */

	//display task call
	Task_Params_init(&displayTaskParams);
    displayTaskParams.stackSize = STACKSIZE;
    displayTaskParams.stack = &displayTaskStack;
    displayTaskParams.priority=2;

    displayTaskHandle = Task_create( (Task_FuncPtr) displayTask, &displayTaskParams, NULL);
    if (displayTaskHandle == NULL) {
    	System_abort("Task create failed!");
    }



    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
