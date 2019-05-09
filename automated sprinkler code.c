//Amr, Nihil, Steven
#include <asf.h>
// function declaration
//tc functions
void simple_clock_init(void);
void configure_tc4(void);
void enable_clock(int, uint32_t);
//adc functions
void configure_adc(int);
void configure_adc_clock(void);
int read_adc(void);
int water_check(int);
//general functions
void pin_setup(void);
void port_setup(void);
void wait(int t);
//keypad functions
void getKey(void);
void debounce(void);
void keySweep(void);
void release(void);

// Pointers
Adc *adcPtr = (Adc*)ADC; //initialize ADC pointer
Tc *tcPtr4 = (Tc*)TC4;     //initialize TC4  pointer

Port *ports = PORT_INSTS;
PortGroup *portAs = (PortGroup *)PORT;
PortGroup *portBs = (PortGroup *)PORT+ 1;


int debounceCount = 0;
volatile int state = 0;
char keyInput = 0;
char keyLayout[4][4] = {{'D','#',0,'*'},{'C',9,8,7},{'B',6,5,4},{'A',3,2,1}};
int i;
int j;
int k;

int main(void){

    simple_clock_init();
	configure_tc4();
	port_setup();
	pin_setup();

    while(1){
		switch(state){
			case 0: keySweep(); break;
			case 1: getKey(); break;

		}
	}

}


void TC4_Handler(void){ //


	j++;

	if(j == 1000){// every 33secs at 1000, .03hz //now 40s lol


		configure_adc(0x0B);
		int x = read_adc();
		x = water_check(x);

		configure_adc(0x0C);
		int x1 = read_adc();
		x1 = water_check(x1);

		configure_adc(0x0D);
		int x2 = read_adc();
		x2 = water_check(x2);// returns 1 if no or low amount of water and 0 if water

		configure_adc(0x0E);
		int x3 = read_adc();
		x3 = water_check(x3);

		configure_adc(0x0F);
		int x4 = read_adc();
		x4 = water_check(x4);

		while(1){
			if(x == 1){
				portBs->OUTSET.reg = PORT_PB11;
			}
			if(x1 == 1){
				portBs->OUTSET.reg = PORT_PB16;
			}
			if(x2 == 1){
				portBs->OUTSET.reg = PORT_PB17;
			}
			if(x3 == 1){
				portBs->OUTSET.reg = PORT_PB22;
			}
			if(x4 == 1){
				portBs->OUTSET.reg = PORT_PB23;
			}
			/*if((x&x1&x2&x3&x4)==0){
				portBs->OUTCLR.reg = PORT_PB11|PORT_PB16|PORT_PB17|PORT_PB22|PORT_PB23; // set all off
				break;
			}*/

			wait(2000);
			portBs->OUTCLR.reg = PORT_PB11|PORT_PB16|PORT_PB17|PORT_PB22|PORT_PB23; // set all off

			break;
		}
		k++;
		j=0;


	}


    tcPtr4->COUNT8.INTFLAG.bit.OVF = 1;

}

void enable_clock(int mask, uint32_t id){
	PM->APBCMASK.reg |= (1 << mask);			// Clock enable
	GCLK->CLKCTRL.reg = id;						// GCLK selection ID
	GCLK->CLKCTRL.reg |= 0x1u << 14;			// Enable bit
}

void configure_tc4(void){

	enable_clock(12, 0x1C);

	tcPtr4->COUNT8.CTRLA.reg = 0;				//disable tc2
    while(tcPtr4->COUNT8.STATUS.bit.SYNCBUSY);
    tcPtr4->COUNT8.CTRLA.bit.MODE = 0x1;		// 8 bit Mode
    tcPtr4->COUNT8.CTRLA.bit.WAVEGEN = 0x2;		// Normal PWM
    tcPtr4->COUNT8.CTRLA.bit.PRESCALER = 0x7;	// GCLK_TC/1024
    tcPtr4->COUNT8.CTRLA.bit.PRESCSYNC = 0x1;	// reset on prescaler clock
    tcPtr4->COUNT8.INTENSET.bit.OVF = 1;		// enables overflow interrupt
	tcPtr4->COUNT8.PER.reg = 255;				// max value for count8
	while(tcPtr4->COUNT8.STATUS.bit.SYNCBUSY);
    tcPtr4->COUNT8.CTRLA.reg |= 1<<1;			// enable tc2
	while(tcPtr4->COUNT8.STATUS.bit.SYNCBUSY);

	NVIC->ISER[0] = (1 << 19);					// enable tc4 interrupt
	NVIC->IP[4] |= (0 << 31) | (1 << 30);		// Priority Order 01, Second highest priority

}

