#include <SPI.h>

volatile uint16_t limite_interrupcoes_contagem = 2500;
volatile uint16_t num_interrupcoes_contagem = 0;
volatile bool timer_acabou = 0;
int8_t potaux; // variável auxiliar que recebe o valor da potência via SPI do nodemcu
volatile int8_t aux;

ISR(TIMER0_OVF_vect) 
{
  /**
   * Esse bloco trata a interrupção de overflow do timer0.
   * Contamos limite_interrupcoes_contagem interrupções.
   * Passando disso, setamos timer_acabou = 1, indicando para
   * o restante do código que é hora de agir
   */
  num_interrupcoes_contagem++;
  if (num_interrupcoes_contagem >= limite_interrupcoes_contagem) {
    timer_acabou = 1;
    num_interrupcoes_contagem = 0;
  }
}

ISR(SPI_STC_vect)
{

/**
 * Esse bloco trata a interrupção do SPI.
 * Recebemos o sinal de leitura do nodemcu a cada 1 segundo
 */
 aux = SPDR;
 
 if(aux != 0)
  potaux = SPDR;
}

int main() {
  /**
   * ENTRADAS, SAÍDAS E PULL-UP
   */

  int8_t potencia_medida;
  int potencia_adc;
   
  DDRB = 0b00010010; // PB0(pino 8) é entradas para o botão de ligar
  DDRC = 0b0
    |0b000    // PC2(pino A2)para o ADC2 (potenciometro), PC0(A0) entrada do botão de desligar
    |0b111000 // PC3,4,5 (A3,A4,A5) saídas para o stack de leds
    ; 

  PORTB = 0b000000001; // pull-up no PB0 para o botão de ligar
  PORTC = 0b000000001; // pull-up no PC0 para o botão de desligar

  /**
   * ADC
   */
  ADMUX = 0b0 
    |0b01000000 // seleciona AVcc, que é 5V
    |0b10; // seleciona o ADC2, que entra pelo pino A2

  ADCSRA = 0b0 
    | _BV(ADEN) // habilita o ADC (enable)
    | 0b111; // seta o prescaler para 128, me dando 16mHz / 128 = 125 kHz

  
  /**
   * CONTADOR TIMER0
   */
   // todo: explicar essas configurações abaixo
  TCCR0A |= 0b00000000;
  TCCR0B |= 0b00000011; // seta a seleção de clock = clk/64
  TIMSK0 |= 0b00000001;


  /**
   * CONTADOR TIMER1 (usado no PWM)
   */
  TCCR1A = _BV(COM1A1) | _BV(WGM10) | _BV(WGM11);
  TCCR1B = _BV(CS11) | _BV(WGM12);

  OCR1A = 0; // inicializando a saída do PWM baixa

  /**
   * COMUNICAÇÃO SPI
   */

  //DDRB |= _BV(PB4); //Seta o pino PB4 como sendo de saída, o MOSI da comunicação SPI
  SPCR |= _BV(SPE); //Seta SPE como sendo 1, para habilitar o SPI
  SPCR |= _BV(SPIE); //Habilita a interrupção
  SPCR &= ~(_BV(MSTR)); //Seta como slave

  /**
   * MISC
   */
  sei(); // enable de interrupções
  Serial.begin(115200); // todo: tirar funcionalidades alto nível

  bool status_medidor = 0; // 0: medidor desligado, 1: medidor ligado

  int contador_eventos = 0; // usado pra dar uma quebra de linha depois de 10 prints

  int limite_inferior_OVF = 100; // o menor número de interrupções (overflows)do timer que eu conto

  int limite_superior_OVF = 1400; // o maior número de interrupções (overflows)do timer que eu conto

  /**
   * LAÇO PRINCIPAL
   */
  while (1) {
    /**
     * CONTROLANDO O FUNCIONAMENTO: LIGAR OU DESLIGAR
     */
    if (PINB & 0b00000001) { // botão de ligar apertado
      status_medidor = 1;
    }
    if (PINC & 0b00000001) { // botão de desligar apertado
      status_medidor = 0;
    }

    if (status_medidor) {
      /**
       * CONTROLANDO O ADC: O POTENCIÔMETRO DÁ O INTERVALO ENTRE 2 EVENTOS
       */
      ADCSRA |= _BV(ADSC); // começar conversão
      while (!(ADCSRA & 0b10000)); // esperar terminar conversão, checando o bit ADIF

      // aqui, transformamos um número que varia entre 0 e 1023 (do ADC)
      // em um número que varia entre limite_inferior_OVF e limite_superior_OVF.
      limite_interrupcoes_contagem = limite_inferior_OVF + (ADC / 1023.0) * (limite_superior_OVF - limite_inferior_OVF);
      potencia_adc = 600 - (6*abs(potencia_medida));

      Serial.println("negocio");
      Serial.println(600 - 8*abs(potencia_medida));
      
      // SENDO QUE OCR1A NA VDD VAI RECEBER O VALOR DO MEDIDOR DE WIFI
      OCR1A = potencia_adc;

      if (potencia_adc < 50 || potencia_medida == 31){
        OCR1A = 0;
        PORTC = 0b00000000;
      }
      else if (potencia_adc < 150)
          PORTC = 0b00100000;
      else if (potencia_adc < 400)
          PORTC = 0b00110000;
      else if (potencia_adc > 400)
          PORTC = 0b00111000;

      /**
       * DISPARANDO O EVENTO QUE ACONTECE A INTERVALOS REGULARES
       */
      // quando timer_acabou = 1, o timer terminou de contar o intervalo esperado
      if (timer_acabou) {
        timer_acabou = 0; // resetar a flag
        potencia_medida = potaux;
        contador_eventos++;
        if (contador_eventos > 10){
          Serial.println("");
          contador_eventos = 0;
        }
        
        //Serial.print("hi ");
      }

      
    } // fim if (status_medidor)
    else { // botão de desligar apertado
      OCR1A = 0;
      PORTC = 0b000000001; // pull-up no PC0 para o botão de desligar
    }
  } // fim while (1)
} // fim main
