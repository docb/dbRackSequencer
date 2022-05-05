#include "plugin.hpp"
#include "SEMessage.hpp"
extern Model *modelSE;
struct Sum : Module {
	enum ParamId {
		PARAMS_LEN=NUM_INPUTS
	};
	enum InputId {
		INPUTS_LEN=NUM_INPUTS
	};
	enum OutputId {
		SUM_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
  SEMessage rightMessages[2][1];
	Sum() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    for(int k=0;k<NUM_INPUTS;k++) {
      configParam(k,0,2,1,"Sign " + std::to_string(k+1));
      getParamQuantity(k)->snapEnabled=true;
      configInput(k,"CV");
    }
    configOutput(SUM_OUTPUT,"Sum");
    rightExpander.producerMessage = rightMessages[0];
    rightExpander.consumerMessage = rightMessages[1];
	}

  void processChannel(int chn) {
    float sum=0;
    for(int k=0;k<NUM_INPUTS;k++) {
      if(inputs[k].isConnected()) {
        sum+= inputs[k].getPolyVoltage(chn)*(params[k].getValue()-1);
      }
    }
    outputs[SUM_OUTPUT].setVoltage(sum,chn);
  }

  void populateExpanderMessage(SEMessage *message,int channels) {
    message->channels=channels;
    for(int chn=0;chn<channels;chn++) {
      for(int k=0;k<NUM_INPUTS;k++) {
        if(inputs[k].isConnected()) {
          message->ins[k][chn]=inputs[k].getPolyVoltage(chn);
        } else {
          message->ins[k][chn]=0.f;
        }
      }
    }
  }

	void process(const ProcessArgs& args) override {
    int maxChannels=0;
    for(int k=0;k<NUM_INPUTS;k++) {
      if(inputs[k].getChannels()>maxChannels) maxChannels=inputs[k].getChannels();
    }
    if(maxChannels==0) maxChannels=1;
    for(int chn=0;chn<maxChannels;chn++) {
      processChannel(chn);
    }
    outputs[SUM_OUTPUT].setChannels(maxChannels);
    if (rightExpander.module) {
      if(rightExpander.module->model==modelSE) {
        SEMessage *messageToExpander=(SEMessage*)(rightExpander.module->leftExpander.producerMessage);
        populateExpanderMessage(messageToExpander,maxChannels);
        rightExpander.module->leftExpander.messageFlipRequested = true;
      }
    }
	}
};


struct SumWidget : ModuleWidget {
	SumWidget(Sum* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Sum.svg")));

    addChild(createWidget<ScrewSilver>(Vec(0, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 0)));
    addChild(createWidget<ScrewSilver>(Vec(0, 365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

    float x=3;
    float y=TY(109);
    for(int k=0;k<NUM_INPUTS;k++) {
      addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,k));
      auto selectParam=createParam<SelectParamH>(mm2px(Vec(x+8,y+0.75)),module,k);
      selectParam->box.size=mm2px(Vec(7.5,5));
      selectParam->init({" "," "," "});
      addParam(selectParam);
      y+=8;
    }
    addOutput(createOutput<SmallPort>(mm2px(Vec(11,TY(9))),module,Sum::SUM_OUTPUT));
	}
};


Model* modelSum = createModel<Sum, SumWidget>("Sum");