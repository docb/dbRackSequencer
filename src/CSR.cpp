#include "plugin.hpp"

struct CSR : Module {
  ShiftRegister<float,16> sr;
	enum ParamId {
		CHN_PARAM,PARAMS_LEN
	};
	enum InputId {
		CLK_INPUT,RST_INPUT,CV_INPUT,SEED_INPUT,INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT,CHN_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger resetTrigger;
  dsp::SchmittTrigger setTrigger;
  dsp::PulseGenerator resetPulse;
  int counter=0;
	CSR() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configInput(CLK_INPUT,"Clock");
    configInput(RST_INPUT,"Reset");
    configInput(CV_INPUT,"CV");
    configInput(SEED_INPUT,"Seed");
    configParam(CHN_PARAM,1,16,1,"Out channel");
    getParamQuantity(CHN_PARAM)->snapEnabled=true;
    configOutput(CHN_OUTPUT,"CV");
    configOutput(CV_OUTPUT,"Poly CV");
	}

	void process(const ProcessArgs& args) override {

    if(resetTrigger.process(inputs[RST_INPUT].getVoltage())) {
      sr.reset();
      if(inputs[SEED_INPUT].isConnected()) {
        for(int k=0;k<16;k++) {
          sr.set(k,inputs[SEED_INPUT].getVoltage(k));
        }
      } else {
        sr.step(inputs[CV_INPUT].getVoltage());
      }
      for(unsigned k=0;k<16;k++) {
        outputs[CV_OUTPUT].setVoltage(0.f,k);
      }
      resetPulse.trigger(0.001f);
    }
    bool resetGate=resetPulse.process(args.sampleTime);
    if(clockTrigger.process(inputs[CLK_INPUT].getVoltage())&&!resetGate) {
      counter=10;
    }
    if(counter==0) {
      sr.step(inputs[CV_INPUT].getVoltage());
    }
    if(counter>=0) {
      counter--;
    }
    for(unsigned k=0;k<16;k++) {
      outputs[CV_OUTPUT].setVoltage(sr.get(k),k);
    }
    outputs[CHN_OUTPUT].setVoltage(sr.get(params[CHN_PARAM].getValue()-1));
    outputs[CV_OUTPUT].setChannels(16);
	}
};


struct CSRWidget : ModuleWidget {
	CSRWidget(CSR* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/CSR.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    float x=1.9;
    addInput(createInput<SmallPort>(mm2px(Vec(x,18)),module,CSR::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x,30)),module,CSR::RST_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x,42)),module,CSR::CV_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x,54)),module,CSR::SEED_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,88)),module,CSR::CHN_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,100)),module,CSR::CHN_OUTPUT));
	  addOutput(createOutput<SmallPort>(mm2px(Vec(x,112)),module,CSR::CV_OUTPUT));
  }
};


Model* modelCSR = createModel<CSR, CSRWidget>("CSR");