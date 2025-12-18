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
  bool curHigh = true;
  bool prevHigh = false;
  unsigned long  currentPos=0;
  unsigned long edges=0;
  bool orig_pw=true;
  bool roundSteps=false;

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
          float f=clamp(inputs[DIV_INPUT+k].getVoltage(),0.f,9.99f)*float(MAX_DIV)/10.f;
          int c;
          if(roundSteps) c=(int)std::round(f); else c=(int)f;
          setImmediateValue(getParamQuantity(DIV_PARAM+k),float(c));
        }
      }
    }

    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())) {
      rstPulse.trigger(0.001f);
      currentPos=0;
      edges=0;
      curHigh=true;
      prevHigh=false;
    } else {
      prevHigh=curHigh;
      curHigh=clockTrigger.isHigh();
      if ((curHigh && !prevHigh) || (!curHigh && prevHigh)) {
        edges++;
      }
    }
    bool resetGate=rstPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage()) && !resetGate) {
      currentPos++;
    }
    for(int k=0;k<NUM_DIV;k++) {
      int currentDiv=params[DIV_PARAM+k].getValue();
      if (orig_pw) {
	      bool gate = currentPos%currentDiv==0;
	      outputs[CLK_OUTPUT+k].setVoltage(gate?inputs[CLK_INPUT].getVoltage():0.f);
      } else {
	      bool gate = edges%(currentDiv*2) < (unsigned long)currentDiv;
	      outputs[CLK_OUTPUT+k].setVoltage(gate?10.f:0.f);
      }
    }
	}

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_object_set_new(data,"roundSteps",json_boolean(roundSteps));
    json_object_set_new(data, "origPW", json_boolean(orig_pw));
    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    json_t *jRoundSteps=json_object_get(rootJ,"roundSteps");
    if(jRoundSteps)
      roundSteps=json_boolean_value(jRoundSteps);

    json_t *jOrigPW = json_object_get(rootJ, "origPW");
    if (jOrigPW)
      orig_pw = json_boolean_value(jOrigPW);
  }


};

struct DivisionSelect : SpinParamWidget {
  CDiv *module;
  DivisionSelect() {
    init();
  }
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

  void appendContextMenu(Menu* menu) override {
    CDiv* module = getModule<CDiv>();
    menu->addChild(new MenuSeparator);
    menu->addChild(createBoolPtrMenuItem("Keep input clock original pulse width", "", &module->orig_pw));
    menu->addChild(createBoolPtrMenuItem("Round division input","",&module->roundSteps));
  }
};


Model* modelCDiv = createModel<CDiv, CDivWidget>("CDiv");
