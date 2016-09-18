# TempI Indicator

Author: Ian Caio

## Description

A CPU temperature indicator for Unity (tested on Ubuntu 16.04). Uses the output of `sensors` version 3.4.0 (libsensors version 3.4.0) to gather information. To download `sensors` on Ubuntu, just run `sudo apt-get install lm-sensors`.

## Configuration file

There are 3 parameters that can be configured in the TempI.config file:
- DELAY: Indicates the delay between each call to the 'sensors' application.
- LOG: Indicates wheter the user wants to log the temperatures in a file.
- LOGFILE: Indicates the logfile name, in case LOG is true.

The values for each of those parameters must be written *strictly* like described below:
- DELAY=\<*number*\>
- LOG=*TRUE/FALSE*
- LOGFILE=\<*name-of-logfile*\>

Any line that doesn't follow the above sintax will be ignored.
The \<*number*\> on DELAY must be in the range from 2-5 seconds or will be defaulted to 2.
If the string after "LOG=" is anything other than TRUE or FALSE, it will be defaulted
to FALSE.

## How-to use

- Set the configurations in the ./bin/Config/TempI.config file.
- Run `./bin/TempI &` from the project folder to start the program in the background.
- If you're running the debug version it's recommended to run it on foregroun in the
shell to be able to read the debugging output.
- The logfile will be written in the current folder from the shell calling the program.
- Click on the grey icon on the panel and then on "Quit" to leave the program.

## Requirements

- lm-sensors
