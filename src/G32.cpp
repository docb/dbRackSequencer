#include "plugin.hpp"


struct G32 : Module {
  enum ParamId {
    BI_PARAM,METHOD_PARAM,PARAMS_LEN
  };
  enum InputId {
    CV_INPUT,CLK_INPUT,METHOD_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV1_OUTPUT,CV2_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  dsp::SchmittTrigger clock;
  RND rnd;


  G32() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configButton(BI_PARAM,"BiPolar");
    configInput(CV_INPUT,"CV");
    configInput(METHOD_INPUT,"Method");
    configInput(CLK_INPUT,"Clk");
    configOutput(CV1_OUTPUT,"CV1");
    configOutput(CV2_OUTPUT,"CV2");
    configSwitch(METHOD_PARAM,0,2,0,"algo",{"ieee","round","rnd"});
    getParamQuantity(METHOD_PARAM)->snapEnabled=true;
	}

  void out(uint32_t bitValue) {
    unsigned k=0;
    for(;k<16;k++) {
      outputs[CV1_OUTPUT].setVoltage((1<<k)&bitValue?10.f:0.f,k);
    }
    for(;k<32;k++) {
      outputs[CV2_OUTPUT].setVoltage((1<<k)&bitValue?10.f:0.f,k-16);
    }

  }

  uint32_t conv() {
    double input=inputs[CV_INPUT].getVoltage()*0.2;
    if(params[BI_PARAM].getValue()>0) {
      input=clampd(input,-1.0,1.0);
      input=(input+1.0f)*0.5f;
    } else {
      input=clampd(input/2,.0,1.0);
    }
    return uint32_t(std::round(input * float(UINT32_MAX)));
  }

  void floatRnd() {
    rnd.reset(conv());
    out(rnd.next());
  }

  void float2int() {
    out(conv());
  }

  void floatcast() {
    float input=inputs[CV_INPUT].getVoltage();
    auto bitValue=*(reinterpret_cast<uint32_t *>(&input));
    out(bitValue);
  }

  void process() {
    if(inputs[METHOD_INPUT].isConnected()) {
      setImmediateValue(getParamQuantity(METHOD_PARAM),inputs[METHOD_INPUT].getVoltage());
    }
    auto method=(uint8_t )params[METHOD_PARAM].getValue();
    switch(method) {
      case 0: floatcast();break;
      case 1: float2int();break;
      default: floatRnd();break;
    }
  }

  void process(const ProcessArgs &args) override {
    if(inputs[CLK_INPUT].isConnected()) {
      if(clock.process(inputs[CLK_INPUT].getVoltage())) {
        if(inputs[CV_INPUT].isConnected())
          process();
      }
    } else {
      process();
    }


    outputs[CV1_OUTPUT].setChannels(16);
    outputs[CV2_OUTPUT].setChannels(16);
  }
};


struct G32Widget : ModuleWidget {
	G32Widget(G32* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/G32.svg")));
    float x=1.9f;
    addInput(createInput<SmallPort>(mm2px(Vec(x,11)),module,G32::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x,23)),module,G32::CV_INPUT));
    auto biPolarButton=createParam<SmallButtonWithLabel>(mm2px(Vec(1.5,31)),module,G32::BI_PARAM);
    biPolarButton->setLabel("BiP");
    addParam(biPolarButton);
    auto selectParam=createParam<SelectParam>(mm2px(Vec(1.5,44)),module,G32::METHOD_PARAM);
    selectParam->box.size=mm2px(Vec(7,12));
    selectParam->init({"ieee","int","rnd"});
    addParam(selectParam);
    addInput(createInput<SmallPort>(mm2px(Vec(x,58)),module,G32::METHOD_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,116)),module,G32::CV2_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,108)),module,G32::CV1_OUTPUT));

	}
};


Model* modelG32 = createModel<G32, G32Widget>("G32");