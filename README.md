# base746 Project

usb fix: 'sudo chmod 666 /dev/ttyACM0
'; missing module: 'sudo pacman -S openocd libusb'


Description du capteur ou de l'actionneur

<img width="647" height="662" alt="image" src="https://github.com/user-attachments/assets/b58a8133-2985-4b93-bd07-40410483bfa6" /> 

The LD2450 is a motion target tracking sensor module from the
Hilink 24G millimetre wave radar series, which includes extremely simplified 24 GHz
radar sensor hardware and intelligent algorithm firmware. The solution is mainly used
in general indoor scenarios such as homes, offices and hotels to enable the location
tracking of moving human bodies. The sensor hardware consists of an AloT millimetre wave radar chip, a high
performance one-transmitter-two-receiver microstrip antenna and a low cost MCU and
peripheral auxiliary circuitry. The intelligent algorithm firmware uses FMCW
waveforms and the radar chip's proprietary advanced signal processing technology. 
It supports serial output of detection data, which is plug-and-play and can be
flexibly applied to different smart scenarios and end products.





Description de la liaison et principe de fonctionnement hardware
type d'alimentation, type de communication : i2c ou analogique ou ...

<img width="527" height="245" alt="image" src="https://github.com/user-attachments/assets/ff627c3b-77b7-4394-ab61-8274e6a6193a" />
<img width="302" height="45" alt="image" src="https://github.com/user-attachments/assets/452edda2-02cf-43f0-ace1-f561530b1136" />


LD2450 module directly through the serial port in accordance with the prescribed
protocol for the output of the detection results data, the serial output data contains up
to three targets position and speed and other information, the user can be used flexibly
according to the specific application scenarios. The module power supply voltage is 5V, and the power supply capacity of the
input power supply is required to be greater than 200mA. The module IO output level is 3.3 V. The default baud rate of the serial port is
256000, with 1 stop bit and no parity bit. 5.2 Description of the visualization


Schéma et typon de raccordement à la carte DISCO

The original board is designed to accept Arduino uno type modules.
<img width="960" height="720" alt="image" src="https://github.com/user-attachments/assets/a7c3e9cc-3f78-4f84-be1b-75d8d045157d" />

Therefore, we used the Arduino template. In the circuit I added a xt30 connector for battery support, and an additional port for GPIOs
in case we'd ever use them.

<img width="488" height="500" alt="image" src="https://github.com/user-attachments/assets/ff54028a-3b58-4ad9-a5ed-ded841f2f25b" />

The resulting pcb is single sided, rather compact and fits as expected.
<img width="578" height="633" alt="image" src="https://github.com/user-attachments/assets/207647aa-081e-4477-a43c-fc3969939cd1" />

Description de la liaison et principe de fonctionnement software
bibliothèques utilisées




Description du fonctionnement de l'application réalisée
1 page


![Version](https://img.shields.io/badge/version-1.0.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)






<details>
<summary>Click to see installation steps</summary>

1. Run `npm install`
2. Copy `.env.example` to `.env`

</details>


