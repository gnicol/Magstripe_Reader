Magstripe_Reader
================

Arduino to serial interface for three track TTL magstripe readers.

The track data is captured as raw 8 bit binary in received bit 
order including any parity, leading trailing 0's, etc. The results 
will be output to the serial port as raw HEX; if you have cards of an
unknown format this allows you to play around on the PC to decode.

I also check if, based on parity, the data appears to be 7 or 5
bit text. If so, we follow on the hex output with the decoded 
text on a track by track basis.

As my arduino doesn't have a sufficient number of interrupts to
track three clocks and a card present signal I've elected to 
bit-bang in the data which seems to work well enough. A more 
capable chip using interrupt driven inputs would quite possibly
be more reliable though.

Hardware Used:
* Ardweeny with FTDI adapter
* Magtek three track reader model 21050145
** http://www.magtek.com/documentation/public/99821101-6.03.pdf
