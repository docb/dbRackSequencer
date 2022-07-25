#include "plugin.hpp"


struct P16S : Module {
  enum ParamId {
    RUN_MODE_PARAM,PARAMS_LEN
  };
  enum InputId {
    CLK_INPUT,RST_INPUT,CV_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    CV_LIGHTS,POS_LIGHTS=CV_LIGHTS+16,LIGHTS_LEN=POS_LIGHTS+16
  };

  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::ClockDivider lightDivider;
  RND rnd;
  int stepCounter=0;
  bool reverseDirection=false;

  P16S() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configSwitch(RUN_MODE_PARAM,0,4,0,"Run Mode",{"-->","<--","<->","?-?","???"});
    configInput(CLK_INPUT,"Clock");
    configInput(RST_INPUT,"RST");
    configInput(CV_INPUT,"Poly CV");
    configOutput(CV_OUTPUT,"CV");
    lightDivider.setDivision(32);
  }

  int incStep(int step,int length,int amount) {
    return (step+amount)%length;
  }

  int decStep(int step,int length,int amount) {
    return (step-amount+length)%length;
  }

  void next(int stepLength) {
    switch(int(params[RUN_MODE_PARAM].getValue())) {
      case 0:
        stepCounter=incStep(stepCounter,stepLength,1);
        break;
      case 1:
        stepCounter=decStep(stepCounter,stepLength,1);
        break;
      case 2:
        if(reverseDirection) { //ping pong
          int step=decStep(stepCounter,stepLength,1);
          if(step>stepCounter) {
            stepCounter=incStep(stepCounter,stepLength,1);
            reverseDirection=false;
          } else {
            stepCounter=step;
          }
        } else {
          int step=incStep(stepCounter,stepLength,1);
          if(step<stepCounter) {
            stepCounter=decStep(stepCounter,stepLength,1);
            reverseDirection=true;
          } else {
            stepCounter=step;
          }
        }
        break;
      case 3:
        if(rnd.nextCoin(0.5))
          stepCounter=incStep(stepCounter,stepLength,1);
        else
          stepCounter=decStep(stepCounter,stepLength,1);
        break;
      case 4:
        stepCounter=rnd.nextRange(0,stepLength-1);
        break;
      default:
        break;
    }
  }

  void showLights(int channels) {
    for(int k=0;k<16;k++) {
      lights[POS_LIGHTS+k].setBrightness(0.f);
      float v=rescale(inputs[CV_INPUT].getVoltage(k),-10.f,10.f,0.1f,1.f);
      lights[CV_LIGHTS+k].setBrightness(k<channels?v:0.f);
    }
    lights[POS_LIGHTS+stepCounter].setBrightness(1.f);
  }

  void process(const ProcessArgs &args) override {
    int channels=inputs[CV_INPUT].getChannels();
    if(channels>0) {
      if(rstTrigger.process(inputs[RST_INPUT].getVoltage())) {
        rstPulse.trigger(0.001f);
        stepCounter=0;
      }
      bool resetGate=rstPulse.process(args.sampleTime);
      if(clockTrigger.process(inputs[CLK_INPUT].getVoltage())&&!resetGate) {
        next(channels);
      }
      outputs[CV_OUTPUT].setVoltage(inputs[CV_INPUT].getVoltage(stepCounter));
    } else {
      outputs[CV_OUTPUT].setVoltage(0.f);
    }
    if(lightDivider.process())
      showLights(channels);
  }
};


struct P16SWidget : ModuleWidget {
  P16SWidget(P16S *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/P16S.svg")));
    float x=1.9;
    float y=10;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,P16S::CLK_INPUT));
    y+=12;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,P16S::RST_INPUT));
    y+=12;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,P16S::CV_INPUT));
    y+=9;
    for(int k=0;k<16;k++) {
      addChild(createLightCentered<DBSmallLight<RedLight>>(mm2px(Vec(x+1.5,y)),module,P16S::POS_LIGHTS+k));
      addChild(createLightCentered<DBSmallLight<GreenLight>>(mm2px(Vec(x+4.5,y)),module,P16S::CV_LIGHTS+k));
      y+=3;
    }
    y=95;
    auto selectParam=createParam<SelectParam>(mm2px(Vec(x-0.3,y)),module,P16S::RUN_MODE_PARAM);
    selectParam->box.size=mm2px(Vec(7,3.2*5));
    selectParam->init({"-->","<--","<->","?-?","???"});
    addParam(selectParam);
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,116)),module,P16S::CV_OUTPUT));
  }
};


Model *modelP16S=createModel<P16S,P16SWidget>("P16S");