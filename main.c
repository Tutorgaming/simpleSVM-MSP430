/*
 * UART MSP430F5529 SERIAL STORE
 * by C3MX.
 * BAUDRATE : 9600
 * SMCLK : 1.5Mhz
 */

#include <msp430f5529.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char inputArray[11];
volatile int ptr;
volatile int i = 0 ;
volatile int input_enable=1;


typedef struct _svm_node_
{
	int index;
	double value;
} svm_node;

void initialize(){
	 // P3.3,4 = USCI_A1 TXD/RXD (USB TX RX on PORT 3.3,3.4)
	 P4SEL |= BIT5+BIT4;
	 // Reset UCA1CTL1
	 UCA1CTL1 |= UCSWRST;
	 // Select Source For UART => SMCLK (1.5Mhz)
	 UCA1CTL1 |= UCSSEL_3;
	 // Set Baud rate
	 UCA1BR1 = 0x00; UCA1BR0 = 0x6D; // 1.5MHz/9600=0x006D
	 UCA1MCTL |= UCBRS_2 + UCBRF_0; // Modulation UCBRSx=2, UCBRFx=0 (FROM DATASHEET)
	 UCA1CTL1 &= ~UCSWRST; // Initialize the state machine
	 // Enable USCI_A1 RX interrupt
	 UCA1IE |= UCRXIE;

	 //LED Initialization
	 P1DIR |= 0x01;
	 P4DIR |= 0x80;
	 P1OUT = 0b00000000;
	 P4OUT = 0b00000000;
}

void uart_putchar(char input){
		while (!(UCA1IFG&UCTXIFG));
		UCA1TXBUF = input;
}
void uart_newline(){
	uart_putchar('\n');
	uart_putchar('\r');
}

void sendACK(){
	uart_putchar('@');
	uart_newline();
	ptr=0;
	input_enable = 1;
	inputArray[0] = ' ';
	inputArray[1] = ' ';
	inputArray[2] = ' ';
	inputArray[3] = ' ';
	inputArray[4] = ' ';
	inputArray[5] = ' ';
	inputArray[6] = ' ';
	inputArray[7] = ' ';
	inputArray[8] = ' ';
	inputArray[9] = ' ';
	inputArray[10] = ' ';
	P1OUT = 0b00000000;
	P4OUT = 0b00000000;
}

void println(char *input , int input_size){
	int i = input_size;
	for(; i>0 ; i--){
		uart_putchar(input[input_size-i]);
	}
	uart_putchar('\n');
	uart_putchar('\r');
}

void main(void){
		//Stop Watchdog timer
			WDTCTL = WDTPW + WDTHOLD;
		//Initialize Register for UART and LED
			initialize();
 	 	//TRANSMISSION TEST
			char * message = "HELLO WORLD :D ";
			println(message,strlen(message));

		//Receiver Interrupt
			__bis_SR_register(GIE); //interrupts enabled
			__no_operation();

		//RECEIVE PARAMETERS

		while(input_enable == 1);
			int nr_class = atoi(inputArray);
		sendACK(); //Send @ as ACK signal to get the next Value

		while(input_enable == 1);
			int total_sv = atoi(inputArray);
		sendACK();

		int rhosize = nr_class*(nr_class-1)/2;
        double *rho = (double *)malloc((rhosize)*sizeof(double));
		unsigned int it=0;
		for(; it < rhosize ; it++){
			while(input_enable == 1);
				rho[it] = atof(inputArray);
			sendACK();
		}

		it=0;
		int *label = (int *)malloc((nr_class)*sizeof(int));
		for(; it < nr_class ; it++){
			while(input_enable == 1);
				label[it] = atoi(inputArray);
			sendACK();
		}

		it=0;
		//model->nSV = Malloc(int,nr_class);
		int *nr_sv = (int*)malloc((nr_class)*sizeof(int));
		for(; it < nr_class ; it++){
			while(input_enable == 1);
			nr_sv[it] = atoi(inputArray);
			sendACK();
		}

		//For Model
		//double **sv_coef = (double**) malloc((total_sv)*sizeof(double*));
		double sv_coef[2][9];
		svm_node SV[9][3];

		//For Classification
		double *kvalue = (double *)malloc ((total_sv)*sizeof(double));
		int *start = (int *)malloc ((nr_class)*sizeof(int));
		int *vote = (int *)malloc ((nr_class)*sizeof(int));
		int NUM_OF_SV_COEF_ELEMENT = nr_class -1 ;

		it =0 ;
//		for(; it< total_sv ; it++){
//			sv_coef[it] = (double*)malloc(NUM_OF_SV_COEF_ELEMENT*sizeof(double));
//		}

		int it_i=0;
		int it_j=0;
		for(;it_i<total_sv ;it_i++){

				//SV_REF
				while(input_enable ==1);
				int size_of_sv_i = atoi(inputArray);
				sendACK();

			//SV_COEF
			for(it_j=0;it_j<NUM_OF_SV_COEF_ELEMENT;it_j++){
				while(input_enable ==1);
				sv_coef[it_j][it_i] = atof(inputArray);
				sendACK();
			}
			int lastValue;
			for(it = 0;it<size_of_sv_i-1;it++){
				while(input_enable ==1);
				SV[it_i][it].index = (int)atoi(inputArray);
				sendACK();

				while(input_enable ==1);
				SV[it_i][it].value = (double)atof(inputArray);
				sendACK();
			}
		}
		P4OUT = 0b10000000;

}

// RECEIVER ( INTERRUPT ON RECEIVE )
// This is the RX UART ISR and is entered when the RX buffer is full
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void){
	//Input
	if(input_enable){
		if(UCA1RXBUF == ','){
			input_enable = 0;
			P1OUT = 0b00000001;
		}else{
			//while (!(UCA1IFG&UCTXIFG));
			//UCA1TXBUF = UCA1RXBUF;
			inputArray[ptr++] = UCA1RXBUF;
		}
	}
}