void pin_setup(void){

	//mosfet gate pins
	portBs->DIRSET.reg = PORT_PB11|PORT_PB16|PORT_PB17|PORT_PB22|PORT_PB23;// set as output
	portBs->OUTCLR.reg = PORT_PB11|PORT_PB16|PORT_PB17|PORT_PB22|PORT_PB23;// set low

	//keypad pins
	portAs->DIRSET.reg = PORT_PA04 | PORT_PA05 | PORT_PA06 | PORT_PA07; //setting the columns of keypads as output to power
	portAs->OUTSET.reg = PORT_PA04 | PORT_PA05 | PORT_PA06 | PORT_PA07; //set high
	portAs->DIRCLR.reg = PORT_PA19 | PORT_PA18 | PORT_PA17 | PORT_PA16; //set up as inputs
	portAs->OUTCLR.reg = PORT_PA19 | PORT_PA18 | PORT_PA17 | PORT_PA16; //set low

	portAs->PINCFG[16].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
	portAs->PINCFG[17].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
	portAs->PINCFG[18].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
	portAs->PINCFG[19].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
}

void port_setup(void){
	//adc pins

	portBs->DIRCLR.reg = PORT_PB03|PORT_PB04|PORT_PB05|PORT_PB06|PORT_PB07;
	portBs->OUTSET.reg = PORT_PB03|PORT_PB04|PORT_PB05|PORT_PB06|PORT_PB07;
	portAs->DIRCLR.reg = PORT_PA03;
	portAs->OUTSET.reg = PORT_PA03;

	portAs->PINCFG[3].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
	portBs->PINCFG[3].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
	portBs->PINCFG[4].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
	portBs->PINCFG[5].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
	portBs->PINCFG[6].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;
	portBs->PINCFG[7].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;


	//adc pins
	portAs->PMUX[1].bit.PMUXO = 0x1;		//refer to pg345 data sheet//PMUX[n] and PMUXEN[pin#] and 2n=pin#
	portAs->PINCFG[3].bit.PMUXEN = 1 ;	//refer to pg347 data sheet
	portBs->PMUX[1].bit.PMUXO = 0x1;		//refer to pg345 data sheet//PMUX[n] and PMUXEN[pin#] and 2n=pin#
	portBs->PINCFG[3].bit.PMUXEN = 1 ;	//refer to pg347 data sheet
	portBs->PMUX[2].bit.PMUXE = 0x1;		//refer to pg345 data sheet//PMUX[n] and PMUXEN[pin#] and 2n=pin#
	portBs->PINCFG[4].bit.PMUXEN = 1 ;	//refer to pg347 data sheet
	portBs->PMUX[2].bit.PMUXO = 0x1;		//refer to pg345 data sheet//PMUX[n] and PMUXEN[pin#] and 2n=pin#
	portBs->PINCFG[5].bit.PMUXEN = 1 ;	//refer to pg347 data sheet
	portBs->PMUX[3].bit.PMUXE = 0x1;		//refer to pg345 data sheet//PMUX[n] and PMUXEN[pin#] and 2n=pin#
	portBs->PINCFG[6].bit.PMUXEN = 1 ;	//refer to pg347 data sheet
	portBs->PMUX[3].bit.PMUXO = 0x1;		//refer to pg345 data sheet//PMUX[n] and PMUXEN[pin#] and 2n=pin#
	portBs->PINCFG[7].bit.PMUXEN = 1 ;	//refer to pg347 data sheet
}

int read_adc(void){ //checked
	// start the conversion, see 0x0C in the table in Section 28.7
	adcPtr->SWTRIG.reg = 1 << 1; //start convo
	while(!(adcPtr->INTFLAG.bit.RESRDY));//polling wait for conversion to be available
	return(adcPtr->RESULT.reg); 					//insert register where ADC store value
}

void configure_adc_clock(void){
	PM->APBCMASK.reg |= 1 << 16; 			// PM_APBCMASK adc is in the 16 bit position pg 148
	GCLK->CLKCTRL.reg = 0x1E; 			    // Setup in the CLKCTRL register
	GCLK->CLKCTRL.reg |= 0x1 << 14; 		// enable it.
}

