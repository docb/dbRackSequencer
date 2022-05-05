# dbRackSequencer

A collection of sequencers

## Voltage addressable Sequencers

This type of sequencer does not have any clock inputs. 
The current steps are selected via voltage inputs, 
similar to using the step CV input from Bogaudio's ADDR sequencer.
However, the plugin provides some modules which do the job of addressing based on a clock.
But every other module which outputs step functions (e.g. S&H) can be used. Another advantage is that
f.e. for an 8-step sequencer there are 40320 possibilities to make a sequence (permutation) from 8 values
and not only one (1,2,3,4,5,6,7,8).

### TD4

This module provides 16 Tracks of a 4x4 Grid Sequencer. It is intended to be a building block 
for simulating hardware sequencers like Rene or Z8000. It is inspired by the Z8K and the Renato module
from the XOR plugin (which seems not to be available anymore in Rack v2).

The top two input rows are the 16 CV address inputs. The 16 steps are addressed in the range of 0-10V i.e. 10/16V per step.
Below there is the 4x4 Grid with 16 CV value knobs. The range of the knobs is configurable in the menu. 
The values can also be quantized to semi notes. The 16 lights per step show the position of each track
and the polyphonic gate output besides the knob reflects the status of the lights. 

Below that there are two rows of polyphonic gate inputs. With these, the steps for each track 
can be muted separately which is reflected by the next two rows of monophonic gate outputs, the lights and
the output besides the cv knob.

The next two rows are the monophonic CV outputs for each track.

On the bottom there are some additional polyphonic inputs and outputs.

- A polyphonic CV input which takes over the CV Knob values
- A polyphonic CV output which reflects the current voltages of each knob
- A polyphonic gate output which reflects the 16 monophonic gate outputs
- A polyphonic cv output which reflects the 16 monophonic CV outputs.

