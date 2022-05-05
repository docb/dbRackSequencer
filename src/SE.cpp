#include "plugin.hpp"
#include "SEMessage.hpp"

extern Model *modelSum;
extern Model *modelSE;
struct SE : Module {
  enum ParamId {
    PARAMS_LEN=NUM_INPUTS
  };
  enum InputId {
    INPUTS_LEN
  };
  enum OutputId {
    SUM_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  SEMessage rightMessages[2][1];
  SEMessage leftMessages[2][1];

  SE() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    for(int k=0;k<NUM_INPUTS;k++) {
      configParam(k,0,2,1,"Sign "+std::to_string(k+1));
      getParamQuantity(k)->snapEnabled=true;
    }
    configOutput(SUM_OUTPUT,"Sum");
    rightExpander.producerMessage=rightMessages[0];
    rightExpander.consumerMessage=rightMessages[1];
    leftExpander.producerMessage=leftMessages[0];
    leftExpander.consumerMessage=leftMessages[1];
  }

  void processChannel(SEMessage *message,int chn) {
    float sum=0;
    for(int k=0;k<NUM_INPUTS;k++) {
      sum+=message->ins[k][chn]*(params[k].getValue()-1);
    }
    outputs[SUM_OUTPUT].setVoltage(sum,chn);
  }

  void process(const ProcessArgs &args) override {
    if(leftExpander.module) {
      if(leftExpander.module->model==modelSE||leftExpander.module->model==modelSum) {
        auto messageFromLeft=(SEMessage *)(leftExpander.consumerMessage);
        int channels=messageFromLeft->channels;
        for(int chn=0;chn<channels;chn++) {
          processChannel(messageFromLeft,chn);
        }
        outputs[SUM_OUTPUT].setChannels(channels);
        if (rightExpander.module) {
          if(rightExpander.module->model==modelSE) {
            SEMessage *messageToExpander=(SEMessage*)(rightExpander.module->leftExpander.producerMessage);
            memcpy(messageToExpander,messageFromLeft,sizeof(SEMessage));
            rightExpander.module->leftExpander.messageFlipRequested = true;
          }
        }
      }
    }
  }
};


struct SEWidget : ModuleWidget {
  SEWidget(SE *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/SE.svg")));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
    float x=1.2;
    float y=TY(109);
    for(int k=0;k<NUM_INPUTS;k++) {
      auto selectParam=createParam<SelectParamH>(mm2px(Vec(x,y+0.75)),module,k);
      selectParam->box.size=mm2px(Vec(7.5,5));
      selectParam->init({" "," "," "});
      addParam(selectParam);
      y+=8;
    }
    addOutput(createOutput<SmallPort>(mm2px(Vec(1.9,TY(9))),module,SE::SUM_OUTPUT));
  }
};


Model *modelSE=createModel<SE,SEWidget>("SE");