void configure_adc(int mask){

	configure_adc_clock();

	adcPtr->CTRLA.reg = 0 << 1;				// ADC block is disabled
	adcPtr->REFCTRL.bit.REFSEL = 0x3;		// VREFA = PA03
	adcPtr->AVGCTRL.bit.SAMPLENUM = 0x0;	// 1 sample
	adcPtr->AVGCTRL.bit.ADJRES = 0x0;		// divide by 1
	adcPtr->CTRLB.bit.RESSEL = 0x0;			// 12 bit result
	while(adcPtr->STATUS.reg & ADC_STATUS_SYNCBUSY) {};
	adcPtr->CTRLB.bit.PRESCALER = 0x4;		// divide by 64
	while(adcPtr->STATUS.reg & ADC_STATUS_SYNCBUSY) {};
	adcPtr->INPUTCTRL.bit.MUXPOS = mask;	// mask is input
	while(adcPtr->STATUS.reg & ADC_STATUS_SYNCBUSY) {};
	adcPtr->INPUTCTRL.bit.MUXNEG = 0x18;	// ground
	while(adcPtr->STATUS.reg & ADC_STATUS_SYNCBUSY) {};
	adcPtr->INPUTCTRL.bit.GAIN = 0x2;		// 4x gain
	while(adcPtr->STATUS.reg & ADC_STATUS_SYNCBUSY) {};
	adcPtr->CTRLA.reg |= 1 << 1;			// Enable ADC

}

void getKey(){ //scan keypad
	int j;
	for(j=0; j<=3; j++){
		portAs->OUTCLR.reg = (1 << (j+4));
		int k;
		for(k=0;k<=3;k++){
			if(portAs->IN.reg & (1 << (k+16))){
				keyInput = keyLayout[j][k];
				state =0;
				return;
			}
		}
		state = 0;
		portAs->OUTSET.reg = (1 << j);
	}
}

void debounce(){
	while(portAs->IN.reg & (PORT_PA16|PORT_PA17|PORT_PA18|PORT_PA19)){
		debounceCount++;
		if(debounceCount == 1000){
			state = 1;
			debounceCount = 0;
			return;
		}
	}
	debounceCount = 0;
}

void keySweep(){
	portAs->OUTSET.reg = PORT_PA07|PORT_PA06|PORT_PA05|PORT_PA04;
	int i;
	for(i = 16; i<= 19; i++){
		if(portAs->IN.reg & (1 << i)){
			debounce();
			break;
		}
	}
	portAs->OUTCLR.reg = PORT_PA07|PORT_PA06|PORT_PA05|PORT_PA04;
}


int water_check(int adc){
	if(adc < 1700){
		return 1;
	}
	return 0;
}

void simple_clock_init(void){
    /* Various bits in the INTFLAG register can be set to one at startup.
       This will ensure that these bits are cleared */

    SYSCTRL->INTFLAG.reg = SYSCTRL_INTFLAG_BOD33RDY|SYSCTRL_INTFLAG_BOD33DET|SYSCTRL_INTFLAG_DFLLRDY;

    system_flash_set_waitstates(0);      //Clock_flash wait state =0

    SYSCTRL_OSC8M_Type temp = SYSCTRL->OSC8M;      /* for OSC8M initialization  */

    temp.bit.PRESC    = 0;            // no divide, i.e., set clock=8Mhz  (see page 170)
    temp.bit.ONDEMAND = 1;            // On-demand is true
    temp.bit.RUNSTDBY = 0;            // Standby is false

    SYSCTRL->OSC8M = temp;

    SYSCTRL->OSC8M.reg |= 0x1u << 1;      // SYSCTRL_OSC8M_ENABLE bit = bit-1 (page 170)

    PM->CPUSEL.reg = (uint32_t)0;        // CPU and BUS clocks Divide by 1  (see page 110)
    PM->APBASEL.reg = (uint32_t)0;         // APBA clock 0= Divide by 1  (see page 110)
    PM->APBBSEL.reg = (uint32_t)0;         // APBB clock 0= Divide by 1  (see page 110)
    PM->APBCSEL.reg = (uint32_t)0;         // APBB clock 0= Divide by 1  (see page 110)

    PM->APBAMASK.reg |= 01u<<3;           // Enable Generic clock controller clock (page 127)

    /* Software reset Generic clock to ensure it is re-initialized correctly */
	//here
    GCLK->CTRL.reg = 0x1u << 0;           // Reset gen. clock (see page 94)
    while (GCLK->CTRL.reg & 0x1u ) {  /* Wait for reset to complete */ }

    // Initialization and enable generic clock #0
	//here
    *((uint8_t*)&GCLK->GENDIV.reg) = 0;      // Select GCLK0 (page 104, Table 14-10)

    GCLK->GENDIV.reg  = 0x0100;           // Divide by 1 for GCLK #0 (page 104)

    GCLK->GENCTRL.reg = 0x030600;          // GCLK#0 enable, Source=6(OSC8M), IDC=1 (page 101)
}

void wait(int t){
    int count = 0;
    while (count < t*1000){
        count++;
    }
}