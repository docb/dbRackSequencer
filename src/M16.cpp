#include "plugin.hpp"

struct M16 : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN=16
	};
	enum OutputId {
		POLY_CV_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	M16() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    for(int k=0;k<16;k++) {
      std::string label=std::to_string(k+1);
      configInput(k,"Chn "+label);
    }
    configOutput(POLY_CV_OUTPUT,"Poly");
	}

	void process(const ProcessArgs& args) override {
    int channels=0;
    for(int k=0;k<16;k++) {
      if(inputs[k].isConnected()) {
        channels=k+1;
        outputs[POLY_CV_OUTPUT].setVoltage(inputs[k].getVoltage(),k);
      } else {
        outputs[POLY_CV_OUTPUT].setVoltage(0.f,k);
      }
    }
    outputs[POLY_CV_OUTPUT].setChannels(channels);
	}
};


struct M16Widget : ModuleWidget {
	M16Widget(M16* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/M16.svg")));
    float x=1.9;
    float y=4;
    for(int k=0;k<16;k++) {
      addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,k));
      y+=7;
    }
    y=118.5;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,M16::POLY_CV_OUTPUT));
	}
};


Model* modelM16 = createModel<M16, M16Widget>("M16");