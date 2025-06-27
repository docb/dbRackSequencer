#include "plugin.hpp"


struct MTD4 : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		ADDR_INPUT, CV_INPUT, INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT, TRIG_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
  float lastCV[16]={};
  dsp::PulseGenerator triggers[16];
	MTD4() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configInput(ADDR_INPUT,"Address CV");
    configInput(CV_INPUT,"CV");
    configOutput(CV_OUTPUT,"CV ");
    configOutput(TRIG_OUTPUT,"Change cv trigger");
	}

	void process(const ProcessArgs& args) override {
    int channels=inputs[ADDR_INPUT].getChannels();
    for(int k=0;k<channels;k++) {
      int index=int(inputs[ADDR_INPUT].getVoltage(k)*1.6f);
      while(index<0)
        index+=16;
      index%=16;
      float a=inputs[CV_INPUT].getVoltage(index);
      if(a!=lastCV[k]) {
        triggers[k].trigger();
        lastCV[k]=a;
      }
      outputs[CV_OUTPUT].setVoltage(a,k);
      outputs[TRIG_OUTPUT].setVoltage(triggers[k].process(args.sampleTime)?10.f:0.f,k);
    }
    outputs[CV_OUTPUT].setChannels(channels);
    outputs[TRIG_OUTPUT].setChannels(channels);
	}
};


struct MTD4Widget : ModuleWidget {
	MTD4Widget(MTD4* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/MTD4.svg")));
    float x=1.9;
    addInput(createInput<SmallPort>(mm2px(Vec(x,11)),module,MTD4::ADDR_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x,23)),module,MTD4::CV_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,104)),module,MTD4::TRIG_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,116)),module,MTD4::CV_OUTPUT));
	}
};


Model* modelMTD4 = createModel<MTD4, MTD4Widget>("MTD4");