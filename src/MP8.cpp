#include <utility>
#include "plugin.hpp"

struct Wave {
  std::vector<uint8_t> samples={};

  explicit Wave(json_t *jSamples) {
    size_t pLen=json_array_size(jSamples);
    for(unsigned int k=0;k<pLen;k++) {
      samples.push_back(json_integer_value(json_array_get(jSamples,k)));
    }
  }
};

struct Bank {
  std::string name;
  std::vector<Wave> waves={};

  Bank(std::string _name,json_t *jWaves) : name(std::move(_name)) {
    size_t pLen=json_array_size(jWaves);
    for(unsigned int k=0;k<pLen;k++) {
      waves.emplace_back(json_array_get(jWaves,k));
    }
  }
};

struct ROM {
  std::vector<Bank> banks;

  void init(json_t *jRom) {
    const char *key;
    json_t *value;
    json_object_foreach(jRom,key,value) {
      banks.emplace_back(key,value);
    }
  }

  uint8_t getSample(unsigned bank,unsigned wave,uint8_t pos) {
    return banks[bank].waves[wave].samples[pos];
  }
};

struct MP8 : Module {
  enum ParamId {
    BANK_PARAM,WAVE_PARAM,PHASE_PARAM,PARAMS_LEN
  };
  enum InputId {
    POLY_INPUT=8,BANK_INPUT,WAVE_INPUT,PHASE_INPUT,INPUTS_LEN
  };
  enum OutputId {
    POLY_OUTPUT=8,OUTPUTS_LEN
  };
  enum LightId {
    OUT_LIGHTS=8,LIGHTS_LEN=16
  };
  ROM rom;

  std::string getCurrentLabel() {
    return rom.banks[int(params[BANK_PARAM].getValue())].name;
  }


  bool load(const std::string &path) {
    INFO("Loading rom %s",path.c_str());
    FILE *file=fopen(path.c_str(),"r");
    if(!file)
      return false;
    DEFER({
            fclose(file);
          });

    json_error_t error;
    json_t *rootJ=json_loadf(file,0,&error);
    if(!rootJ) {
      WARN("%s",
           string::f("ROM file has invalid JSON at %d:%d %s",error.line,error.column,error.text).c_str());
      return false;
    }
    try {
      rom.init(rootJ);
    } catch(Exception &e) {
      WARN("%s",e.msg.c_str());
      return false;
    }
    json_decref(rootJ);
    return true;
  }

  MP8() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    if(!load(asset::plugin(pluginInstance,"res/rom.json"))) {
      INFO("user rom file %s does not exist or failed to load. using default_rom.json ....","res/rom.json");
      if(!load(asset::plugin(pluginInstance,"res/default_rom.json"))) {
         throw Exception(string::f("Default rom config is damaged, try reinstalling the plugin"));
      }
    }
    configParam(BANK_PARAM,0,float(rom.banks.size()-1),0,"Bank");
    getParamQuantity(BANK_PARAM)->snapEnabled=true;
    configInput(BANK_INPUT,"Bank");
    configParam(WAVE_PARAM,0,1,0,"Wave"," %",0,100);
    configInput(WAVE_INPUT,"Wave");
    configParam(PHASE_PARAM,0,1,0,"Phase Add");
    configInput(PHASE_INPUT,"Phase Add");
    for(int k=0;k<8;k++) {
      std::string label=std::to_string(k+1);
      configInput(k,"IN"+label);
      configOutput(k,"OUT"+label);
    }
    configInput(POLY_INPUT,"Poly");
    configOutput(POLY_OUTPUT,"Poly");

  }

  uint8_t getCurrentSample(uint8_t pos) {
    int bank=int(params[BANK_PARAM].getValue());
    unsigned numWaves=rom.banks[bank].waves.size();
    auto phase=uint8_t(params[PHASE_PARAM].getValue()*256);
    float f=params[WAVE_PARAM].getValue()*float(numWaves-1);
    auto wave0=unsigned(f);
    float frac=f-float(wave0);
    uint8_t out=rom.getSample(bank,wave0,pos+phase);
    if(wave0<numWaves-1) {
      uint8_t out1=rom.getSample(bank,wave0+1,pos+phase);
      out=out1>out?out+(out1-out)*frac:out-(out-out1)*frac;
    }
    return out;
  }

  void process(const ProcessArgs &args) override {
    if(inputs[PHASE_INPUT].isConnected()) {
      getParamQuantity(PHASE_PARAM)->setImmediateValue(inputs[PHASE_INPUT].getVoltage()*0.1f);
    }
    if(inputs[WAVE_INPUT].isConnected()) {
      getParamQuantity(WAVE_PARAM)->setImmediateValue(inputs[WAVE_INPUT].getVoltage()*0.1f);
    }
    if(inputs[BANK_INPUT].isConnected()) {
      getParamQuantity(BANK_PARAM)->setImmediateValue(
        inputs[BANK_INPUT].getVoltage()*0.1f*float(rom.banks.size()));
    }
    uint8_t in=0;
    if(inputs[POLY_INPUT].isConnected()) {
      uint8_t s=0;
      for(int k=0;k<8;k++) {
        if(inputs[POLY_INPUT].getVoltage(k)>1.f) {
          s|=(1<<k);
        }
      }
      in=s;
    }
    for(int k=0;k<8;k++) {
      if(inputs[k].isConnected()) {
        if(inputs[k].getVoltage()>1.f) {
          in|=(1<<k);
        } else {
          in&=~(1<<k);
        }
      }
    }
    uint8_t out=getCurrentSample(in);
    for(int k=0;k<8;k++) {
      bool b=(out&(1<<k))!=0;
      outputs[k].setVoltage(b?10.f:0.f);
      outputs[POLY_OUTPUT].setVoltage(b?10.f:0.f,k);
      lights[OUT_LIGHTS+k].setBrightness(b?1.f:0.f);
      b=(in&(1<<k))!=0;
      lights[k].setBrightness(b?1.f:0.f);
    }
    outputs[POLY_OUTPUT].setChannels(8);
  }
};

