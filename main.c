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
volatile int i = 0 ,result = 9;
volatile int input_enable=1;
volatile double multiply_temp = 0;

unsigned int selector = 0;

//data structure for model
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
	for(; i>0 ; i--)uart_putchar(input[input_size-i]);
	uart_putchar('\n');
	uart_putchar('\r');
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

		//NUMBER OF DIMENSION
			while(input_enable == 1);
				int dimension = atoi(inputArray);
			sendACK(); //Send @ as ACK signal to get the next Value

		//NUMBER OF CLASSES
			while(input_enable == 1);
				int nr_class = atoi(inputArray);
			sendACK(); //Send @ as ACK signal to get the next Value

		//NUMBER OF TOTAL SUPPORT VECTORS
			while(input_enable == 1);
				int total_sv = atoi(inputArray);
			sendACK();

		//RHO Array
			int rhosize = nr_class*(nr_class-1)/2;
			double *rho = (double *)malloc((rhosize)*sizeof(double));
			unsigned int it=0;
				for(; it < rhosize ; it++){
					while(input_enable == 1);
						rho[it] = atof(inputArray);
					sendACK();
				}

		//CLASS LABEL TAG
		it=0;
			int *label = (int *)malloc((nr_class)*sizeof(int));
			for(; it < nr_class ; it++){
				while(input_enable == 1);
					label[it] = atoi(inputArray);
				sendACK();
			}

		//NR_SV
		it=0;
		//model->nSV = Malloc(int,nr_class);
			int *nr_sv = (int*)malloc((nr_class)*sizeof(int));
			for(; it < nr_class ; it++){
				while(input_enable == 1);
				nr_sv[it] = atoi(inputArray);
				sendACK();
			}

		int NUM_OF_SV_COEF_ELEMENT = nr_class -1 ;

		//Support Vector and its Coefficient

		//double sv_coef[2][9];
		//svm_node SV[9][4];

		svm_node **SV = (svm_node **) malloc ((total_sv)*sizeof(svm_node*));
		int z = 0;
		for(it=0 ; it < total_sv ; it++){
			SV[it] = (svm_node*) malloc ((dimension+1)*sizeof(svm_node));
			//SV[it] = (svm_node*) malloc ((4)*sizeof(svm_node));
			for(z=0;z < dimension+1 ; z++){
				//for(z=0;z < 4; z++){
				SV[it][z].index = 0;
				SV[it][z].value = 0.0;
			}
		}

		z=0;
		it=0;
		double **sv_coef = (double**) malloc((nr_class-1)*sizeof(double*)); //double **sv_coef = (double**) malloc((2)*sizeof(double*));
		for(it=0; it < (nr_class-1);it++){
			sv_coef[it] = (double*) malloc (total_sv * sizeof(double));
			for(z =0; z<total_sv ; z++){
				sv_coef[it][z] = 0.00;
			}
		}

		it =0 ;
		int it_i=0;
		int it_j=0;
		for(it_i=0;it_i<total_sv ;it_i++){

			//LOOP REFERENCE
			while(input_enable ==1);
			int size_of_sv_i = atoi(inputArray);
			sendACK();

			//SV_COEF
			for(it_j=0;it_j<NUM_OF_SV_COEF_ELEMENT;it_j++){
				while(input_enable ==1);
				sv_coef[it_j][it_i] = atof(inputArray);
				sendACK();
			}

			//SV
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
		//GREEN LED SHOW THE INPUT PROCESS HAS BEEN ENDED.
		P4OUT = 0b10000000;

		//LINEAR Classification Process
		double *dec_values = (double *) malloc((rhosize)*sizeof(double));


		P1OUT = 0b00000000;

		//TEST VALUE INPUT
		svm_node x[3];
		//RESULT==2 1:0.680000 2:0.884000 3:0.000800
		if(selector == 2){
		x[0].index = 1;
		x[0].value = 0.680000;
		x[1].index = 2;
		x[1].value = 0.884000;
		x[2].index = 3;
		x[2].value = 0.000800;
		}
//		//RESULT == 1 1:0.768000 2:0.690000 3:0.034800
		if(selector == 1){
		x[0].index = 1;
		x[0].value = 0.768000;
		x[1].index = 2;
		x[1].value = 0.694000;
		x[2].index = 3;
		x[2].value = 0.000800;
		}
		//0 1:0.528000 2:0.668000 3:0.834800
		if(selector == 0){
		x[0].index = 1;
		x[0].value = 0.528000;
		x[1].index = 2;
		x[1].value = 0.668000;
		x[2].index = 3;
		x[2].value = 0.834800;
		}


		double *kvalue = (double *)malloc ((total_sv)*sizeof(double));
		for(it=0;it<total_sv;it++)
			kvalue[it] = dot(x,SV[it]);


		int *start = (int *)malloc ((nr_class)*sizeof(int));
		start[0] = 0;
		for(it=1;it<nr_class;it++)
			start[it] = start[it-1]+nr_sv[it-1];

		//VOTING ARRAY (decided which class that the 'X' data should be)
		int *vote = (int *)malloc ((nr_class)*sizeof(int));
		for(it=0;it<nr_class;it++)
			vote[it] = 0;

		//VOTING PROCESS
		int p=0;
		double *coef1,*coef2,sum;
		int si,sj,ci,cj,k,vote_max_idx;

		for(it=0;it<nr_class;it++){
			for(it_j = it+1;it_j<nr_class;it_j++){
				sum = 0;
				si = start[it];
				sj = start[it_j];
				ci = nr_sv[it];
				cj = nr_sv[it_j];

				coef1 = sv_coef[it_j-1]; //coef1 = &sv_coef[it_j-1]; //ARRAY IMPLEMENTATION
				coef2 = sv_coef[it]; //coef2 = &sv_coef[it];    //ARRAY IMPLEMENTATION

				for(k=0;k<ci;k++)
					sum+= multiply(&coef1[si+k],&kvalue[si+k]); //	sum += coef1[si+k] * kvalue[si+k];
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
		}
		//Find Max Voting Value
		vote_max_idx = 0;
		for(i=1;i<nr_class;i++)
			if(vote[i] > vote[vote_max_idx])vote_max_idx = i;

		free(kvalue);
		free(start);
		free(vote);
		free(dec_values);

		result = label[vote_max_idx];

		//TEST EXPECTING RESULT
		if(result ==selector /*&& SV[0][0].index ==1*/)P1OUT = 0b00000001; //SHOW RED LED when result is correct :D
		else P1OUT = 0b00000000;

		P2REN = 0x02;         // Turn on pull up resistor to push button
		P2IE |= BIT1;         // P1.3 interrupt enabled
		while(1);

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

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void){
	sleep(50);
	selector++;
	P2IFG &= ~BIT1; // P1.3 IFG cleared
}

