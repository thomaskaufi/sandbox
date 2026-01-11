# Gople Kostume for Lilibeth Cuenca
- 24 LED WS2812 strips af ≈260LEDs pr strip
- Master ESP styrer animation jf. drejeknap (4 stadier) og trykknap (panic-mode)
- Driver ESP generer animation via FASTLED.h
- Animationer skiftes deterministisk og kan overskrives. Hver animation har INIT fase på ≈1500ms til fade. Panic-mode overskriver til hver en tid og fryser logik indtil afviklet.

*indhold:*
- /ressources: dokumentation af wiring og skitser til animation
- /kode:
  - **Arduino_harness** Bare minimum structure til at prototype animationer på arduino UNO (uploader hurtigere end ESP)
  - **DriverV3.ino** Driver ESP script
  - **Full_Receive** Driver skelet (kun parsing og serial, ingen animationer)
  - **master_wiring_check** Til at debugge master ESP hardware (viser knap værdier)
  - **masterV1.ino:** Master ESP script
  - **Simple_receive** til at debugge driver ESP hardware (viser indkommen data fra master)

*Syntax genereret med GPT5.2*

<img width="1239" height="822" alt="Wiring" src="https://github.com/user-attachments/assets/edf47a0e-64ca-4351-8b61-e17bbd65ae0b" />
<img width="1425" height="1209" alt="wiring_schem" src="https://github.com/user-attachments/assets/54de69d9-e9a9-4d41-8249-6025ed35c9e6" />
<img width="1490" height="1809" alt="Stadier" src="https://github.com/user-attachments/assets/46e966a3-735e-4761-9f24-b020029452f1" />