template<class M>
struct WDisplay : LedDisplay {
  M *module=nullptr;

  void drawLayer(const DrawArgs &args,int layer) override {
    if(!module) return;
    nvgScissor(args.vg,RECT_ARGS(args.clipBox));

    if(layer==1) {
      std::string fontPath=asset::system("res/fonts/ShareTechMono-Regular.ttf");
      std::shared_ptr<Font> font=APP->window->loadFont(fontPath);
      if(!font)
        return;
      nvgFontSize(args.vg,13);
      nvgFontFaceId(args.vg,font->handle);
      nvgFillColor(args.vg,SCHEME_GREEN);
      nvgText(args.vg,4.0,13.0,module->getCurrentLabel().c_str(),nullptr);

      nvgScissor(args.vg,RECT_ARGS(args.clipBox));
      nvgBeginPath(args.vg);
      Vec scopePos=Vec(0.0,13.0);
      Rect scopeRect=Rect(scopePos,box.size-scopePos);
      scopeRect=scopeRect.shrink(Vec(4,5));
      size_t iSkip=3;

      for(size_t i=0;i<=256;i+=iSkip) {
        uint8_t sample=module->getCurrentSample(i);
        Vec p;
        p.x=float(i)/256;
        p.y=float(256-sample)/256;
        p=scopeRect.pos+scopeRect.size*p;
        if(i==0)
          nvgMoveTo(args.vg,VEC_ARGS(p));
        else
          nvgLineTo(args.vg,VEC_ARGS(p));
      }
      nvgLineCap(args.vg,NVG_ROUND);
      nvgMiterLimit(args.vg,2.f);
      nvgStrokeWidth(args.vg,1.5f);
      nvgStrokeColor(args.vg,SCHEME_GREEN);
      nvgStroke(args.vg);
    }
    nvgResetScissor(args.vg);
  }
};

template<typename M>
struct BankSelect : SpinParamWidget {
  M *module=nullptr;

  BankSelect() {
    init();
  }

  void onChange(const event::Change &e) override {
  }
};

struct MP8Widget : ModuleWidget {
  explicit MP8Widget(MP8 *module) {
    setModule(module);
    float x1=4;
    float x2=14.56;
    float x3=26;
    setPanel(createPanel(asset::plugin(pluginInstance,"res/MP8.svg")));
    auto *display=createWidget<WDisplay<MP8>>(mm2px(Vec(0.504,4.0)));
    display->box.size=mm2px(Vec(34.06,29.224));
    display->module=module;
    addChild(display);
    float y=40;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x1,y)),module,MP8::WAVE_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(11,y)),module,MP8::WAVE_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(20,y)),module,MP8::PHASE_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(27,y)),module,MP8::PHASE_INPUT));
    auto bankParam=createParam<BankSelect<MP8>>(mm2px(Vec(x2,55)),module,MP8::BANK_PARAM);
    bankParam->module=module;
    addParam(bankParam);
    addInput(createInput<SmallPort>(mm2px(Vec(15,67)),module,MP8::BANK_INPUT));

    y=55;
    for(int k=0;k<8;k++) {
      addInput(createInput<SmallPort>(mm2px(Vec(x1,y)),module,k));
      addChild(createLight<TinySimpleLight<GreenLight>>(mm2px(Vec(x1+6.5f,y+4.5f)),module,k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(x3,y)),module,k));
      addChild(createLight<TinySimpleLight<GreenLight>>(mm2px(Vec(x3+6.5f,y+4.5f)),module,
                                                        MP8::OUT_LIGHTS+k));
      y+=7;
    }
    addOutput(createOutput<SmallPort>(mm2px(Vec(x3,116)),module,MP8::POLY_OUTPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x1,116)),module,MP8::POLY_INPUT));
  }
};


Model *modelMP8=createModel<MP8,MP8Widget>("MP8");