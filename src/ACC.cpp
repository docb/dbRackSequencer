#include "plugin.hpp"


struct ACC : Module {
	enum ParamId {
		STEP_AMT_PARAM,SET_PARAM,RST_PARAM,THRESHOLD_PARAM,PARAMS_LEN
	};
	enum InputId {
    CLOCK_INPUT,RST_INPUT,STEP_AMT_INPUT,SET_INPUT,THRESHOLD_INPUT,INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT,TRIG_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
  float state = 0;
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::SchmittTrigger manualRstTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::PulseGenerator trigPulse;
  dsp::ClockDivider divider;
	ACC() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configParam(STEP_AMT_PARAM,-1,1,0,"Step Amount");
    configParam(SET_PARAM,-10,10,0,"Set");
    configParam(THRESHOLD_PARAM,-10,10,0,"Trigger Threshold");
    configButton(RST_PARAM,"Reset");
    configInput(CLOCK_INPUT,"Clock");
    configInput(RST_INPUT,"Reset");
    configInput(STEP_AMT_INPUT,"Step Amount");
    configInput(SET_INPUT,"Set");
    configInput(THRESHOLD_INPUT,"Trigger Threshold");
    configOutput(TRIG_OUTPUT,"Trig");
    configOutput(CV_OUTPUT,"CV");
    divider.setDivision(32);
	}

	void process(const ProcessArgs& args) override {
    if(divider.process()) {
      if(inputs[SET_INPUT].isConnected()) {
        getParamQuantity(SET_PARAM)->setValue(inputs[SET_INPUT].getVoltage());
      }
      if(inputs[STEP_AMT_INPUT].isConnected()) {
        getParamQuantity(STEP_AMT_PARAM)->setValue(clamp(inputs[STEP_AMT_INPUT].getVoltage(),-10.f,10.f)/10.f);
      }
      if(inputs[THRESHOLD_INPUT].isConnected()) {
        getParamQuantity(THRESHOLD_PARAM)->setValue(inputs[THRESHOLD_INPUT].getVoltage());
      }
    }
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())|manualRstTrigger.process(params[RST_PARAM].getValue())) {
      rstPulse.trigger(0.001f);
      state = params[SET_PARAM].getValue();
    }
    bool restGate=rstPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLOCK_INPUT].getVoltage()) & !restGate) {
        float amount=params[STEP_AMT_PARAM].getValue();
        float threshold=params[THRESHOLD_PARAM].getValue();
        if(amount<0) {
          if(state>threshold && state+amount <=threshold) {
            trigPulse.trigger();
          }
        } else {
          if(state<threshold && state+amount >=threshold) {
            trigPulse.trigger();
          }
        }
        state+=amount;
    }
    outputs[TRIG_OUTPUT].setVoltage(trigPulse.process(args.sampleTime)?10.f:0.f);
    outputs[CV_OUTPUT].setVoltage(state);
	}
};


struct ACCWidget : ModuleWidget {
	ACCWidget(ACC* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ACC.svg")));

    float x=1.9f;
    float y=10;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,ACC::CLOCK_INPUT));
    y+=12;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,ACC::RST_INPUT));
    y+=8;
    addParam(createParam<MLEDM>(mm2px(Vec(x,y)),module,ACC::RST_PARAM));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,ACC::SET_PARAM));
    y+=8;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,ACC::SET_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,ACC::STEP_AMT_PARAM));
    y+=8;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,ACC::STEP_AMT_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,ACC::THRESHOLD_PARAM));
    y+=8;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,ACC::THRESHOLD_INPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,ACC::TRIG_OUTPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,ACC::CV_OUTPUT));
  }
};


Model* modelACC = createModel<ACC, ACCWidget>("ACC");