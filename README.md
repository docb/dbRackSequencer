# dbRackSequencer

A collection of sequencers. Some modules of this plugin are inspired by the XOR plugin
which is not available in Rack v2.
New modules in 2.2.0

New modules in 2.1.0: [P16A](#p16a), [P16B](#p16b), [P16S](#p16s), [UnoA](#unoa), [CCA](#cca), [CCA2](#cca2),
[Ant](#ant), [TME](#tme), [SigMod](#sigmod), [MouseSeq](#mouseseq), [Preset](#preset), [CDiv](#cdiv), [CSR](#csr)

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  

- [Voltage addressable Sequencers](#voltage-addressable-sequencers)
  - [AG](#ag)
    - [ACC](#acc)
  - [Chords](#chords)
  - [TD4](#td4)
    - [P4](#p4)
      - [Some patching to illustrate](#some-patching-to-illustrate)
        - [Simulating a Z8000 Sequencer](#simulating-a-z8000-sequencer)
        - [A complex 16-step sequence](#a-complex-16-step-sequence)
    - [P16](#p16)
    - [P16A](#p16a)
    - [P16B](#p16b)
    - [P16S](#p16s)
    - [UnoA](#unoa)
    - [PXY](#pxy)
  - [C42](#c42)
    - [Gate/Trigger Sequencer](#gatetrigger-sequencer)
    - [Creating melodies](#creating-melodies)
    - [Generate Chaos](#generate-chaos)
    - [Examples](#examples)
      - [Classic Sequencing](#classic-sequencing)
      - [Chaos](#chaos)
  - [TheMatrix](#thematrix)
  - [CCA](#cca)
  - [CCA2](#cca2)
  - [Ant](#ant)
- [Some other sequencers](#some-other-sequencers)
  - [Uno](#uno)
  - [Klee](#klee)
  - [M851](#m851)
  - [CYC](#cyc)
  - [N3](#n3)
- [Some further sequencers](#some-further-sequencers)
  - [TME](#tme)
  - [SigMod](#sigmod)
  - [MouseSeq](#mouseseq)
  - [Preset](#preset)
- [Utilities](#utilities)
  - [Sum](#sum)
  - [CV](#cv)
  - [PwmClock](#pwmclock)
  - [CDiv](#cdiv)
  - [CSR](#csr)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->


## Voltage addressable Sequencers

This type of sequencer does not have any clock inputs. 
The current steps are selected via voltage inputs, 
similar to using the step CV input from Bogaudio's ADDR sequencer.
However, the plugin provides some modules which do the job of addressing based on a clock.
But every other module or combinations which outputs step functions (e.g. S&H) can be used. 

### AG
![](images/AG.png?raw=true)

- The AG module provides 100 voltage addressable polyphonic gates or gate patterns. 
- The number of channels can be set in the menu. 
- The address input recognizes 0.1V per step.
- If the Edit button is pressed the module behaves like the address input is not connected i.e. the steps 
can be manually selected, otherwise a connected address input will disable manual stepping.
- A click on the copy button will make a copy of the gate values.
- After selecting a new step it can be pasted there.
- The + button inserts an empty pattern i.e. moves the following patterns one to the right. The last pattern is removed.
- The - button removes the current pattern by moving all following patterns one to the left. 
- On randomize the density setting in the menu is taken to randomize the selected pattern (only). 

Some use cases of the module:
- Meta sequencing. Turn on and off other modules.
- Drum sequencing
- Arpeggiator
- ....


#### ACC
![](images/ACC.png?raw=true)

- ACC is a simple accumulator. 
- On every trigger event on the Clk input it updates its state by adding the value of the Step parameter
- On a trigger on the reset input or pressing the rst button the state is set to the value of the set param or set input if connected.
- The output always reflects the current the state.
- If the internal state crosses the value given by the Trsh parameter or input, a pulse will be sent through the trigger output.
  The trigger output can be self patched to the rst input to make loops.

If the step param is set to 0.1V ACC can be used to drive AG, Chords, Faders, P16, P16A.

### Chords
![](images/Chords.png?raw=true)

- The Chord module provides 100 voltage addressable polyphonic semi note values in the range from C0 to C8.
- The number of channels can be set in the menu.
- The address input recognizes 0.1V per step.
- If the Edit button is pressed the module behaves like the address input is not connected i.e. the steps
  can be manually selected, otherwise a connected address input will disable manual stepping.
- A click on the copy button will make a copy of the note values.
- After selecting a new step it can be pasted there.
- Pressing the N+ button transposes one semi up.
- Pressing the N- button transposes one semi down.
- Pressing the O+ button transposes one octave up.
- Pressing the O- button transposes one octave down.

- The chord module outputs behaves like the corresponding ones in the MIDI CV module.
- In the menu the number of polyphonic channels and the polyphony modes Rotate,Reset and Reuse for the channel assignment can be configured.
- The Reorder action in the menu causes the chord to be ordered in the polyphonic channels from the lowest note to the highest.
- The Auto-Reorder option causes the Reorder action on every change and the polyphony modes are ignored. 
- The Auto-Channel option causes the outputs only have as many channels as key buttons are pressed. In this case the Polyphonic Channels setting is ignored.
- Clicking The menu "Insert Chord" causes inserting an empty chord and moving the following chords one to the right. The last chord is removed.
- Clicking The menu "Delete Chord" causes removes the current chord by moving all following chords one to the left.
(Note: currently there are wrong labels - "Insert Pattern" and "Delete Pattern")

The ACC module can be used to drive a chord sequence.

### TD4

This module provides 16 Tracks of a 4x4 Grid Sequencer. It is intended to be a building block 
for simulating hardware sequencers like Rene or Z8000. It is inspired by the Z8K and the Renato module
from the XOR plugin.

The top two input rows are the 16 CV address inputs. The 16 steps are addressed in the range of 0-10V i.e. 10/16V per step.
Below there is the 4x4 Grid with 16 CV value knobs. The range of the knobs is configurable in the menu. 
The values can also be quantized to semi notes. The 16 lights per step show the position of each track
and the polyphonic gate output besides the knob reflects the status of the lights. 

Below that there are two rows of polyphonic gate inputs. With these, the steps for each track 
can be muted separately which is reflected by the next two rows of monophonic gate outputs, the lights and
the output besides the cv knob.

The next two rows are the monophonic CV outputs for each track.

On the bottom there are some additional polyphonic inputs and outputs.
- A polyphonic address CV input which takes over the 16 address inputs
- A polyphonic CV input which takes over the CV Knob values
- A polyphonic CV output which reflects the current voltages of each knob
- A polyphonic gate output which reflects the 16 monophonic gate outputs
- A polyphonic cv output which reflects the 16 monophonic CV outputs.

![](images/TD4Doc.png?raw=true)


Here are some modules which are suited for a clocked addressing of the TD4 but not limited to.

#### P4

![](images/P4.png?raw=true)
- A single P4 makes 4 Step Sequences inside an address space of 4-32 Steps (Size Knob). 
In the case of driving TD4 the size would be 16.
- The Ofs parameter controls the starting step inside the address space.
- The Perm parameter sets one of the 24 possible 4-Step permutations.
- If Y is set in the dir parameter the output is: `(step%4)*4+step/4` which causes moving up or down
  in TD4 and other grid sequencers (see below)
- It can be used to simulate the 4-Step sequencers of the Z8000, but it can do a lot more sequences.


##### Some patching to illustrate
NB: The videos have sound which could be turned on.

###### Simulating a Z8000 Sequencer

This example illustrates the use of four P4. 
The second (from right) connected P4 has an offset of 4, so it will go through the second row.
The third P4 is set to Y and offset 4, so it goes through the second column.
The forth P4 is set to Y and offset 12, so it goes through the forth column.

The use of AG is also shown to mute some steps via the Gate inputs.

https://user-images.githubusercontent.com/1134412/167461678-ca6a8c83-2a86-48d4-830f-3fba07001cec.mp4

###### A complex 16-step sequence

This example shows how to make a complex 16-step sequence with three P4 modules.
Only the right most P4 is connected to the TD4. The left most is clocked x4 and controls the 
permutation selection and the middle P4 (also clocked x4) controls the offset - by set to direction Y it always moves
the address by 4 steps.

https://user-images.githubusercontent.com/1134412/167403636-a7df029f-7f20-447d-914b-bb02f3699082.mp4


#### P16

![](images/P16.png?raw=true)

The P16 module provides 100 predefined 16-Step Sequences (ok, some kind of tiny collection compared to
the possible 20922789888000 permutations). But at least the direction (back or forth) and an offset can be selected per sequence.
However, e.g. an S&H or Towers+SwitchN1 or like in the example above can be used to make the rest.
NB: This size should be 16 for TD 4.
Here an example of the first 10 sequences.

https://user-images.githubusercontent.com/1134412/167461717-dcd016cd-858e-4bdb-9e3b-25aaaae53013.mp4

#### P16A
![](images/P16A.png?raw=true)

A variant of P16 with editable patterns and some extras:

- Active buttons below the lights cause that the step will not be touched on random generation.
- There is a polyphonic CV output for all values.
- The pattern can be reversed via the rev button.

#### P16B
![](images/P16B.png?raw=true)

This is a variant of the [TME](#tme) module for generating CV addressing patterns.
The output is built by `(A*1+B*2+C*4+D*8) * 10/size` where A (B,C,D) is zero or one reflecting if the
clock dividers are on or off at the position of A (B,C,D)

#### P16S

![](images/P16S.png?raw=true)

P16S sequences the values provided by the polyphonic In port according to the method given by the dir param.
It can also used as sequential switch (like 23volts SwitchN1)

#### UnoA
![](images/UnoA.png?raw=true)

This is a 16 step [Uno](#uno) with polyphonic inputs for probability and sequence reset (SR) suited for
addressing.

Example Usage:


https://user-images.githubusercontent.com/1134412/198848122-db6a0e34-0f93-4179-af81-bff62eb621ce.mp4



#### PXY

![](images/PXY.png?raw=true)

PXY is a 2D walker controlled by X+,Y+,X-,Y- Trigger inputs.
The start point can be controlled and the maximum length in each direction where wrapping around will occur.
This size should be 16 for TD 4.

Here an example:

https://user-images.githubusercontent.com/1134412/167461807-b4f6fbb2-5726-4762-abbb-8dfcab86d55c.mp4

#### PMod
![](images/PMod.png?raw=true)

Generates address sequences by multiplying the step number (increased by the clock input)
with the value of the Mult parameter and modulo the value given by the Mod parameter.


### C42
A universal sequencer. Vaguely inspired by the o88o module from the XOR plugin.

CAUTION: This module will trick you into spending a lot of time. 
Consider taking a walk in nature instead.

![](images/C42.png?raw=true)

- C42 provides a grid with sizes 8x8 upto 32x32.
- Each cell of the grid is either on or off.
- On the grid there can be placed up to 16 play heads.
- The play heads are placed via (polyphonic) CV addresses.
- The column of the play head is addressed via 10V/size * column_number via
  CvX param
- The row of the play head is addressed via 10V/size * row_number via
  CvY param
- The CvX/CvY input is added to the CvX/CvY param if the right On-Button is on.
- The On Buttons can be controlled via a gate signal on the right side input.
- The resulting position is always wrapped e.g. 10.1 V will place the head on the first column or row.
- A right click on the grid causes the CvX, CvY params set to the mouse position. The position is now visible in the grid.

Menus:
- Size: Set the size of the grid
- Clear: clears the grid
- Rule: Sets the cellular automation rule (see below)
- Paste RLE from clipboard:
  The RLE format looks like this:

   ```
   x = 18, y = 18, rule = B3/S23
   4b2o6b2o$3bo2bo4bo2bo$3bobo6bobo$b2o2bo6bo2b2o$o4b8o4bo$ob3ob6ob3obo$b
   o2b2o2b2o2b2o2bo$4b2o5b3o$4b3o2b2ob2o$4b3obobob2o$4b2o2b2ob3o$bo2b2obo
   2bob2o2bo$ob3ob6ob3obo$o4b8o4bo$b2o2bo6bo2b2o$3bobo6bobo$3bo2bo4bo2bo$
   4b2o6b2o!
   ```
   Something like that can be copied and pasted at the position of the CvX,CvY param. The x and y must be less than the current size.
   The most common application used in the context of 2D cellular automation is Golly. In this app a selection on the grid
   can be copied and then the RLE is in the clipboard. (the rule int RLE is ignored and must be set in the menu)

#### Gate/Trigger Sequencer
- C42 outputs a trigger on the polyphonic channel of the play head 
  - if the play head is changed to a cell which is on
  - if the cell where the play head is placed is turned on 
- The gate output has 10V on the polyphonic channel of the play head while the play head is on a cell which is on, 0V otherwise.
- The clock output represents the signal on the Gen (generate) input if the gate output is high.

#### Creating melodies
- There are nine polyphonic CV outputs which work as follows:
  - Top-Left: Every active cell on the Top Left diagonal of the play head is counted and multiplied with the value of the level param.
  - Bottom-Left, Bottom-Right, Top-Right similar.
  - Col-Top: Every active cell on the column above the play head is counted and multiplied with the value of the level param
  - Col-Bottom: Every active cell on the column below the play head is counted and multiplied with the value of the level param
  - Row-Left: Every active cell on the row and left side of the play head is counted and multiplied with the value of the level param
  - Col-Top: Every active cell on the row and right side the play head is counted and multiplied with the value of the level param
  - CV Gate (centered output): If the cell of the play head is on the output is the value of the level param, 0 otherwise

There is additionally an expander C42E which provides additional outputs:
  - Row (Row-Left + Row-Right)
  - Col (Col-Top + Col Bottom)
  - R+C (Row+Col)
  - R-C (Row-Col)
  - C-R (Col-Row)
  - MD (Top-Left + Row Bottom)
  - AD (Bottom-Left + Top-Right)
  - M+A (MD+AD)
  - M-A (MD-AD)
  - A-M (AD-MD)
  
A general alternative is to combine the outputs via the Sum module (see below) and its SE Expander
or any other module which processes multiple polyphonic inputs.

#### Generate Chaos

- If the Gen Button is clicked or a trigger on the Gen input is received a new 2D cellular automata generation is produced
- The following rules can be configured in the menu:
  - B3/S23 Classic Conways life 
  - B34/S34 (34 Life) 
  - B234/S (Serviettes)
  - B2/S (Seeds)
  - B1/S1 (Gnarl)
  - B36/S125 (2x2)
  - B345/S5 (Long life)
  - B2/S1
  - B2/S2
  
- NB: These rules all operate on a torus.
- The reset button or a trigger on the reset input rewinds the grid to the last edit operation.
- The Rnd button generates a random grid using the Dens (density) parameter. 

#### Examples
##### Classic Sequencing

Here is a demo showing  classic sequencing.
The melody and bass are made with one play head driven by a P16 by using the Col-Top and Col-Bottom output.
The middle line where the play head is moved and which is not counted in Col-Top or Col-Bottom 
is used for adding some gates triggering an ADSR/filter cutoff.
The level parameter is set to 1/7 (+a bit more) and the mode of Quant is "equi-likely" 
so that one more counted cell will move up by one position of the scale and an octave 
is exactly 7 counted cells.

On the second C42 one P16 and three P4 are used to make a 3-voice 32-step drum sequence.

(Due to the limited size of the video the sound quality is not so good).

https://user-images.githubusercontent.com/1134412/167498725-c5f50865-482a-4753-905a-3c2e5e11c66f.mp4

##### Chaos
In this example a period-16 oscillator in conways life is used. Four playheads are placed randomly via RndH.
The M+A (main diagonal + anti diagonal) output of the C24E expander is used to make the CV. The trigger output triggers
an ADSR/Filter cutoff.

https://user-images.githubusercontent.com/1134412/167783412-6dd213e9-b0b4-4d48-86a2-25c285fb9923.mp4


### TheMatrix
![](images/TheMatrix.png?raw=true)

Similar to C42 TheMatrix provides a grid of size 32x32 where up to 16 play heads can be placed
via the CvX/CvY params and inputs. But on each cell there can be edited an ascii char in the
range from 32 (space) to 126 (~).
The polyphonic CV output is built in the following way if a play head is on a cell which 
is no space:
First an integer number is looked up from the following table:

| char | value | char | value | char | value |
|------|-------|------|-------|------|-------|
| ! | -47 | A | -15 | a | 17 |
| " | -46 | B | -14 | b | 18 |
| # | -45 | C | -13 | c | 19 |
| $ | -44 | D | -12 | d | 20 |
| % | -43 | E | -11 | e | 21 |
| & | -42 | F | -10 | f | 22 |
| ' | -41 | G | -9 | g | 23 |
| ( | -40 | H | -8 | h | 24 |
| ) | -39 | I | -7 | i | 25 |
| * | -38 | J | -6 | j | 26 |
| + | -37 | K | -5 | k | 27 |
| , | -36 | L | -4 | l | 28 |
| - | -35 | M | -3 | m | 29 |
| . | -34 | N | -2 | n | 30 |
| / | -33 | O | -1 | o | 31 |
| 0 | -32 | P | 0 | p | 32 |
| 1 | -31 | Q | 1 | q | 33 |
| 2 | -30 | R | 2 | r | 34 |
| 3 | -29 | S | 3 | s | 35 |
| 4 | -28 | T | 4 | t | 36 |
| 5 | -27 | U | 5 | u | 37 |
| 6 | -26 | V | 6 | v | 38 |
| 7 | -25 | W | 7 | w | 39 |
| 8 | -24 | X | 8 | x | 40 |
| 9 | -23 | Y | 9 | y | 41 |
| : | -22 | Z | 10 | z | 42 |
| ; | -21 | [ | 11 | { | 43 |
| &lt; | -20 | \ | 12 | &#x7c; | 44 |
| = | -19 | ] | 13 | } | 45 |
| &gt; | -18 | ^ | 14 | ~ | 46 |
| ? | -17 | _ | 15 ||  |
| @ | -16 | ` | 16 ||  |

Then this number is multiplied by the level parameter value and passed to the output.

So if the level parameter is set to 1/12 then we have a semi note range from
C#-2 to B-3 and the letter 'A' denotes A-2. 

A space will cause that the gate output is set to 0V, every other character causes the gate
output set to 10V.

This seems to be a bit complicated, but there are some editing features which make life easy:

- connect a midi keyboard and the  V/Oct of the MIDI CV Module to the Rec CV input 
  and the Retrigger output to the Rec Trg input. On pressing a note on the keyboard
  the corresponding ascii char will be inserted so that the cell will output the same note
  if the level is set to 1/12.
- Similar with the Chords module: set polyponic channels to one and connect the V/O output to the CV in and the Rtr output to Trg.
- If the cursor is on a cell then the Keys PageUp and PageDown will increase/decrease the current character
  according to the ascii table.

The editor supports also selecting a rectangular region with cut/copy/paste operations.
Text can also be pasted from the system clipboard.

The editor is also connected to the undo/redo system of VCVRack.

The Rnd Button or a trigger on the Rnd input will fill the grid or the selection with random characters in the
range set by the Range parameters min and max. The dens parameter or input controls how many spaces
are generated.

Here an impression what can be done with multiple playheads:



https://user-images.githubusercontent.com/1134412/198893701-24a33a29-0b95-4325-b53b-31a91c0a0682.mp4



### CCA
![](images/CCA.png?raw=true)

A one dimension [continuous cellular automaton](https://www.wolframscience.com/nks/p155--continuous-cellular-automata/) voltage addressable sequencer.

- The inputs 0-15, 16-31 are for setting the initial values displayed on the left bar.
- On the initial values the automaton rule defined by Func and Param is successively applied from left to right.
- For the other parameters and inputs and outputs see C42/TheMatrix.
- Gates are fired if the value of the cell is above the threshold set by the knob.

Here an example with one playhead driven by Walk2 and switching the function parameter with AddrSeq. The initial values are
randomly chosen.


https://user-images.githubusercontent.com/1134412/199918183-bbc5518b-632d-49e4-a298-79926b29c9ff.mp4



### CCA2
![](images/CCA2.png?raw=true)

A two-dimensional variant of continuous cellular automaton. Similar to C42. The rule is defined by a function and a parameter.

Here an example: 4 playheads are placed static on the grid and the clock drives the generation.



https://user-images.githubusercontent.com/1134412/198815185-18c993f1-2439-45ca-8715-29882053e671.mp4




### Ant
![](images/Ant.png?raw=true)

A 2D turing machine also known as Langton's Ant.

- Rules can have up to 16 colors. The Rule input interprets the polyphonic signal as follows:
  - 0V = B (backward)
  - 1V = R (right)
  - 2V = F (forward)
  - 3V = L (left)

- There can be placed up to eight Ants via the Ants input:
  - The number of ants is `# channels div 2`
  - The x coordinate is given in even channels
  - The y coordinate is given in odd channels
  - The orientation is given by the signs of the x,y pair

- There are additionally some standard rules and number of ants configurable in the menu to start with (applied if the
Ants/Rule inputs are disconnected).
- The Xout and Yout ports deliver the coordinates of the ants. These can directly used for
  TheMatrix, CCA, CCA2 or C42 with grid size 32.
- The number of steps per clock pulse can be set using the Steps parameter
- The set knob or input triggers storing the current grid and ants, which will be set if a reset is triggered.

Here is a simple example with 3 Ants configured in the menu with the rule LLRRRLRLRLLR, where it is not clear
in which time this sequence would repeat.

https://user-images.githubusercontent.com/1134412/198703138-915d2495-e716-4cb7-9cbd-be547074b18c.mp4


Here an example configured with the ants and rule input (BBBRLBFB).


https://user-images.githubusercontent.com/1134412/198720606-a42da3ca-bc6c-4c92-b1da-49ea7be213b6.mp4



## Some other sequencers

The following sequencers are reimplementations with additional/different features
of some sequencers of the XOR plugin.
### Uno
![](images/Uno.png?raw=true)

Uno is a reimplementation of the qu4ttro sequencer of the XOR plugin.
With the default values this is a normal 8-step sequencer. However, this sequencer answers
the question of what happens if a probability can be set for skipping a step.

Differences/Additions to the original:
- It does not provide 4 Tracks but a chainable expander to have an arbitrary count of tracks.
- Polyphonic inputs for the probability and CV Values are provided
- The CV Range can be adjusted in the menu
- Quantize to semis can be turned on in the menu.
- The probability for skipping a step is in the range of 0-100% and independent of the Glide and Rst.
- If the Rst is on and the step is not skipped then it resets the sequence to the first step in forward and pendulum mode
  and to the last step in backward mode.
- The gate output represents the clock input if the gate is on and the Glide on the step is turned off.
- If glide is On and the step is not skipped then the gate stays high
- A polyphonic step gate output is provided
- The seed input can be used to make reproducible sequences.
If connected and not zero on a reset it will result in the same sequence for the same value provided.

Here an example showing the effect of the Rst button:



https://user-images.githubusercontent.com/1134412/168129909-76ae9c21-6e5a-4e34-a107-bf9f9eb173e2.mp4



### Klee
![](images/Klee.png?raw=true)

You know, it may be better to take a walk in nature ...

This module is a reimplementation of the Klee module of the XOR plugin with a bunch of additional features.
It simulates a Klee like sequencer - please read carefully the PDF first you will find if you type
'klee sequencer pdf' in your search engine to understand what this is all about.
There are also tutorials on youtube for the hardware klee (search for 'klee sequencer')  
and you also will find a three years old great tutorial from Omri Cohen about the XOR Klee.

The additional features:
- Every CV Knob has a single input which takes over the value.
- The CV Knob range can be changed in the menu.
- The CV Values can be quantized to semi steps in the menu.
- Polyphonic inputs
  - A CV input which controls all CV Knobs
  - A Bus input which controls the Gate bus
  - A Pattern input which controls the start pattern used when load is triggered
- Polyphonic outputs:
  - A static CV output which holds all knob values
  - A trigger output where a trigger is fired on a channel if the corresponding step is turned on.
  - A Gate output which is high if the corresponding step is active.
  - A Clock output which represents the input clock if the gate is high.
- Feedback Tabs (to make this module complex in a further dimension):
  - Beside the lights there are small buttons for turning on a feedback tab for the step.
  - This turns the shift register into a linear feedback shift register LFSR (see e.g. wikipedia).
  - A polyphonic feedback tab input can be used to set the tabs. 
  - The feedback is only active if the On button on right side of the input is on.
- By default, the CV outputs will only be updated if a clock signal arrives. This can be changed
  in the menu by check the "Instant" item. Then the CV is always calculated, but it will cost
  about 0.7% more CPU.


NB: this module needs a bunch of tutorials or may be better taking a walk ....

Here is an example how to use the Klee without a quantizer connected to the CV outputs but
still staying in a scale.
With a script (can be run with `node klee.js` in the project root if node.js is installed)
there are precomputed scales which would occur if only three values of 16 are
not zero or its octaves. It shows that e.g the semi notes 2-5-9 and its octaves make a major scale 0,2,4,5,7,9,11 whch will not be left
regardless of the bit pattern.
It requires more bits in the pattern to get a sequence which covers most of the scale.
The video also shows the use of the polyphonic CV input. 



https://user-images.githubusercontent.com/1134412/168665134-d6fbbb24-1278-4fd4-afe9-92c217cbfe2d.mp4






### M851
![](images/M851.png?raw=true)

This module is a reimplementation of the M581 of the XOR plugin.

- With the default values this is a normal 8-step sequencer. 
- There are additional CV inputs which take over the step CV value.
- The Knob range can be changed in the menu.
- The CV Values can be quantized to semi steps in the menu.
- The Gate output always represents the clock input  (there is no gate length parameter
as in the originals).
- There are five play modes: forward, backward, pendulum,random walk and random.

Things change dramatically if the repetition values are set to a higher value than one.
The step then will stay for the number of repetitions. This will make the sequence longer
(however the sequence length can be held constant via the rst input).
Now the gate mode parameter comes into play:
- '-----' means no gate at all (works also for one repetition)
- '*----' means only a single gate at the beginning (the default)
- '*****' for every repetition there will be a gate
- '\*-\*-*' for repetition 1,3,5,7 there will be a gate
- '\*--\*-' for repetition 1,4,7 there will be a gate
- '\*---\*' for repetition 1,5 there will be a gate
- '?????' gates are fired randomly
- '▪▪▪▪▪' the gate will stay high for gliding to the next step

If the glide button is on, there will be a portamento with speed adjustable by the glider knob.
If the step is turned off it will be skipped.

### CYC
![](images/CYC.png?raw=true)

CYC is a reimplementation of the Spiral sequencer of the XOR plugin.

- CYC is a 6 track cyclic step sequencer on a common 32 knob/input value space.
- The Knob range can be changed in the menu.
- The CV Values can be quantized to semi steps in the menu.
- There are two polyphonic CV inputs for step 0-15 and 16-31
- For each track the play mode (forward, backward, pendulum,random walk and random), the offset (0-31), length (1-32) and stride (1-8) can be configured.
- For each track the steps can be muted/unmuted by clicking on the step light.
- If one of the random modes is used with the polyphonic seed input a random seed 
(by setting channel 1-6) can be set to obtain the same sequence (if the seed value is not zero)
on every reset.

### N3
![](images/n3.png?raw=true)

N3 is a reimplementation of the Nag-Nag-Nag sequencer from the XOR plugin with the following
differences:
- It has 8 Tracks
- Each track has additionally a clock output which represents the incoming clock and a gate output
  which is high from the start of the trigger until the next clock.
- There are polyphonic outputs for gate, trigger and clock
- The Rotate, Skew and Degree parameters are floating point numbers.

Here an example how to use N3 as 16-step trigger sequencer by setting the degrees to 22.5 .


https://user-images.githubusercontent.com/1134412/169410926-7ad989b9-d5fe-4d3f-9039-793251da9c1b.mp4



Here an example of increasing the degree parameter via the degrees input while modulating all Rotate and
Skew parameters with a RndC.



https://user-images.githubusercontent.com/1134412/169410966-ff140fd2-2877-47cc-abd2-93af830ad472.mp4



Note that in the Exact mode (exact hits) the Skew,Rotate and Degree parameters should somehow
fit together to get overall some hits, e.g. could be integers.

## Some further sequencers
### Carambol
![](images/Carambol.png?raw=true)

A billiard simulation for sequencing and modulation.

Params:
- **R** sets the radius for all balls if the R input is not connected
- **COR** the coefficient of restitution, should be normally 1 or below. 
  Values greater than one will exponentially speed up over time.
- **Count** number of balls (1-16). Changing the count will reset the scene.

Outputs:
- **X,Y** Polyphonic outputs for the positions of all balls.
- **Wall** Polyphonic output of triggers whenever a wall is hit.
- **Hit** Polyphonic output of triggers whenever two balls collide (one trigger for each).

Inputs:
- **R** Polyphonic input for set the radius for each ball separately.
- **Rst** resets the scene, all balls will get new random positions and velocities
- **seed** if connected and > 0 the value is taken as seed for the random generation.
  This causes identical start positions and velocities on a reset.
- **SX,SY,VX,VY** if connected the random start positions or velocities are overwritten by the given values.
- **Clk** If connected the trigger outputs are delayed until a clock trigger is received

### TME
![](images/TME.png?raw=true)

A Triadex Muse Emulator (see e.g. [here](https://till.com/articles/muse/)).

- Has a Scl input for setting a custom scale (eight valued polyphonic signal with semi tones in v/oct).
- The resulting CV comes through Note output.
- The Trg output delivers a pulse if the CV has changed.
- The CV output delivers the number `D*8+C*4+B*2+A` multiplied with Lvl.
- The Clock mode can be set in the menu
  - If PWMClock is checked then the falling edge of the clock will be used for the C1/2
  - Otherwise (the default) only the rising edge is used - so it should be clocked with double speed.

Here an example of changing the scale.


https://user-images.githubusercontent.com/1134412/198822402-09162005-72b9-4659-880a-378d2ab2c5ef.mp4


In the following example the CV output is used for addressing the TD4 sequencer.
The Level must be set to 10/16V to have 16 steps in 0-10V.


https://user-images.githubusercontent.com/1134412/198822450-cddd8ef5-4e0a-44fe-9389-a5469b538bcf.mp4



### SigMod
![](images/sigmod.png?raw=true)

SigMod has a 16 value shift register (SR)
with 5 tabs which can be placed inside the SR via the Pos parameters (Pos1,Pos2,Pos3,PosR,PosO).
On a clock trigger the first three tabs are used for making a decision if either the incoming signal
(through the IN input) is read and  put into the SR 
or the value inside the SR at the read position (PosR) is taken.
The decision is build as follows:

`!(v1<Cmp1 XOR v2<Cmp2) XOR v3<Cmp3`  

where v1,v2,v3 are the values of the SR at the tab positions Pos1,Pos2,Pos3.
The 'less than' decision can be inverted in the menu for each comparator separately.

Here an example. It shows how the original sequence is scrumbled but still there somehow.


https://user-images.githubusercontent.com/1134412/198817855-d4cc421a-d1b8-493b-b96d-0abde74013b1.mp4



### MouseSeq
![](images/MouseSeq.png?raw=true)

A sequencer, solo player driven by the mouse.

How to use:

![](images/MouseSeqUsage.png?raw=true)

Then press and move the mouse on the green pad.
The current clock input can be switched with the keyboard `z,x,c,v`.
The sales can be switched via `a,s,d,f`.
Here an example:


https://user-images.githubusercontent.com/1134412/198819843-25800021-1423-41da-b80b-4b2a301cdc4f.mp4




### Preset
![](images/Preset.png?raw=true)

- Shows the presets of a module (user and factory)
- Presets are selectable via mouse or CV (0.1V steps)

## Utilities

### Sum
![](images/Sum.png?raw=true)

This module does the same as Sum Mk 3 from the ML Modules plugin, 
but it has 12 polyphonic inputs and a chainable expander which reduces the amount of cables if
different sums are built from the same inputs. 

### CV
![](images/CV.png?raw=true)

CV is another discrete CV value source. The number on the selected button is multiplied by the level
value and passed to the output. The original idea for this module was born after watching
the video about the octave sequencer from Jakub Ciupinski. With the CV module it looks like this:

![](images/CVSeq.png?raw=true)

It basically behaves like the OCT module of the Fundamental plugin.
It can be used without a quantizer as the level can be set to 1/12.
 
### PwmClock
![](images/PwmClock.png?raw=true)

Yet another clock generator module. The purpose of this module is to have many clock generators with
different ratios and adjustable pwm. You may need it when working with the sequencers described above.

The BPM input X (range -1 to 2) expects the frequency in HZ via 2^X 
i.e 
- -1V = 0.5 HZ = 30 BPM
- 0V = 1 HZ = 60 BPM
- 1V = 2 HZ = 120 BPM
- 2V = 4 HZ = 240 BPM

The BPM output delivers vice versa.

### CDiv

![](images/CDiv.png?raw=true)

Clock dividers. Why another?

I needed some with a reliable reset behaviour and divisions up to 100. 
The dividers are always in sync even while changing the division.

### CSR
![](images/CSR.png?raw=true)

A 16 value shift register (used also in SigMod).
