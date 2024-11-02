#include "plugin.hpp"

using simd::float_4;

struct Mix8 : Module {
  enum ParamId {
    PARAMS_LEN=8
  };
  enum InputId {
    INPUTS_LEN=8
  };
  enum OutputId {
    CV_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };

  Mix8() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    for(int k=0;k<8;k++) {
      std::string label=std::to_string(k+1);
      configParam(k,0,1,0,"P"+label,"%",0,100);
      configInput(k,label);
    }
    configOutput(CV_OUTPUT,"CV");
  }

  void process(const ProcessArgs &args) override {
    int channels=0;
    for(int k=0;k<8;k++) {
      if(inputs[k].getChannels()>channels) channels=inputs[k].getChannels();
    }
    for(int chn=0;chn<channels;chn+=4) {
      float_4 sum=0.f;
      for(int k=0;k<8;k++) {
        sum+=params[k].getValue()*inputs[k].getVoltageSimd<float_4>(chn);
      }
      outputs[CV_OUTPUT].setVoltageSimd(sum,chn);
    }
    outputs[CV_OUTPUT].setChannels(channels);
  }
};


struct Mix8Widget : ModuleWidget {
  Mix8Widget(Mix8 *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/Mix8.svg")));
    float x=1.9f;
    float y=4;
    for(int k=0;k<8;k++) {
      addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,k));
      addInput(createInput<SmallPort>(mm2px(Vec(x,y+56)),module,k));
      y+=7;
    }
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,118.5)),module,Mix8::CV_OUTPUT));
  }
};


Model *modelMix8=createModel<Mix8,Mix8Widget>("Mix8");