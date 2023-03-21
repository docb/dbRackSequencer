#include "plugin.hpp"

struct M16S : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN=32
	};
	enum OutputId {
		L_OUTPUT,R_OUTPUT,OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
  bool mergePoly=true;

	M16S() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    for(int k=0;k<16;k++) {
      std::string label=std::to_string(k+1);
      configInput(k*2,"L/Mon Chn "+label);
      configInput(k*2+1,"Right Chn "+label);
    }
    configOutput(L_OUTPUT,"Left Poly");
    configOutput(R_OUTPUT,"Right Poly");
	}

	void process(const ProcessArgs& args) override {
    int channels=0;
    for(int k=0;k<16;k++) {
      if(inputs[k*2].isConnected()) {
        channels=k+1;
        if(inputs[k*2].getChannels()>1 && mergePoly) {
          outputs[L_OUTPUT].setVoltage(inputs[k*2].getVoltageSum(),k);
        } else {
          outputs[L_OUTPUT].setVoltage(inputs[k*2].getVoltage(),k);
        }
        if(inputs[k*2+1].isConnected()) {
          if(inputs[k*2+1].getChannels()>0 && mergePoly) {
            outputs[R_OUTPUT].setVoltage(inputs[k*2+1].getVoltageSum(),k);
          } else {
            outputs[R_OUTPUT].setVoltage(inputs[k*2+1].getVoltage(),k);
          }
        } else {
          outputs[R_OUTPUT].setVoltage(inputs[k*2].getVoltage(),k);
        }
      } else {
        outputs[L_OUTPUT].setVoltage(0.f,k);
        outputs[R_OUTPUT].setVoltage(0.f,k);
      }
    }
    outputs[L_OUTPUT].setChannels(channels);
    outputs[R_OUTPUT].setChannels(channels);
	}

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_object_set_new(data,"mergePoly",json_boolean(mergePoly));
    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    json_t *jMergePoly = json_object_get(rootJ,"mergePoly");
    if(jMergePoly!=nullptr) mergePoly = json_boolean_value(jMergePoly);
  }

};


struct M16SWidget : ModuleWidget {
	M16SWidget(M16S* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/M16S.svg")));

    float x1=2;
    float x2=12;
    float y=4;
    for(int k=0;k<16;k++) {
      addInput(createInput<SmallPort>(mm2px(Vec(x1,y)),module,k*2));
      addInput(createInput<SmallPort>(mm2px(Vec(x2,y)),module,k*2+1));
      y+=7;
    }
    y=118.5;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x1,y)),module,M16S::L_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x2,y)),module,M16S::R_OUTPUT));
	}

  void appendContextMenu(Menu* menu) override {
    M16S *module=dynamic_cast<M16S *>(this->module);
    assert(module);

    menu->addChild(new MenuSeparator);

    menu->addChild(createBoolPtrMenuItem("Merge Poly Cables","",&module->mergePoly));
  }
};


Model* modelM16S = createModel<M16S, M16SWidget>("M16S");