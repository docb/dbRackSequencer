#include "plugin.hpp"

struct MCPoint {
  MCPoint(unsigned long _pos,float _y) : pos(_pos),y(_y) {
  }

  MCPoint(json_t *json) {
    int k=0;
    pos=json_integer_value(json_array_get(json,k++));
    y=json_real_value(json_array_get(json,k++));

  }

  unsigned long pos=0;
  float y=0;

  json_t *toJson() {
    json_t *jList=json_array();
    json_array_append_new(jList,json_integer(pos));
    json_array_append_new(jList,json_real(y));
    return jList;
  }

};

template <typename T>
struct MSlewLimiter {
  T out = 0.f;
  T rise = 2.f;
  T fall = 2.f;

  void reset(float v=0.f) {
    out = v;
  }

  T process(T deltaTime, T in) {
    out = simd::clamp(in, out - fall * deltaTime, out + rise * deltaTime);
    return out;
  }
};

struct MC1 : Module {
  enum ParamId {
    TRG_PARAM,Y_PARAM,PARAMS_LEN
  };
  enum InputId {
    TRG_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,GATE_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  float st;
  bool gate=false;
  bool replay=false;
  bool rec=false;
  float y=1;

  float getY() {
    return slew.process(st,y);
  }

  float oldY=1;
  unsigned long recPos=0;
  unsigned long playPos=0;
  unsigned long pointPos=0;

  std::vector<MCPoint> points={};
  dsp::SchmittTrigger manualPlayTrigger;
  dsp::SchmittTrigger playTrigger;
  MSlewLimiter<float> slew;

	MC1() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configOutput(CV_OUTPUT,"CV");
    configOutput(GATE_OUTPUT,"Gate");
    configInput(TRG_INPUT,"Replay");
    configButton(TRG_PARAM,"Replay");
    configParam(Y_PARAM,0,1,0,"Y");
    st=APP->engine->getSampleTime();
  }

  void onSampleRateChange() override {
    st=APP->engine->getSampleTime();
  }

  void process(const ProcessArgs &args) override {

    if((playTrigger.process(inputs[TRG_INPUT].getVoltage())&& !rec)|manualPlayTrigger.process(params[TRG_PARAM].getValue()) ) {
      replay=true;
      playPos=0;
      pointPos=0;
      gate=true;
    }
    if(!replay) {
      if(oldY!=y) {
        if(gate) {
          points.emplace_back(recPos,y);
        }
        oldY=y;
      }
    } else {
      if(playPos==points[pointPos].pos) {

        if(pointPos==points.size()-1) {
          replay=false;
          gate=false;
        }
        y=points[pointPos].y;
        if(playPos==0) slew.reset(y);
        pointPos++;
      }
    }
    outputs[CV_OUTPUT].setVoltage((1.f-getY())*10.f);
    outputs[GATE_OUTPUT].setVoltage(gate?10.f:0.f);
    recPos++;
    playPos++;
  }

  void startMouse(float yy) {
    gate=true;
    replay=false;
    recPos=0;
    rec=true;
    points.clear();
    slew.reset(yy);
    y=yy;
  }

  void stopMouse() {
    points.emplace_back(recPos,y);
    gate=false;
    rec=false;
  }

  json_t *dataToJson() override {
    json_t *data=json_object();
    json_t *jPoints=json_array();
    for(auto p:points) {
      json_array_append_new(jPoints,p.toJson());
    }
    json_object_set_new(data,"points",jPoints);
    json_object_set_new(data,"y",json_real(y));
    return data;
  }

  void dataFromJson(json_t *rootJ) override {
    points.clear();
    json_t *jPoints=json_object_get(rootJ,"points");
    int len=json_array_size(jPoints);
    for(int i=0;i<len;i++) {
      points.emplace_back(json_array_get(jPoints,i));
    }
    json_t *jY=json_object_get(rootJ,"y");
    if(jY) {
      y=json_real_value(jY);
    }
  }

};

struct MCDisplay : OpaqueWidget {
  MC1 *module=nullptr;
  float oldY=0;
  Vec dragPosition={};

  MCDisplay(MC1 *_module,Vec pos,Vec size) : module(_module) {
    box.size=size;
    box.pos=pos;
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  void _draw(const DrawArgs &args) {
    if(module==nullptr)
      return;

    NVGcolor color=nvgRGBA(0,255,0,140);
    nvgBeginPath(args.vg);
    nvgEllipse(args.vg,box.size.x/2,module->y*box.size.y,box.size.x/2,7);
    nvgFillColor(args.vg,color);
    nvgFill(args.vg);
    nvgBeginPath(args.vg);
    nvgMoveTo(args.vg,0.f,module->y*box.size.y);
    nvgLineTo(args.vg,box.size.x,module->y*box.size.y);
    nvgStrokeColor(args.vg,nvgRGBA(255,255,255,140));
    nvgStroke(args.vg);
  }

  virtual void onButton(const event::Button &e) override {
    if(!module)
      return;

    if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_LEFT) {
      float y=e.pos.y/box.size.y;
      e.consume(this);
      dragPosition=e.pos;
      module->startMouse(y);
    }
  }

  void onDragMove(const event::DragMove &e) override {
    if(!module)
      return;
    dragPosition=dragPosition.plus(e.mouseDelta.div(getAbsoluteZoom()));
    float y=dragPosition.y/box.size.y;
    if(y<0)
      y=0;
    if(y>=1)
      y=1;
    if(y!=oldY) {
      if(e.button==GLFW_MOUSE_BUTTON_LEFT) {
        module->y=y;
      }
      oldY=y;
    }
  }

  void onDragEnd(const event::DragEnd &e) override {
    module->stopMouse();
  }

  bool isMouseInDrawArea(Vec position) {
    if(position.x<0)
      return (false);
    if(position.y<0)
      return (false);
    if(position.x>=box.size.x)
      return (false);
    if(position.y>=box.size.y)
      return (false);
    return (true);
  }

};


struct MC1Widget : ModuleWidget {
	MC1Widget(MC1* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/MC1.svg")));

    auto display=new MCDisplay(module,mm2px(Vec(3,7)),mm2px(Vec(14,89.3)));
    addChild(display);
    addInput(createInput<SmallPort>(mm2px(Vec(2,104)),module,MC1::TRG_INPUT));
    addParam(createParam<MLEDM>(mm2px(Vec(12,104)),module,MC1::TRG_PARAM));

    addOutput(createOutput<SmallPort>(mm2px(Vec(12,116)),module,MC1::CV_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(2,116)),module,MC1::GATE_OUTPUT));
	}
};


Model* modelMC1 = createModel<MC1, MC1Widget>("MC1");