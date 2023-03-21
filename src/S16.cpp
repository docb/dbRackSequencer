#include "plugin.hpp"

struct S16 : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
    POLY_CV_INPUT,INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN=16
	};
	enum LightId {
		LIGHTS_LEN
	};

	S16() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    for(int k=0;k<16;k++) {
      std::string label=std::to_string(k+1);
      configOutput(k,"Chn " + label);
    }
    configInput(POLY_CV_INPUT,"Poly");
	}

	void process(const ProcessArgs& args) override {
    for(int k=0;k<16;k++) {
      outputs[k].setVoltage(inputs[POLY_CV_INPUT].getVoltage(k));
    }
	}
};


struct S16Widget : ModuleWidget {
	S16Widget(S16* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/S16.svg")));

		float x=1.9;
    float y=4;
    for(int k=0;k<16;k++) {
      addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,k));
      y+=7;
    }
    y=118.5;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,S16::POLY_CV_INPUT));
	}
};


Model* modelS16 = createModel<S16, S16Widget>("S16");