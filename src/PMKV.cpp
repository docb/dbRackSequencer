#include "plugin.hpp"


struct PMKV : Module {
  enum ParamId {
    SEED_PARAM,SIZE_PARAM,PARAMS_LEN
  };
  enum InputId {
    CLK_INPUT=16,RST_INPUT,SEED_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,TRG_OUTPUT,CLK_OUTPUT,GATE_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN=16
  };
  RND rnd;
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::PulseGenerator trgPulse[16];
  int currentState=0;

  PMKV() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    for(int k=0;k<16;k++) {
      std::string nr=std::to_string(k+1);
      configInput(k,"probabilities for step "+nr);
      configLight(k,"step "+nr);
    }
    configInput(SEED_INPUT,"Random Seed");
    configParam(SEED_PARAM,0,1,0,"Random Seed");
    configParam(SIZE_PARAM,2,32,16,"Size");
    getParamQuantity(SIZE_PARAM)->snapEnabled=true;
    configInput(CLK_INPUT,"Clock");
    configInput(RST_INPUT,"Reset");
    configOutput(CV_OUTPUT,"CV");
    configOutput(GATE_OUTPUT,"Gate");
    configOutput(CLK_OUTPUT,"Gate");
    configOutput(TRG_OUTPUT,"Trg");
  }

  void process(const ProcessArgs &args) override {
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())) {
      float seedParam;
      if(inputs[SEED_INPUT].isConnected()) {
        seedParam=clamp(inputs[SEED_INPUT].getVoltage()*0.1f);
        setImmediateValue(getParamQuantity(SEED_PARAM),seedParam);
      } else {
        seedParam=params[SEED_PARAM].getValue();
      }
      seedParam=floorf(seedParam*10000)/10000;
      auto seedInput=(unsigned long long)(floor((double)seedParam*(double)ULONG_MAX));
      rnd.reset(seedInput);
      currentState=0;
      rstPulse.trigger(0.001f);
    }
    bool resetGate=rstPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage())&&!resetGate) {
      float probs[16];
      float sum=0;
      for(int k=0;k<16;k++) {
        probs[k]=inputs[currentState].getVoltage(k)*0.1f;
        if(probs[k]<0) probs[k]=0;
        if(probs[k]>1) probs[k]=1;
        sum+=probs[k];
      }
      for(int k=0;k<16;k++) {
        probs[k]/=sum;
      }
      for(int k=1;k<16;k++) {
        probs[k]=probs[k-1]+probs[k];
      }
      auto r=float(rnd.nextDouble());
      for(int k=0;k<16;k++) {
        if(r<probs[k]) {
          currentState=k;
          break;
        }
      }
      trgPulse[currentState].trigger();
      int div=(int)params[SIZE_PARAM].getValue();
      outputs[CV_OUTPUT].setVoltage((float(currentState)/float(div))*10.f);
      for(int k=0;k<16;k++) {
        outputs[GATE_OUTPUT].setVoltage(currentState==k?10.f:0.f,k);
        lights[k].setBrightness(currentState==k?1.f:0.f);
        outputs[CLK_OUTPUT].setVoltage(currentState==k?inputs[CLK_INPUT].getVoltage():0.f,k);
      }
      outputs[TRG_OUTPUT].setVoltage(trgPulse[currentState].process(args.sampleTime)?10.f:0.f,currentState);
    }
    outputs[CLK_OUTPUT].setChannels(16);
    outputs[GATE_OUTPUT].setChannels(16);
    outputs[TRG_OUTPUT].setChannels(16);
  }
};


struct PMKVWidget : ModuleWidget {
  PMKVWidget(PMKV *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/PMKV.svg")));
    float x1=6;
    float x2=16;
    float y=11;
    for(int k=0;k<16;k++) {
      addInput(createInput<SmallPort>(mm2px(Vec(x1,y)),module,k));
      addChild(createLightCentered<DBSmallLight<GreenLight>>(mm2px(Vec(14,y+3)),module,k));
      y+=7;
    }
    addInput(createInput<SmallPort>(mm2px(Vec(x2,11)),module,PMKV::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x2,25)),module,PMKV::RST_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x2,39)),module,PMKV::SEED_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x2,46)),module,PMKV::SEED_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x2,60)),module,PMKV::GATE_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x2,74)),module,PMKV::CLK_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x2,88)),module,PMKV::TRG_OUTPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x2,102)),module,PMKV::SIZE_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x2,116)),module,PMKV::CV_OUTPUT));

  }
};


Model *modelPMKV=createModel<PMKV,PMKVWidget>("PMKV");