#include <avr/io.h>

int main(){

    DDRB =  0b00000010; // PB0 entrada para o botão e PB1 saída para o LED
    PORTB = 0b00000001; // pull-up no PB0 para o botão

    while(1){
        if (PINB & 0b00000001 == 1){
            // botão apertado
            PORTB = PORTB | 0b10; // colocar um 1
        } else {
            // botão livre
            PORTB = PORTB & 0b11111101;  // zerar um 1
        }
    }
}