#include "plugin.hpp"


struct AD : Module {
	enum ParamId {
		BIPOLAR_PARAM,PARAMS_LEN
	};
	enum InputId {
		CV_INPUT, CLK_INPUT,INPUTS_LEN
	};
	enum OutputId {
		POLY_OUTPUT=8,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN=8
	};
  dsp::SchmittTrigger clock;
	AD() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configInput(CV_INPUT,"CV");
    configInput(CLK_INPUT,"Clk");
    configButton(BIPOLAR_PARAM,"BiPolar");
    for(int k=0;k<8;k++) {
      std::string label="Bit "+std::to_string(k+1);
      configOutput(k,label);
    }
    configOutput(POLY_OUTPUT,"All Bits");
	}

  void processBits() {
    float input = inputs[CV_INPUT].getVoltage() * 0.2f;
    if (params[BIPOLAR_PARAM].getValue() > 0) {
      input = clamp(input, -1.0f, 1.0f);
      input = (input + 1.0f) * 0.5f;
    } else {
      input = clamp(input / 2.f, .0f, 1.0f);
    }
    auto bitValue = uint32_t(std::round(input * float((1 << 8) - 1)));
    for (int k = 0; k <= 8; k++) {
      uint32_t mask = 1 << k;
      outputs[k].setVoltage(bitValue & mask ? 10.f : 0.f);
      outputs[POLY_OUTPUT].setVoltage(bitValue & mask ? 10.f : 0.f, k);
      lights[k].setBrightness(bitValue & mask ? 1.f : 0.f);
    }
    outputs[POLY_OUTPUT].setChannels(8);
  }

	void process(const ProcessArgs& args) override {
    if (inputs[CLK_INPUT].isConnected()) {
      if (clock.process(inputs[CLK_INPUT].getVoltage())) {
        if (inputs[CV_INPUT].isConnected())
          processBits();
      }
    } else {
      processBits();
    }
  }
};


struct ADWidget : ModuleWidget {
	explicit ADWidget(AD* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/AD.svg")));
    float x=1.9;
    float y=8;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,AD::CV_INPUT));
    y+=12;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,AD::CLK_INPUT));
    y+=12;
    auto biPolarButton=createParam<SmallButtonWithLabel>(mm2px(Vec(x,y)),module,AD::BIPOLAR_PARAM);
    biPolarButton->setLabel("BiP");
    addParam(biPolarButton);
    y=55;
    for(int k=0;k<8;k++) {
      addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,k));
      addChild(createLight<TinySimpleLight<GreenLight>>(mm2px(Vec(x+6.5f,y+4.5f)),module,k));
      y+=7;
    }
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,116)),module,AD::POLY_OUTPUT));

	}
};


Model* modelAD = createModel<AD, ADWidget>("AD");