# Piano com CH32V002 

(english documentation at the end)

## Português

A ideia básica deste projeto é usar o PWM de um microcontrolador para tocar uma nota conforme a tecla apertada. A inspiração veio de um kit chinês que utiliza um 555.

### Hardware

Eu utilizei um CH32V002A4M6, um microprocessador RISC-V de 32 bits, operando a 48MHz, com 4 kbytes de RAM e 16 kbytes de Flash e encapsulamento SOP16. É uma escolha certamente exagerada para o projeto, mas foi baseada no meu interesse pelo CH32V00x e ter 50 destes chips na gaveta. O CH32V003A4M6, com menos memória, provavelmente seria mais apropriado.

Após algumas experiências, optei por pegar a saída do PWM (uma onda quadrada), passar por um filtro RC e amplificar usando um transistor S8050.

Os botões estão todos ligados a uma única entrada analógica. A tecla apertada é inferida a partir da tensão presente na entrada.

Um trimpot é usado para realizar a afinação e um LED foi acrescentado para o debug.

A alimentação pode ser feita pelo conector de programação ou (no caso da PCI) por um conector micro USB.

### Software

O software foi desenvolvido com o ch32vfun, uma infraestrutura de baixo nível (a programação é feita em C, acessando diretamente os registradores do microcontrolador).

No momento em que escrevo isso, a maioria dos exemplos no ch32Vfun é para o CH32V003 e requer ajustes para os demais modelos (como o CH32V002).

A programação do ADC para a leitura dos botões e do trimpot é relativamente simples. O código realiza várias leituras consecutivas para tentar pegar valores estáveis.

A programação do PWM é mais trabalhosa, mas é simples mudar a frequência uma vez feita a iniciação.

No uso normal, o trimpot de afinação é lido somente na iniciação e usado para ajustar todas as notas. Ligando o circuito com uma tela pressionada, entra-se no modo de afinação. Uma nota é tocada continuamente e sua frequência é atualizada periodicamente conforme a posição do trimpot.

### Placa de Circuito Impresso

Uma placa foi projetada com o Kicad e está atualmente em produção.

### Diretórios

Este repositório possui os seguintes diretórios:

* Hardware: esquema e projeto da PCI, feito com o Kicad 10
* Imagens: logotipo usado na PCI e imagem renderizada da PCI
* Outros: planilha excel para cálculo da programação do PWM
* Software: fontes e executáveis

## English

The basic idea behind this project is to use a microcontroller's PWM to play a note corresponding to the key being pressed. The inspiration came from a Chinese kit that uses a 555 timer.

### Hardware

I used a CH32V002A4M6, a 32-bit RISC-V microcontroller operating at 48 MHz with 4 KB of RAM and 16 KB of Flash in an SOP16 package. It is certainly an overkill for the project, but the choice was driven by my interest in the CH32V00x series and the fact that I had 50 of these chips in my drawer. The CH32V003A4M6, which has less memory, would likely have been more appropriate.

After some experimentation, I opted to take the PWM output (a square wave), pass it through an RC filter, and amplify it using an S8050 transistor.

The buttons are all connected to a single analog input. The specific button pressed is determined based on the voltage present at the input.

A trimpot is used for fine-tuning, and an LED has been added for debugging purposes.

Power can be supplied via the programming connector or (in the case of the PCB) via a micro-USB connector.

### Software

The software was developed using ch32vfun, a low-level framework (programming is done in C, directly accessing the microcontroller's registers).

At the time of writing, most ch32vfun examples are designed for the CH32V003 and require adjustments for other models (such as the CH32V002).

Programming the ADC to read the buttons and the trimpot is relatively straightforward. The code performs several consecutive readings to obtain stable values.

Programming the PWM is more involved, though changing the frequency is simple once initialization is complete.

During normal operation, the tuning trimpot is read only at startup and is used to adjust all notes. Powering on the circuit while a key is pressed enters tuning mode; a note plays continuously, and its frequency is periodically updated based on the trimpot's position.

### Printed Circuit Board

I used Kicad to design a PCB, it is currently in production. 

### Directories

This repository contains the following directories:

* Hardware: schematic and PCB design, created with KiCad 10
* Images: logo used on the PCB and a rendered image of the PCB
* Other: Excel spreadsheet for calculating PWM settings
* Software: source code and executables

