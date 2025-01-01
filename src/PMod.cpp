#include "plugin.hpp"


struct PMod : Module {
  enum ParamId {
    SIZE_PARAM,MOD_PARAM,MULT_PARAM,OFS_PARAM,PARAMS_LEN
  };
  enum InputId {
    CLK_INPUT,RST_INPUT,MULT_INPUT,MOD_INPUT,OFS_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };

  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::PulseGenerator rstPulse;
  int stepCounter=0;
  bool resetOnOffset=true;

  PMod() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(OFS_PARAM,0,31,0,"Offset");
    getParamQuantity(OFS_PARAM)->snapEnabled=true;
    configParam(MOD_PARAM,2,32,16,"Mod");
    getParamQuantity(MOD_PARAM)->snapEnabled=true;
    configParam(MULT_PARAM,1,32,1,"Multiply");
    getParamQuantity(MULT_PARAM)->snapEnabled=true;
    configParam(SIZE_PARAM,2,32,16,"Size");
    getParamQuantity(SIZE_PARAM)->snapEnabled=true;
    configInput(OFS_INPUT,"Offset");
    configInput(CLK_INPUT,"Clock");
    configInput(RST_INPUT,"Reset");
    configInput(MOD_INPUT,"Mod");
    configInput(MULT_INPUT,"Mult");
    configOutput(CV_OUTPUT,"CV");
  }

  void process(const ProcessArgs &args) override {
    if(inputs[MOD_INPUT].isConnected()) {
      int c=clamp(inputs[MOD_INPUT].getVoltage(),0.f,9.99f)*3.2f;
      setImmediateValue(getParamQuantity(MOD_PARAM),c);
    }
    bool changed=false;
    if(inputs[OFS_INPUT].isConnected()) {
      int old=params[OFS_PARAM].getValue();
      int c=clamp(inputs[OFS_INPUT].getVoltage(),0.f,9.99f)*1.6f;
      setImmediateValue(getParamQuantity(OFS_PARAM),c);
      changed=resetOnOffset&&(old!=c);
    }
    if(inputs[MULT_INPUT].isConnected()) {
      int c=clamp(inputs[MULT_INPUT].getVoltage(),0.f,9.99f)*3.2f;
      setImmediateValue(getParamQuantity(MULT_PARAM),c);
    }

    bool advance=false;
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())||changed) {
      stepCounter=0;
      rstPulse.trigger(0.001f);
      advance=true;
    }
    bool resetGate=rstPulse.process(args.sampleTime);
    int mod=params[MOD_PARAM].getValue();
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage())&&!resetGate) {
      stepCounter++;
      if(stepCounter==mod)
        stepCounter=0;
      advance=true;
    }
    if(advance) {
      int mult=params[MULT_PARAM].getValue();
      int ofs=params[OFS_PARAM].getValue();
      int index=(stepCounter*mult+ofs)%mod;
      int div=params[SIZE_PARAM].getValue();
      outputs[CV_OUTPUT].setVoltage((float(index)/float(div))*10.f);
    }
  }
};


struct PModWidget : ModuleWidget {
  PModWidget(PMod *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/PMod.svg")));

    float xpos=1.9f;
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,9)),module,PMod::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,21)),module,PMod::RST_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(xpos,38)),module,PMod::OFS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,46)),module,PMod::OFS_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(xpos,58)),module,PMod::MOD_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,66)),module,PMod::MOD_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(xpos,78)),module,PMod::MULT_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,86)),module,PMod::MULT_INPUT));

    addParam(createParam<TrimbotWhite>(mm2px(Vec(xpos,104)),module,PMod::SIZE_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(xpos,116)),module,PMod::CV_OUTPUT));


  }
};


Model *modelPMod=createModel<PMod,PModWidget>("PMod");