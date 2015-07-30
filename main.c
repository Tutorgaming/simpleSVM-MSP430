/*
 * LIBSVM LINEAR KERNEL - "Model" C_SVM Implementation
 * on MSP430F5529
 * Version 1.0
 * by Theppasith N.
 * ( TU-Chemnitz Summer Internship Program )
 * Serial Comm Detail
 * 	BAUDRATE : 9600
 * 	SMCLK    : 1.5Mhz
 * 	Credited to : LIBSVM library.
 */

#include <msp430f5529.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char inputArray[11]; // Buffer For Numbers
double sum_temp = 0; // used in DOT Function
double sum = 0;		// Voting Process
unsigned int expect =0; // Expecting Result
volatile int result = -1; // Result

volatile int ptr; //All Purpose Pointer
volatile int i = 0 ; //All Purpose Pointer
volatile int input_enable=1; //Input Enable
volatile double multiply_temp = 0; // used in Multiply Function

//DATA STRUCTURE FOR MODEL
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

//UART SERIAL COMMUNICATION //////////////////
void uart_putchar(char input){
	while (!(UCA1IFG&UCTXIFG));
	UCA1TXBUF = input;
}
void uart_newline(){
	uart_putchar('\n');
	uart_putchar('\r');
}
void println(char *input , int input_size){
	int i = input_size;
	for(; i>0 ; i--)uart_putchar(input[input_size-i]);
	uart_putchar('\n');
	uart_putchar('\r');
}

char * itoa(int input,char str[11]){
	sprintf(str,"%d",input);
	return str;
}

void printstring(char *input){
	int i;
	for(i = 0 ; i < strlen(input); i++){
		uart_putchar(input[i]);
	}
}

/////////////////////////////////////////////

void sendACK(){
	uart_putchar('@');
	uart_newline();
	ptr=0;
	input_enable = 1;
	//CLEAR BUFFER FOR NEXT INPUT
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
	//TURN LED OFF
		P1OUT = 0b00000000;
}

double multiply(double *left , double *right){
		multiply_temp =0;
		multiply_temp = (float)(*left) * (float)(*right);
		return multiply_temp;
}

double dot(const svm_node *px, const svm_node *py){
		sum_temp = 0;
		while(px->index != -1 && py->index != -1){
			if(px->index == py->index){
				sum_temp += px->value * py->value;
				++px;
				++py;
			}
			else{
				if(px->index > py->index)
					++py;
				else
					++px;
			}
		}
		return sum_temp;
}

void blinking(int times){
	int t;
	for (t = 0 ; t < times ; t++){
		P1OUT = 0b00000001;
		_delay_cycles(500000);
		P1OUT = 0b00000000;
		_delay_cycles(500000);

	}
}

