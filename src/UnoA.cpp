#include "plugin.hpp"
#define NUM_STEPS 16

struct UnoA : Module {
	enum ParamId {
		DEFAULT_PROP_PARAM,DIR_PARAM,SIZE_PARAM,PARAMS_LEN
	};
	enum InputId {
		CLK_INPUT,RST_INPUT,SEED_INPUT,PROB_INPUT,SR_INPUT,INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN=16
	};
  int stepCounter=0;
  dsp::PulseGenerator rstPulse;
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::ClockDivider lightDivider;
  bool forward=true;
  RND rnd;

  UnoA() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configSwitch(DIR_PARAM,0,2,0,"Direction",{"-->","<--","<->"});
    configParam(SIZE_PARAM,4,32,32,"Pattern Size");
    getParamQuantity(SIZE_PARAM)->snapEnabled=true;
    configParam(DEFAULT_PROP_PARAM,0,1,1,"Default Probability");
    configInput(CLK_INPUT,"Clock");
    configInput(RST_INPUT,"Rst");
    configInput(PROB_INPUT,"Probability");
    configInput(SEED_INPUT,"Seed");
    configInput(SR_INPUT,"Sequence Reset");
    configOutput(CV_OUTPUT,"CV");
  }

  void resetCurrentStep(int direction) {
    switch(direction) {
      case 0:  // forward
        stepCounter=NUM_STEPS-1;
        break;
      case 1: // backward
        stepCounter=0;
        break;
      case 2:
        stepCounter=1;
        forward=false;
        break;
    }
    next(direction);
  }

  void next(int direction) {

    if(inputs[SR_INPUT].getVoltage(stepCounter)>1.f) {
      if((stepCounter==NUM_STEPS-1 && direction==0)||(stepCounter==0&&direction==1)) {}
      else {
        resetCurrentStep(direction);
        return;
      }
    }
    for(int k=0;k<NUM_STEPS*2;k++) {
      switch(direction) {
        case 0:  // forward
          if(stepCounter>=NUM_STEPS-1)
            stepCounter=0;
          else
            stepCounter++;
          break;
        case 1: // backward
          if(stepCounter<=0)
            stepCounter=NUM_STEPS-1;
          else
            stepCounter--;
          break;
        case 2: // <->
          if(forward) {
            if(stepCounter>=NUM_STEPS-1) {
              stepCounter=NUM_STEPS-1;
              forward=false;
            } else {
              stepCounter++;
            }
          } else {
            if(stepCounter<=0) {
              stepCounter=1;
              forward=true;
            } else {
              stepCounter--;
            }
          }
          break;
      }
      float prob;
      if(inputs[PROB_INPUT].isConnected()) {
        prob=clamp(inputs[PROB_INPUT].getVoltage(stepCounter),-5.f,5.f)/10.f+0.5f;
      } else {
        prob=params[DEFAULT_PROP_PARAM].getValue();
      }
      double d=rnd.nextDouble();
      //INFO("%d prop %f %lf",stepCounter,prob,d);
      if(d>=(1-prob)) break;
    }
  }

	void process(const ProcessArgs& args) override {
    int direction=params[DIR_PARAM].getValue();;
    bool advance=false;
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())) {
      rstPulse.trigger(0.001f);
      float seedParam=0;
      if(inputs[SEED_INPUT].isConnected()) {
        seedParam=inputs[SEED_INPUT].getVoltage();
        seedParam = floorf(seedParam*10000)/10000;
        seedParam/=10.f;
      }
      auto seedInput = (unsigned long long)(floor((double)seedParam*(double)ULONG_MAX));
      rnd.reset(seedInput);
      resetCurrentStep(direction);
      advance=true;
    }
    bool resetGate=rstPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage())&&!resetGate) {
      next(direction);
      advance=true;
    }
    if(advance) {
      int size = params[SIZE_PARAM].getValue();
      outputs[CV_OUTPUT].setVoltage(10.f*(stepCounter%size)/float(size));
    }
    if(lightDivider.process()) {
      for(int k=0;k<NUM_STEPS;k++) {
        if(k==stepCounter) {
          lights[k].setSmoothBrightness(1.f,0.1f);
        }
        else {
          lights[k].setSmoothBrightness(0.f,0.1f);
        }
      }
    }

  }
};


struct UnoAWidget : ModuleWidget {
	UnoAWidget(UnoA* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/UnoA.svg")));

    float x=1.9;
    addInput(createInput<SmallPort>(mm2px(Vec(x,9)),module,UnoA::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x,21)),module,UnoA::RST_INPUT));
    auto selectParam=createParam<SelectParam>(mm2px(Vec(x,36)),module,UnoA::DIR_PARAM);
    selectParam->box.size=mm2px(Vec(7,3.2*3));
    selectParam->init({"-->","<--","<->"});
    addParam(selectParam);
    addInput(createInput<SmallPort>(mm2px(Vec(x,56)),module,UnoA::SEED_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,68)),module,UnoA::DEFAULT_PROP_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x,76)),module,UnoA::PROB_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x,92)),module,UnoA::SR_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,104)),module,UnoA::SIZE_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,116)),module,UnoA::CV_OUTPUT));
	}
};


Model* modelUnoA = createModel<UnoA, UnoAWidget>("UnoA");