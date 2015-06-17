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
volatile char inputArray[10];
volatile int ptr;
volatile int i = 0 ;
volatile int input_enable=1;

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
	inputArray[0] = '0';
	P1OUT = 0b00000000;
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
		int it=0;
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


