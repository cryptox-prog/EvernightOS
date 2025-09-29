Interrupt is a routine which essentially tells the processer to stop whatever it is doing to handle something else. This is done instead of polling, i.e., continuosly checking for the state of some device to determine if a certain action needs to be taken.
As may be obvious interrupts are more performance effiecient than polling.
In the real mode BIOS interrupts are active and can perform low level funcitons.
However, once we switch to protected mode, these interrupts are no longer active and a Interrupt Decriptor Table which stores programmer defined interrupts to handle them.
