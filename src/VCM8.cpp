#include "plugin.hpp"

using simd::float_4;

struct VCM8 : Module {
  enum ParamId {
    MOD_PARAM=8,MIX_PARAM=MOD_PARAM+8,PARAMS_LEN
  };
  enum InputId {
    MOD_INPUT=8,INPUTS_LEN=MOD_INPUT+8
  };
  enum OutputId {
    CV_OUTPUT,MIX_OUTPUT=CV_OUTPUT+8,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };

  VCM8() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    for(int k=0;k<8;k++) {
      std::string label=std::to_string(k+1);
      configParam(k,-1,1,0,"P"+label,"%",0,100);
      configParam(MOD_PARAM+k,0,1,0,"CV"+label,"%",0,100);
      configInput(k,label);
      configInput(MOD_INPUT+k,"Mod "+label);
      configOutput(CV_OUTPUT+k,"CV " + label);
    }
    configParam(MIX_PARAM,0,1,1,"Mix","%",0,100);
    configOutput(MIX_OUTPUT,"Mix");
  }

  void process(const ProcessArgs &args) override {
    int channels=0;
    for(int k=0;k<8;k++) {
      if(inputs[k].getChannels()>channels) channels=inputs[k].getChannels();
    }
    for(int chn=0;chn<channels;chn+=4) {
      float_4 sum=0.f;
      for(int k=0;k<8;k++) {
        float_4 param=params[k].getValue();
        param+=inputs[MOD_INPUT+k].getVoltageSimd<float_4>(chn)*0.1f*params[MOD_PARAM+k].getValue();
        float_4 out=param*inputs[k].getVoltageSimd<float_4>(chn);
        outputs[CV_OUTPUT+k].setVoltageSimd(out,chn);
        sum+=out;
      }
      outputs[MIX_OUTPUT].setVoltageSimd(sum*params[MIX_PARAM].getValue(),chn);
    }
    outputs[MIX_OUTPUT].setChannels(channels);
    for(int k=0;k<8;k++) {
      outputs[CV_OUTPUT+k].setChannels(inputs[k].getChannels());
    }
  }
};


struct VCM8Widget : ModuleWidget {
	explicit VCM8Widget(VCM8* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/VCM8.svg")));
    float x=2.f;
    float x2=10.f;
    float x3=18.f;
    float y=8;
    for(int k=0;k<8;k++) {
      addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,k));
      addParam(createParam<TrimbotWhite>(mm2px(Vec(x2,y)),module,k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(x3,y)),module,VCM8::CV_OUTPUT+k));
      addInput(createInput<SmallPort>(mm2px(Vec(x,y+59)),module,VCM8::MOD_INPUT+k));
      addParam(createParam<TrimbotWhite>(mm2px(Vec(x2,y+59)),module,VCM8::MOD_PARAM+k));
      y+=7;
    }
    addOutput(createOutput<SmallPort>(mm2px(Vec(x3,116)),module,VCM8::MIX_OUTPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x3,105.5)),module,VCM8::MIX_PARAM));
	}
};


Model* modelVCM8 = createModel<VCM8, VCM8Widget>("VCM8");