void main(void){
	//////////////////////////////////////////
	//////       INITIALIZATION          /////
	//////////////////////////////////////////
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

	//////////////////////////////////////////
	//////       RECEIVE PARAMETERS      /////
	//////////////////////////////////////////

		// Number of INPUT_DIMENSION
			while(input_enable == 1);
				int dimension = atoi(inputArray);
			sendACK(); //Send @ as ACK signal to get the next Value
		// Number of CLASSES
			while(input_enable == 1);
				int nr_class = atoi(inputArray);
			sendACK();
		// Number of TOTAL_Support Vectors
			while(input_enable == 1);
				int total_sv = atoi(inputArray);
			sendACK();
		// RHO Array (for Voting process)
			int rhosize = nr_class*(nr_class-1)/2;
			double *rho = (double *)malloc((rhosize)*sizeof(double));
		unsigned int it=0; //ALL PURPOSE POINTER
				for(; it < rhosize ; it++){
					while(input_enable == 1);
						rho[it] = atof(inputArray);
					sendACK();
				}
		// Class Name Tag Array
			int *label = (int *)malloc((nr_class)*sizeof(int));
			for(it=0; it < nr_class ; it++){
				while(input_enable == 1);
					label[it] = atoi(inputArray);
				sendACK();
			}
		// model->nSV (for Voting Process 'start[]')
			int *nr_sv = (int*)malloc((nr_class)*sizeof(int));
			for(it=0; it < nr_class ; it++){
				while(input_enable == 1);
				nr_sv[it] = atoi(inputArray);
				sendACK();
			}

		//Support Vector and its Coefficient

		svm_node **SV = (svm_node **) malloc ((total_sv)*sizeof(svm_node*));
		int z = 0;
		for(it=0 ; it < total_sv ; it++){
			SV[it] = (svm_node*) malloc ((dimension+1)*sizeof(svm_node));
			for(z=0;z < dimension+1 ; z++){
				SV[it][z].index = 0;
				SV[it][z].value = 0.0;
			}
		}

		double **sv_coef = (double**) malloc((nr_class-1)*sizeof(double*)); //double **sv_coef = (double**) malloc((2)*sizeof(double*));
		for(it=0; it < (nr_class-1);it++){
			sv_coef[it] = (double*) malloc (total_sv * sizeof(double));
			for(z =0; z<total_sv ; z++){
				sv_coef[it][z] = 0.00;
			}
		}

		it =0 ;
		int NUM_OF_SV_COEF_ELEMENT = nr_class -1 ;
		int it_i=0;
		int it_j=0;
		for(it_i=0;it_i<total_sv ;it_i++){

			//LOOP REFERENCE
			while(input_enable ==1);
			int size_of_sv_i = atoi(inputArray);
			sendACK();

			// Inputs for SV_COEFFICIENT
			for(it_j=0;it_j<NUM_OF_SV_COEF_ELEMENT;it_j++){
				while(input_enable ==1);
				sv_coef[it_j][it_i] = atof(inputArray);
				sendACK();
			}

			// Inputs for Support Vectors (SV[][]) in 'svm_node' format
			for(it = 0;it<size_of_sv_i;it++){
				//index
				while(input_enable ==1);
				SV[it_i][it].index = (int)atoi(inputArray);
				sendACK();
				//value
				while(input_enable ==1);
				SV[it_i][it].value = (double)atof(inputArray);
				sendACK();
			}
		}

		//GREEN LED SHOW THAT MODEL HAS FINISHED LOADING
				P4OUT = 0b10000000; //Green On
				P1OUT = 0b00000000; //Red Off

		//////////////////////////////////
		//       TEST VALUE INPUT
		//////////////////////////////////

	while(1){
		// Create "X" - Unknown Data
		svm_node *x = (svm_node *) malloc((dimension+1)*sizeof(svm_node));
		// Receive new test input (All dimensions)
		for(it = 0;it<dimension;it++){
			x[it].index = it+1;
			while(input_enable ==1);
				x[it].value = (double)atof(inputArray);
			sendACK();
		}
		// Last dimension is -1 (end flag - not used in calculation)
		x[it].index = -1;
		x[it].value = 0.000000;

		//////////////////////////////////
		// LINEAR Classification Process
		//////////////////////////////////

		// Decision Value (For voting process)
		double *dec_values = (double *) malloc((rhosize)*sizeof(double));

		// K-Value ( Calculation between input vector X and SUPPORT VECTOR )
		// this is the DOT method. (C_SVM - Linear Kernel)
		double *kvalue = (double *)malloc ((total_sv)*sizeof(double));
		for(it=0;it<total_sv;it++)
			kvalue[it] = dot(x,SV[it]); //Dotted result of the unknown "X" input to every Support Vectors.

		// Array to store the pre-calculated position of pointer to be.
		int *start = (int *)malloc ((nr_class)*sizeof(int));
		start[0] = 0;
		for(it=1;it<nr_class;it++)
			start[it] = start[it-1]+nr_sv[it-1];

		// VOTING ARRAY (decided which class that the 'X' data should be)
		int *vote = (int *)malloc ((nr_class)*sizeof(int));
		for(it=0;it<nr_class;it++)
			vote[it] = 0;

		// VOTING PROCESS
		int p=0;
		double *coef1,*coef2;
		int si,sj,ci,cj,k,vote_max_idx;
		for(it=0;it<nr_class;it++){
			for(it_j = it+1;it_j<nr_class;it_j++){
				sum = 0;
				//Iteration Things
				si = start[it];
				sj = start[it_j];
				ci = nr_sv[it];
				cj = nr_sv[it_j];

				coef1 = sv_coef[it_j-1];
				coef2 = sv_coef[it];

				for(k=0;k<ci;k++)
					sum+= multiply(&coef1[si+k],&kvalue[si+k]); //sum += coef1[si+k] * kvalue[si+k];
				for(k=0;k<cj;k++)
					sum+= multiply(&coef2[sj+k],&kvalue[sj+k]); //sum += coef2[sj+k] * kvalue[sj+k];
				sum -= rho[p];
				dec_values[p] = sum;
				//Voting From Decision Value
				if(dec_values[p] > 0)
					++vote[it];
				else
					++vote[it_j];
				p++;
			}
		}

		//Find Max Voting Value
		vote_max_idx = 0;
		for(i=1;i<nr_class;i++)
			if(vote[i] > vote[vote_max_idx])vote_max_idx = i;

		//Free The Memories
		free(kvalue);
		free(start);
		free(vote);
		free(dec_values);

		result = label[vote_max_idx];
		//Blink the LED according to the class number

			uart_newline();
			char *string = "Return = ";
			char buf_int[11];
			printstring(string);
			printstring(itoa(result,buf_int));
			uart_putchar('+'); //End Seperator
			uart_newline();

		//Blinking the result according to CLASS TAG ( 0 means no blink )
		blinking(result);
	}
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
			inputArray[ptr++] = UCA1RXBUF;
		}
	}
}


