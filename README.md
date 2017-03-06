# wwanEric
###### F5521GW ST-Ericsson (R) modem management utility

This utility provides a GUI interface for management of modems.

## Features
1. Status display: modem, network, memory, etc
2. USSD: send, receive, sessions, localization
3. SMS: receive, localization. Sending is not implemented yet, although it should not be a complex task.
4. Advanced COM-port configuration

## Building
Compilation should be successful with MS Visual Studio 2013 / 2015.
The application has rich logging functionality.

## Screenshot
![Screenshot](/screenshots/mainwindows.png?raw=true)

## License
The code for wwanEric itself is licensed under [**MIT**](/gsmmanager/LICENSE?raw=true).
However the project uses several 3rd party libraries:
- **Qt**, which licensing options are [different](https://www.qt.io/licensing/)
- **libPdu**, which is distributed under LGPLv2: [homepage](https://sourceforge.net/projects/libpdu/)

## P.S.
The project was some kind of a test area for my skills so code could be strange sometimes. Sorry in advance :-)
