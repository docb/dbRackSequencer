#include "plugin.hpp"

#define MAX_DIV 100
#define NUM_DIV 3
struct CDiv : Module {
	enum ParamId {
		DIV_PARAM,PARAMS_LEN=DIV_PARAM+NUM_DIV
	};
	enum InputId {
		CLK_INPUT, RST_INPUT, DIV_INPUT,INPUTS_LEN=DIV_INPUT+NUM_DIV
	};
	enum OutputId {
		CLK_OUTPUT,OUTPUTS_LEN=CLK_OUTPUT+NUM_DIV
	};
	enum LightId {
		LIGHTS_LEN
	};
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::ClockDivider paramDivider;
  unsigned long  currentPos=0;
	CDiv() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configInput(CLK_INPUT,"Clock");
    configInput(RST_INPUT,"Reset");
    for(int k=0;k<NUM_DIV;k++) {
      configParam(DIV_PARAM+k,1,99,1,"Division " + std::to_string(k+1));
      getParamQuantity(DIV_PARAM+k)->snapEnabled=true;
      configInput(DIV_INPUT+k,"Division " + std::to_string(k+1));
      configOutput(CLK_OUTPUT+k,"Clock " + std::to_string(k+1));
    }
    paramDivider.setDivision(32);
	}

	void process(const ProcessArgs& args) override {
    if(paramDivider.process()) {
      for(int k=0;k<NUM_DIV;k++) {
        if(inputs[DIV_INPUT+k].isConnected()) {
          int c=clamp(inputs[DIV_INPUT+k].getVoltage(),0.f,9.99f)*float(MAX_DIV)/10.f;
          setImmediateValue(getParamQuantity(DIV_PARAM+k),c);
        }
      }
    }

    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())) {
      rstPulse.trigger(0.001f);
      currentPos=0;
    }
    bool resetGate=rstPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage()) && !resetGate) {
      currentPos++;
    }
    for(int k=0;k<NUM_DIV;k++) {
      int currentDiv=params[DIV_PARAM+k].getValue();
      bool gate = currentPos%currentDiv==0;
      outputs[CLK_OUTPUT+k].setVoltage(gate?inputs[CLK_INPUT].getVoltage():0.f);
    }
	}
};

struct DivisionSelect : SpinParamWidget {
  CDiv *module;
  DivisionSelect() {
    init();
  }
  //void onChange(const event::Change &e) override {
  //  if(module)
  //    module->switchPattern();
  //}
};

struct CDivWidget : ModuleWidget {
	CDivWidget(CDiv* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/CDiv.svg")));

    float x=1.9;
    float y=10;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,CDiv::CLK_INPUT));
    y+=12;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,CDiv::RST_INPUT));
    y+=14;
    for(int k=0;k<NUM_DIV;k++) {
      auto divParam=createParam<DivisionSelect>(mm2px(Vec(x-0.3,y)),module,CDiv::DIV_PARAM+k);
      divParam->module=module;
      addParam(divParam);
      addInput(createInput<SmallPort>(mm2px(Vec(x,y+10)),module,CDiv::DIV_INPUT+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(x,y+18)),module,CDiv::CLK_OUTPUT+k));
      y+=30;
    }
	}
};


Model* modelCDiv = createModel<CDiv, CDivWidget>("CDiv");