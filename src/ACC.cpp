#include "plugin.hpp"


struct ACC : Module {
	enum ParamId {
		STEP_AMT_PARAM,SET_PARAM,PARAMS_LEN
	};
	enum InputId {
    CLOCK_INPUT,RST_INPUT,STEP_AMT_INPUT,SET_INPUT,INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
  float state = 0;
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::ClockDivider divider;
	ACC() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configParam(STEP_AMT_PARAM,-1,1,0,"Step Amount");
    configParam(SET_PARAM,-10,10,0,"Set");
    configInput(CLOCK_INPUT,"Clock");
    configInput(RST_INPUT,"Reset");
    configInput(STEP_AMT_INPUT,"Step Amount");
    configInput(SET_INPUT,"Set");
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
    }
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())) {
      rstPulse.trigger(0.001f);
      state = params[SET_PARAM].getValue();
    }
    bool restGate=rstPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLOCK_INPUT].getVoltage()) & !restGate) {
        float amount=params[STEP_AMT_PARAM].getValue();
        state+=amount;
    }

    outputs[CV_OUTPUT].setVoltage(state);
	}
};


struct ACCWidget : ModuleWidget {
	ACCWidget(ACC* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ACC.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    float xpos=1.9f;
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,MHEIGHT-115.f)),module,ACC::CLOCK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,MHEIGHT-103.f)),module,ACC::RST_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(2,MHEIGHT-91.0f)),module,ACC::SET_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,MHEIGHT-83.f)),module,ACC::SET_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(2,MHEIGHT-71.0f)),module,ACC::STEP_AMT_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(xpos,MHEIGHT-63.f)),module,ACC::STEP_AMT_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(xpos,MHEIGHT-19.f)),module,ACC::CV_OUTPUT));
  }
};


Model* modelACC = createModel<ACC, ACCWidget>("ACC");