# LazerCat

By: Andrew Kim & Rocco Scinto

## Description

LazerCat is an innovative toy designed for cat owners, enabling them to interact with their pets remotely through a controlled laser pointer and live video feed. Utilizing AWS WebRTC and IoT for seamless cloud integration, the system consists of three main components: a home-based CC3200 and Raspberry Pi station, an away CC3200 module, and central servers. The home station manages local motion detection and AWS-triggered laser control, while the away device allows users to send commands via an IR remote, displaying these commands on an OLED screen and forwarding them to AWS to update the shadow states. The Raspberry Pi handles video streaming, activating upon user request through our web app to provide live footage of the cat activities. Our implementation goals range from basic laser activation and motion sensor functionality to advanced features like pre-programmed laser routines, and real-time video streaming with minimal delay. Our project aims to keep cats entertained and active and also allow users to play with their cats from far away.
