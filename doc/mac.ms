.TL
MAC Implementation
.AU
Vita Nuova
.br
28 February 2005
.br
Revised, 15 April 2005
.br
Further revised, 3 October 2005
.SP 2
.NH 1
Introduction
.LP
This document describes the Medium Access Control (MAC) protocol for
wireless sensor networks that is implemented in Vita Nuova's operating system for its
revision of the Berkeley motes.
Because the motes are battery operated it is very important to reduce
energy consumption. Because the radio is the component on the motes that
uses the most energy even when listening, it is vital that it be switched
off most of the time. Various MAC protocols have been devised to allow
radio sleep periods and a reduction in power output. These protocols
have to reduce sources of enery waste such as packet collisions and
idle listening. The former occurs when two packets are transmitted at
the same time, collide and become corrupted resulting in the need
for transmission retries. The latter happens when a mote is listening
to receive data for too long and to no purpose.
.PP
MAC protocols may roughly be divided into two types:
.I "scheduled protocols" ,
in which time slots for transmission and reception of data are allocated
to individual motes; and
.I "contention protocols" ,
in which all motes share
the same time slots and contend for access to it. The former do not
scale well if there are many devices
in range, whilst the latter tend to be less energy efficient, because each
collision causes a retransmission, which takes energy.
For this project it was initially decided that a contention-based protocol would
offer the simplest implementation, and that would outweigh any energy
inefficiency.
.PP
Typically the contention-based protocols use control packets to reduce
collisions. The sender sends a small request-to-send (RTS) packet
to its intended receiver. The receiver then responds with a small
clear-to-send (CTS) packet. Once the sender sees the CTS packet
it sends its data (DATA) packet, expecting an acknowledgement (ACK) packet back
in order to confirm its arrival. The purpose of this handshake is to make
an announcement to other neighbours of the the sender and receiver
to back off until the transaction is complete. RTS packets can still
collide but the chance of this happening is greatly reduced as RTS
packets are usually much smaller than data packets.
.PP
Subsequently we found that the situation is not so clear cut:
the energy-efficient contention protocols usually have some form of schedule
(except when there is special low-power hardware to detect a radio wakeup signal during sleep),
so the distinction is really between protocols using Time Division Multiplexing (TDMA), and
contention protocols with or without scheduling.
The original TinyOS MAC protocol uses contention without scheduling, but is not energy efficient;
most of the others, including the ones we tried and extended, use some form of schedule,
but without the strict framing used by TDMA.
.NH 1
S-MAC
.LP
S-MAC is a MAC protocol, devised at the University of Southern California,
that uses a sleep-wakeup cycle to allow motes
to sleep 90-99% of the time. Each mote chooses its own sleep schedule
and then communicates it every so often to neighbours in a broadcast synchronization
(SYNC) packet. Neighbours are encouraged to share schedules in order to
reduce the number of times they have to wakeup to listen for a neighbour's
transmissions. A duration field in sent in each packet so that neigbours
know how long to back off for when they hear control packets.
A node sending a message to another node must transmit in that node's schedule.
.PP
The S-MAC protocol above has the disadvantage that the listening time during
wakeup is typically a fixed period no matter what the current packet load is.
More seriously, schedules can interact, for instance when a node has
two neighbours that cannot see each other and thus are on different schedules,
making the scheduling of sleeps and wakeups non-trivial, since active times can overlap.
The difficulty is implicit in the specification, but is less obvious in the prototype
implementation for TinyOS, perhaps because of its strictly event-driven structure:
it is hard to work out the global invariants assumed by a particular chunk of event code,
and whether those invariants are in fact maintained by all possible interleavings of events.
.NH 1
T-MAC
.LP
An alternative protocol, T-MAC, devised at the Delft University of Technology,
attempts to address the problems with S-MAC.
As with other protocols, T-MAC uses the RTS, CTS, DATA, ACK scheme and
uses SYNC packets for synchronization.
It makes two changes compared to S-MAC.
First, the listening time is made variable, varying the active time dependent upon load.
It exchanges messages in bursts, each node
ending its active period when it has heard nothing for a given period, which is dynamically
adjusted to account for local topology.
Second, the use of the schedules is reversed:
a node transmits only at the start of its own schedule, and only listens on any other schedule
(and then only for that schedule's active time).
.NH 1
Our implementation
.LP
The actual implementation of the MAC protocol in the Vita Nuova system is a hybrid:
it starts with the ideas of T-MAC but incorporates
some of the ideas in S-MAC, as well as some further home-grown optimisations,
developed with an actual radio network.
Its implementation is much smaller and simpler than the various attempts at S-MAC, and was
certainly easier to analyse for tuning.
.PP
Initially a mote listens for a certain period (roughly 15 seconds) for SYNC packets.
If it receives one, it adopts the same schedule, if not, it creates its own schedule
which is to sleep for 1024ms (starting at the current time)
before waking up for transmission/reception.
Once in the radio sleep state, radio wakeup can only occur if the mote's own
scheduled timeout expires or it has found it has neighbours with different schedules,
and any one of those expires.
To reduce the number of schedules a mote has to deal with,
motes will adopt another neighbour's schedule if it realises that no other motes
are sharing its own schedule; if neighbouring schedules are within a given time delta
of its own, it will gradually adjust its own schedule to match, if possible,
causing the nodes sharing that schedule to move as well, leading the
whole network to converge on a single schedule.
Motes actually wake slightly before the start time, to allow time for the
receiver to stabilise and to account for clock drift.
.PP
Once awake, the mote will send out a SYNC packet roughly every 16 frames, a
frame being a sleep/wakeup cycle. If it has something to transmit (a SYNC or
DATA packet) and it is on its own schedule's wakeup, it senses the radio
for other traffic for a random time (2-32 ms) and if it is clear it either transmits
the SYNC packet or a RTS packet in order to send data. In the latter case it will
wait for a CTS before sending the data. If that wait on the CTS times out it will retry
before giving up and deferring the data for later (with the possibility of the
data not being transmitted if the output data queue gets too big). 
.PP
The mote then enters a listening period. Here it may receive a SYNC packet in
which case it updates its scheduler tables and perhaps adjusts its own
schedule in response. It may receive a RTS packet in which case it sends
a CTS packet and then stays in the listening stage for the data. If it
receives data it either passes it on towards the base station or over the serial
device to the main computer if the mote is the base station.
.PP
Routing information is passed along with SYNC packets.
The current scheme uses a vector routing algorithm, similar to one used on the Internet.
Each mote keeps a
record of the best destination back to the sink (base station) and the number
of hops this requires. It also sends information to its neighbour's of the
number of hops to get to the sink via itself (ie, 1 extra hop) in the SYNC packet. 
Motes receiving a SYNC packet can then update their entry if they see
the new entry is better than theirs in terms of hops. The sink sends out
its address with zero hops to set off the whole process whilst other motes
have an large number of hops (a logical `infinity') initially set in their tables,
causing them to adopt the first available route to the sink,
which is adjusted as different routes arrived.
The basic algorithm will be extended to weight the hops based on current energy use by
intermediate nodes, so that the load is spread evenly when more than one path is available.
