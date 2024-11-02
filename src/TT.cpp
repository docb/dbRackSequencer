#include "plugin.hpp"


struct TT : Module {
  enum ParamId {
    PARAMS_LEN
  };
  enum InputId {
    A_INPUT,B_INPUT,C_INPUT,D_INPUT,TF_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };

  bool gates[16]={};
  bool gateCountFromZero=true;
  int maxChannels=16;
  RND rnd;
  TT() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configInput(A_INPUT,"A");
    configInput(B_INPUT,"B");
    configInput(C_INPUT,"C");
    configInput(D_INPUT,"D");
    configInput(TF_INPUT,"TF");
    configOutput(CV_OUTPUT,"CV");
  }

  int getIndex(int chn) {
    return int(inputs[A_INPUT].getVoltage(chn)>1.f)
           |int(inputs[B_INPUT].getVoltage(chn)>1.f)<<1
           |int(inputs[C_INPUT].getVoltage(chn)>1.f)<<2
           |int(inputs[D_INPUT].getVoltage(chn)>1.f)<<3;
  }

  int getChannels() {
    return std::max(std::max(inputs[A_INPUT].getChannels(),inputs[B_INPUT].getChannels()),
                    std::max(inputs[C_INPUT].getChannels(),inputs[D_INPUT].getChannels()));
  }

  void process(const ProcessArgs &args) override {
    if(inputs[TF_INPUT].isConnected()) {
      for(int k=0;k<16;k++) {
        gates[k]=inputs[TF_INPUT].getVoltage(k)>1.f;
      }
    }
    int channels=getChannels();
    for(int chn=0;chn<channels;chn++) {
      int index=getIndex(chn);
      bool result=gates[index];
      outputs[CV_OUTPUT].setVoltage(result?10.f:0.f,chn);
    }
    outputs[CV_OUTPUT].setChannels(channels);
  }

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_t *onList=json_array();
    for(int j=0;j<PORT_MAX_CHANNELS;j++) {
      json_array_append_new(onList,json_boolean(gates[j]));
    }
    json_object_set_new(data,"gates",onList);
    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    json_t *data=json_object_get(rootJ,"gates");
    if(!data)
      return;

    if(data) {
      for(int j=0;j<16;j++) {
        json_t *on=json_array_get(data,j);
        gates[j]=json_boolean_value(on);
      }
    }
  }

  bool getGateStatus(int key) {
    return gates[key];
  }
  void updateKey(int key) {
    gates[key]=!(gates[key]);
  }

  void onRandomize(const RandomizeEvent &e) override {
    for(int k=0;k<16;k++) {
      gates[k]=rnd.nextCoin(0.5f);
    }
  }
};


struct TTWidget : ModuleWidget {
  TTWidget(TT *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/TT.svg")));
    float x=1.9f;
    float y=8.f;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,TT::A_INPUT));
    y+=10;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,TT::B_INPUT));
    y+=10;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,TT::C_INPUT));
    y+=10;
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,TT::D_INPUT));
    addChild(new GateDisplay<TT>(module,mm2px(Vec(1.7f,50))));

    addInput(createInput<SmallPort>(mm2px(Vec(x,105)),module,TT::TF_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,116)),module,TT::CV_OUTPUT));
  }
};


Model *modelTT=createModel<TT,TTWidget>("TT");