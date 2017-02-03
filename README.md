BondWatch
=========

Watches the status of a bond network interface, and resets its mode to static whenever it has changed.

This was written as a band-aid for a problem I've been experiencing with my static bond on macOS Sierra, in which the bond reverts to LACP seemingly spontaneously (appearing more frequently around network changes, and sleep cycles).

It relies upon inspecting the bond to report that it is up, but inactive (since LACP isn't able to succeed in my setup).


```
make
sudo ./bondwatch -i bond0
[bondwatch] 2017-02-03T17:10:55 DEBUG: Opening socket...

< time passes here >

[bondwatch] 2017-02-03T18:11:06 INFO: Bond appears inactive; setting bondmode to static.
~~~

