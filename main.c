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
double sum_temp = 0;
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

double dot(const svm_node *px, const svm_node *py){
		sum_temp = 0;

		while(px->index != -1 && py->index != -1)
		{
			if(px->index == py->index)
			{
				sum_temp += px->value * py->value;
				++px;
				++py;
			}
			else
			{
				if(px->index > py->index)
					++py;
				else
					++px;
			}
		}
		return sum_temp;
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
		svm_node SV[9][4];

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
			for(it = 0;it<size_of_sv_i;it++){
				while(input_enable ==1);
				SV[it_i][it].index = (int)atoi(inputArray);
				sendACK();

				while(input_enable ==1);
				SV[it_i][it].value = (double)atof(inputArray);
				sendACK();
			}
		}
		P4OUT = 0b10000000;



		//For Classification

		double *dec_values = (double *) malloc((rhosize)*sizeof(double));
		svm_node x[3];
		//RESULT==2 1:0.680000 2:0.884000 3:0.000800
		x[0].index = 1;
		x[0].value = 0.680000;
		x[1].index = 2;
		x[1].value = 0.884000;
		x[2].index = 3;
		x[2].value = 0.000800;


		//RESULT == 1 1:0.768000 2:0.690000 3:0.034800
//		x[0].index = 1;
//		x[0].value = 0.768000;
//		x[1].index = 2;
//		x[1].value = 0.694000;
//		x[2].index = 3;
//		x[2].value = 0.000800;

		sum_temp = 0;
		//double *kvalue = Malloc(double,l);
		double *kvalue = (double *)malloc ((total_sv)*sizeof(double));
		for(it=0;it<total_sv;it++)
			kvalue[it] = dot(x,SV[it]); //Kernel::k_function(x,model->SV[i],model->param);
		sum_temp = 0;
		//int *start = Malloc(int,nr_class);
		int *start = (int *)malloc ((nr_class)*sizeof(int));
		start[0] = 0;
		for(it=1;it<nr_class;it++)
			start[it] = start[it-1]+nr_sv[it-1];

		//int *vote = Malloc(int,nr_class);
		int *vote = (int *)malloc ((nr_class)*sizeof(int));
		for(it=0;it<nr_class;it++)
			vote[it] = 0;

		int p=0;
		double *coef1,*coef2;
		for(it=0;it<nr_class;it++)
			for(it_j = it+1;it_j<nr_class;it_j++)
			{
				double sum = 0;
				int si = start[it];
				int sj = start[it_j];
				int ci = nr_sv[it];
				int cj = nr_sv[it_j];

				int k;
				coef1 = &sv_coef[it_j-1];
				coef2 = &sv_coef[it];
				for(k=0;k<ci;k++)
					sum += coef1[si+k] * kvalue[si+k];
				for(k=0;k<cj;k++)
					sum += coef2[sj+k] * kvalue[sj+k];
				sum -= rho[p];
				dec_values[p] = sum;

				if(dec_values[p] > 0)
					++vote[it];
				else
					++vote[it_j];
				p++;
			}

		int vote_max_idx = 0;
		for(i=1;i<nr_class;i++)
			if(vote[i] > vote[vote_max_idx])
				vote_max_idx = i;

		//free(kvalue);
		//free(start);
		//free(vote);
		int result = label[vote_max_idx];
		free(dec_values);
		if(result ==99)P1OUT = 0b00000001;


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


