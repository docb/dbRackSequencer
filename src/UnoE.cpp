#include "plugin.hpp"
#include "Uno.hpp"
extern Model *modelUno;
extern Model *modelUnoE;

struct UnoE : Module {
	enum ParamId {
		DIR_PARAM,PARAMS_LEN
	};
	enum InputId {
    CLK_INPUT,RST_INPUT,SEED_INPUT,INPUTS_LEN
	};
	enum OutputId {
    CV_OUTPUT,GATE_OUTPUT,STEP_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN=NUM_STEPS
	};

  UnoStrip<UnoE> unoStrip;
  UnoExpanderMessage rightMessages[2][1];
  UnoExpanderMessage leftMessages[2][1];
  UnoExpanderMessage *currentMessage;
	UnoE() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    for(int k=0;k<8;k++) {
      configLight(k,"Step "+std::to_string(k+1));
    }
    configSwitch(DIR_PARAM,0,2,0,"Direction",{"-->","<--","<->"});
    configInput(CLK_INPUT,"Clock");
    configInput(RST_INPUT,"Rst");
    configInput(SEED_INPUT,"Random Seed");
    configOutput(CV_OUTPUT,"CV");
    configOutput(GATE_OUTPUT,"GATE");
    configOutput(STEP_OUTPUT,"Step");
    rightExpander.producerMessage=rightMessages[0];
    rightExpander.consumerMessage=rightMessages[1];
    leftExpander.producerMessage=leftMessages[0];
    leftExpander.consumerMessage=leftMessages[1];
    unoStrip.module=this;
	}
  float getCV(int step) {
    return currentMessage->cv[step];
  }
  float getProb(int step) {
    return currentMessage->prob[step];
  }
  bool getGlide(int step) {
    return currentMessage->glide[step];
  }
  bool getRst(int step) {
    return currentMessage->rst[step];
  }
	void process(const ProcessArgs& args) override {
    if(leftExpander.module) {
      if(leftExpander.module->model==modelUno||leftExpander.module->model==modelUnoE) {
        currentMessage=(UnoExpanderMessage *)(leftExpander.consumerMessage);
        unoStrip.setStep=currentMessage->setStep;
        unoStrip.process(args.sampleTime);

        if (rightExpander.module) {
          if(rightExpander.module->model==modelUnoE) {
            auto messageToExpander=(UnoExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);
            memcpy(messageToExpander,currentMessage,sizeof(UnoExpanderMessage));
            rightExpander.module->leftExpander.messageFlipRequested = true;
          }
        }
      }
    }
	}
};


struct UnoEWidget : ModuleWidget {
	UnoEWidget(UnoE* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/UnoE.svg")));
    float x=5.08f;
    float y=TY(80);
    for(int k=0;k<8;k++) {
      addChild(createLightCentered<LargeLight<RedLight>>(mm2px(Vec(x,y+3)),module,k));
      y+=7;
    }
    x=1.9;
    addInput(createInput<SmallPort>(mm2px(Vec(x,TY(118.5))),module,UnoE::CLK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x,TY(111.5))),module,UnoE::RST_INPUT));

    auto selectParam=createParam<SelectParam>(mm2px(Vec(1.7,TY(107.25-3.5))),module,UnoE::DIR_PARAM);
    selectParam->box.size=mm2px(Vec(7,3.2*3));
    selectParam->init({"-->","<--","<->"});
    addParam(selectParam);

    addInput(createInput<SmallPort>(mm2px(Vec(x,TY(92))),module,UnoE::SEED_INPUT));

    y=TY(22);
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,UnoE::GATE_OUTPUT));
    y+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,UnoE::CV_OUTPUT));
    y+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,UnoE::STEP_OUTPUT));

	}
};


Model* modelUnoE = createModel<UnoE, UnoEWidget>("UnoE");