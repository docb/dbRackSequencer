#include "plugin.hpp"
#include "C42ExpanderMessage.hpp"
extern Model *modelC42;
struct C42E : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
    ROW_OUTPUT,COL_OUTPUT,ROW_PLUS_COL_OUTPUT,ROW_MINUS_COL_OUTPUT,COL_MINUS_ROW_OUTPUT,MD_OUTPUT,AD_OUTPUT,MD_PLUS_AD_OUTPUT,MD_MINUS_AD_OUTPUT,AD_MINUS_MD_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

  C42ExpanderMessage leftMessages[2][1];
  C42E() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configOutput(ROW_OUTPUT,"CV Row");
    configOutput(COL_OUTPUT,"CV Col");
    configOutput(ROW_PLUS_COL_OUTPUT,"CV Row+Col");
    configOutput(ROW_MINUS_COL_OUTPUT,"CV Row-Col");
    configOutput(COL_MINUS_ROW_OUTPUT,"CV Col-Row");
    configOutput(MD_OUTPUT,"CV Main Diagonal");
    configOutput(AD_OUTPUT,"CV Anti Diagonal");
    configOutput(MD_PLUS_AD_OUTPUT,"CV Main+Anti");
    configOutput(MD_MINUS_AD_OUTPUT,"CV Main-Anti");
    configOutput(AD_MINUS_MD_OUTPUT,"CV Anti-Main");
    leftExpander.producerMessage = leftMessages[0];
    leftExpander.consumerMessage = leftMessages[1];
  }

	void process(const ProcessArgs& args) override {
    if(leftExpander.module) {
      if(leftExpander.module->model==modelC42) {
        auto messagesFromMaster = (C42ExpanderMessage *)(leftExpander.consumerMessage);
        int channels=messagesFromMaster->channels;
        for(int chn=0;chn<channels;chn++) {
          float topLeft=messagesFromMaster->topLeft[chn];
          float colTop=messagesFromMaster->colTop[chn];
          float topRight=messagesFromMaster->topRight[chn];
          float rowLeft=messagesFromMaster->rowLeft[chn];
          float rowRight=messagesFromMaster->rowRight[chn];
          float bottomLeft=messagesFromMaster->bottomLeft[chn];
          float colBottom=messagesFromMaster->colBottom[chn];
          float bottomRight=messagesFromMaster->bottomRight[chn];
          outputs[ROW_OUTPUT].setVoltage(rowLeft+rowRight,chn);
          outputs[COL_OUTPUT].setVoltage(colTop+colBottom,chn);
          outputs[ROW_PLUS_COL_OUTPUT].setVoltage(rowLeft+rowRight+colTop+colBottom,chn);
          outputs[ROW_MINUS_COL_OUTPUT].setVoltage(rowLeft+rowRight-colTop-colBottom,chn);
          outputs[COL_MINUS_ROW_OUTPUT].setVoltage(colTop+colBottom-rowLeft-rowRight,chn);
          outputs[MD_OUTPUT].setVoltage(topLeft+bottomRight,chn);
          outputs[AD_OUTPUT].setVoltage(bottomLeft+topRight,chn);
          outputs[MD_PLUS_AD_OUTPUT].setVoltage(topLeft+bottomRight+bottomLeft+topRight,chn);
          outputs[MD_MINUS_AD_OUTPUT].setVoltage(topLeft+bottomRight-bottomLeft-topRight,chn);
          outputs[AD_MINUS_MD_OUTPUT].setVoltage(bottomLeft+topRight-topLeft-bottomRight,chn);
        }
        outputs[ROW_OUTPUT].setChannels(channels);
        outputs[COL_OUTPUT].setChannels(channels);
        outputs[ROW_PLUS_COL_OUTPUT].setChannels(channels);
        outputs[ROW_MINUS_COL_OUTPUT].setChannels(channels);
        outputs[COL_MINUS_ROW_OUTPUT].setChannels(channels);
        outputs[MD_OUTPUT].setChannels(channels);
        outputs[AD_OUTPUT].setChannels(channels);
        outputs[MD_PLUS_AD_OUTPUT].setChannels(channels);
        outputs[MD_MINUS_AD_OUTPUT].setChannels(channels);
        outputs[AD_MINUS_MD_OUTPUT].setChannels(channels);
      }
    }
	}
};


struct C42EWidget : ModuleWidget {
	C42EWidget(C42E* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/C42E.svg")));

    addChild(createWidget<ScrewSilver>(Vec(0,0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-15,0)));
    addChild(createWidget<ScrewSilver>(Vec(0,365)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x-15,365)));

    float y=TY(108);
    float x=11;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42E::ROW_OUTPUT));
    y+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42E::COL_OUTPUT));
    y+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42E::ROW_PLUS_COL_OUTPUT));
    y+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42E::ROW_MINUS_COL_OUTPUT));
    y+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42E::COL_MINUS_ROW_OUTPUT));
    y=TY(52);
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42E::MD_OUTPUT));
    y+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42E::AD_OUTPUT));
    y+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42E::MD_PLUS_AD_OUTPUT));
    y+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42E::MD_MINUS_AD_OUTPUT));
    y+=8;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,y)),module,C42E::AD_MINUS_MD_OUTPUT));
	}
};


Model* modelC42E = createModel<C42E, C42EWidget>("C42E");