# dbRackSequencer

A collection of sequencers. Some modules of this plugin are inspired by the XOR plugin
which is not available in Rack v2.

## Voltage addressable Sequencers

This type of sequencer does not have any clock inputs. 
The current steps are selected via voltage inputs, 
similar to using the step CV input from Bogaudio's ADDR sequencer.
However, the plugin provides some modules which do the job of addressing based on a clock.
But every other module or combinations which outputs step functions (e.g. S&H) can be used. 

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



https://user-images.githubusercontent.com/1134412/167461678-ca6a8c83-2a86-48d4-830f-3fba07001cec.mp4



https://user-images.githubusercontent.com/1134412/167461717-dcd016cd-858e-4bdb-9e3b-25aaaae53013.mp4



https://user-images.githubusercontent.com/1134412/167403636-a7df029f-7f20-447d-914b-bb02f3699082.mp4


#### P16


https://user-images.githubusercontent.com/1134412/167461807-b4f6fbb2-5726-4762-abbb-8dfcab86d55c.mp4




### C42
An universal sequencer.

![](images/C42.png?raw=true)

- C42 provides a grid with sizes 8x8 upto 32x32 (config in the menu).
- Each cell of the grid is either on or off.
- On the grid there can be placed up to 16 play heads.
- The play heads are placed via (polyphonic) CV addresses.
- The column of the play head is addressed via 10V/size * colum_number via
  CvX param
- The row of the play head is addressed via 10V/size * row_number via
  CvY param
- The CvX/CvY input is added to the CvX/CvY param if the right On-Button is on.
- The On Buttons can be controlled via a gate signal on the right side input.
- The resulting position is always wrapped e.g. 10.1 V will place the head on the first column or row.


#### Gate/Trigger Sequencer
- C42 outputs a trigger on the polyphonic channel of the play head 
  - if the play head is changed to a cell which is on
  - if the cell where the play head is placed is turned on 
- The gate output has 10V on the polyphonic channel of the play head while the play head is on a cell which is on, 0V otherwise.
- The clock output represents the signal on the Gen (generate) input if the date output is high.

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
  
A general alternative is to combine the outputs via the Sum module and its SE Expander
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
  
- NB: These rules all operate on a torus.
- The reset button or a trigger on the reset input rewinds the grid to the last edit operation.
