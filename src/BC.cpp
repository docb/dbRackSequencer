#include "plugin.hpp"

#define SIZE 8

struct BC : Module {
  enum ParamId {
    PARAMS_LEN
  };
  enum InputId {
    INCR_INPUT,DECR_INPUT,RST_INPUT,SEED_INPUT,INPUTS_LEN
  };
  enum OutputId {
    POLY_OUTPUT=SIZE,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN=SIZE
  };

  uint8_t pos=0;
  dsp::SchmittTrigger incrTrigger;
  dsp::SchmittTrigger decrTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::PulseGenerator rstPulse;
  uint8_t stepValue=1;

  BC() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configInput(INCR_INPUT,"Increment");
    configInput(DECR_INPUT,"Decrement");
    configInput(RST_INPUT,"Reset");
    configInput(SEED_INPUT,"Step Amount");
    for(int k=0;k<SIZE;k++) {
      std::string label="Bit "+std::to_string(k+1);
      configOutput(k,label);
    }
    configOutput(POLY_OUTPUT,"All Bits");
  }

  void process(const ProcessArgs &args) override {
    if(inputs[SEED_INPUT].isConnected()) {
      int s=0;
      for(int k=0;k<SIZE;k++) {
        if(inputs[SEED_INPUT].getVoltage(k)>1.f) {
          s|=(1<<k);
        }
      }
      stepValue=s;
    }
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())) {
      pos=0;
      rstPulse.trigger(0.001f);
    }
    bool resetGate=rstPulse.process(args.sampleTime);
    if(incrTrigger.process(inputs[INCR_INPUT].getVoltage())&&!resetGate) {
      pos+=stepValue;
    }
    if(decrTrigger.process(inputs[DECR_INPUT].getVoltage())) {
      pos-=stepValue;
    }

    for(int k=0;k<SIZE;k++) {
      bool b=(pos&(1<<k))!=0;
      outputs[k].setVoltage(b?10.f:0.f);
      outputs[POLY_OUTPUT].setVoltage(b?10.f:0.f,k);
      lights[k].setBrightness(b?1.f:0.f);
    }
    outputs[POLY_OUTPUT].setChannels(SIZE);
  }
};


struct BCWidget : ModuleWidget {
  explicit BCWidget(BC *module) {
    setModule(module);
    float x=1.9;
    float y=8;
    setPanel(createPanel(asset::plugin(pluginInstance,"res/BC.svg")));
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,BC::INCR_INPUT));
    y+=11;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,BC::DECR_INPUT));
    y+=11;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,BC::RST_INPUT));
    y+=11;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,BC::SEED_INPUT));
    y=55;
    for(int k=0;k<SIZE;k++) {
      addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,k));
      addChild(createLight<TinySimpleLight<GreenLight>>(mm2px(Vec(x+6.5f,y+4.5f)),module,k));
      y+=7;
    }
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,116)),module,BC::POLY_OUTPUT));
  }
};


Model *modelBC=createModel<BC,BCWidget>("BC");