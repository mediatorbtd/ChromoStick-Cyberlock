<h1>ChromoStick-Cyberlock</h1>

 ### https://www.youtube.com/@MediatorSystems

<h2>Description</h2>
🚀 An end-to-end Cyber-Physical Systems (CPS) pipeline bridging bare-metal AVR hardware telemetry with an asynchronous Python gateway and an AI-driven SecOps automation engine (n8n + Gemini 2.5 Flash).

<h2>Website</h2>
https://www.linkedin.com/in/dominion-chibuzor-193153231/

<br/>

<h2>Topics</h2>
embedded-systems, cybersecurity, secops, python, automation, n8n, gemini-api, avr-gcc
<br/>


<h2>Languages and Utilities Used</h2>

- <b>VS code</b> 
- <b>Bare Metal C++</b>

<h2>Environments Used </h2>

- <b>Windows 11</b> 

## 📸 Complete System Walk-Through

Follow the lifecycle of an authentication event across the entire cyber-physical pipeline:

### 1. The Physical Hardware Setup
The edge node consists of an AVR microcontroller, an I2C LCD screen, and an analog joystick tracking directional vectors.

![Arduino Uno development board wired to an I2C LCD screen and an analog dual-axis joystick module on a prototype breadboard](https://github.com/user-attachments/assets/2e83a0c7-b76c-455b-898b-4de6ed16d1a0)


### 2. Embedded Control Layer (Bare-Metal C++)
The firmware handles hardware resets, samples analog noise for dynamic layout randomization, and prints interactive instructions to the user.

![VS Code text editor displaying bare-metal C++ firmware using AVR-GCC registers for ADC tracking and LCD initialization](https://github.com/user-attachments/assets/60e600cd-1045-4302-aa6b-66e53177f0a7)

### 3. The Middleware Gateway (Python Daemon)
When an access attempt occurs, the Python service captures raw serial bits over `COM4`, handles JSON packaging, and streams it to the cloud.

![Windows PowerShell terminal showing the execution of a custom Python gateway script outputting raw serial edge telemetry data](https://github.com/user-attachments/assets/2d031caa-dd67-47f2-9ff0-87d286d89758)

### 4. Cloud Orchestration Engine (n8n Workflow)
The webhook endpoint captures the incoming JSON payload and instantly routes it through the Gemini 2.5 Flash node for evaluation.

![An n8n cloud automation canvas layout mapping a live production pipeline with successful webhook execution indicators](https://github.com/user-attachments/assets/0ddf5a35-7466-4cb5-883f-f8da5786fa5f)

### 5. Automated SecOps Incident Alert (Gmail)
If consecutive failures accumulate or a hardware tamper flag triggers, Gemini crafts an executive threat summary and fires an urgent warning email.

![An automated security incident report alert email interface generated dynamically by Google Gemini 2.5 Flash](https://github.com/user-attachments/assets/4bb3d7ff-a6f4-4e00-a98f-cf815cf21f0d)

### 6. Central Database Audit Logging (Google Sheets)
Simultaneously, the payload metadata is permanently appended to an enterprise security log sheet for future forensic auditing.

![A secure Google Sheets analytics database table actively logging event IDs, telemetry timestamps, and failure metrics](https://github.com/user-attachments/assets/81aaf6f2-48c2-4940-b4c9-cf597f2839df)


+ text in green
! text in orange
# text in gray
@@ text in purple (and bold)@@
```
--!>
