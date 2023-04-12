#include "plugin.hpp"

#define NUM_SEQ 8
#define MAX_VERTICES 32

struct Seq {
  dsp::PulseGenerator pulseGenerator;
  int nr;
  int numVertices=2;
  float rotation=0;
  float skewFactor=0;
  bool enabled=false;
  bool gate=false;
  float sequence[MAX_VERTICES]={};

  Seq(int _nr) : nr(_nr) {
    calcSeq();
  }

  void set(int v,float r,float s) {
    if(r!=rotation||v!=numVertices||s!=skewFactor) {
      rotation=r;
      numVertices=v;
      skewFactor=s;
      calcSeq();
    }
  }

  void calcSeq() {
    float degrees=skewFactor+360.f/float(numVertices);
    float angle=rotation;
    for(int k=0;k<numVertices;k++) {
      sequence[k]=fmodf(angle,360.f);
      angle+=degrees;
    }
  }

  void next(float counter,float lastCounter) {
    if(enabled) {
      bool triggered=false;
      for(int k=0;k<numVertices;k++) {
        if(counter<lastCounter) { // wrapped around
          if(sequence[k]<=counter||sequence[k]>lastCounter) {
            pulseGenerator.trigger(0.1f);
            triggered=true;
          }
        }
        if(lastCounter<sequence[k]&&sequence[k]<=counter) {
          triggered=true;
          pulseGenerator.trigger(0.1f);
        }
      }
      gate=triggered;
    } else {
      gate=false;
    }
  }

  void nextExactHit(float counter) {
    if(enabled) {
      gate=false;
      for(int k=0;k<numVertices;k++) {
        if(sequence[k]==counter) {
          pulseGenerator.trigger(0.1f);
          gate=true;
        }
      }
    }
  }

  bool processPulse(float dt) {
    return pulseGenerator.process(dt);
  }
};

struct N3 : Module {
  enum ParamId {
    ON_PARAM,VERTICES_PARAM=ON_PARAM+NUM_SEQ,ROT_PARAM=VERTICES_PARAM+NUM_SEQ,SKEW_PARAM=ROT_PARAM+NUM_SEQ,RST_PARAM=SKEW_PARAM+NUM_SEQ,DEGREE_PARAM,EXACT_PARAM,PARAMS_LEN
  };
  enum InputId {
    ON_INPUT,VERTICES_INPUT=ON_INPUT+NUM_SEQ,ROT_INPUT=VERTICES_INPUT+NUM_SEQ,SKEW_INPUT=ROT_INPUT+NUM_SEQ,RST_INPUT=SKEW_INPUT+NUM_SEQ,CLOCK_INPUT,DEGREE_INPUT,INPUTS_LEN
  };
  enum OutputId {
    TRIG_OUTPUT,CLK_OUTPUT=TRIG_OUTPUT+NUM_SEQ,GATE_OUTPUT=CLK_OUTPUT+NUM_SEQ,POLY_TRIG_OUTPUT=GATE_OUTPUT+NUM_SEQ,POLY_CLK_OUTPUT,POLY_GATE_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN=NUM_SEQ
  };

  Seq seqs[NUM_SEQ]={{0},
                     {1},
                     {2},
                     {3},
                     {4},
                     {5},
                     {6},
                     {7}};
  dsp::SchmittTrigger rstTrigger;
  dsp::SchmittTrigger rstManualTrigger;
  dsp::SchmittTrigger clockTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::ClockDivider paramDivider;
  float state=0;
  float lastState=0;

  N3() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    for(int k=0;k<NUM_SEQ;k++) {
      configParam(ROT_PARAM+k,0,359,0,"Rotation");
      configParam(SKEW_PARAM+k,0,359,0,"Skew Factor");
      configParam(VERTICES_PARAM+k,1,MAX_VERTICES,2,"Vertices");
      getParamQuantity(VERTICES_PARAM+k)->snapEnabled=true;
      configButton(ON_PARAM+k,"On");
      configInput(ROT_INPUT+k,"Rotation");
      configInput(SKEW_INPUT+k,"SkewFactor");
      configInput(VERTICES_INPUT+k,"Vertices");
      configInput(ON_INPUT+k,"On");
      configOutput(TRIG_OUTPUT+k,"Trigger");
      configOutput(CLK_OUTPUT+k,"Clock");
      configOutput(GATE_OUTPUT+k,"Gate");
    }
    configButton(RST_PARAM,"Reset");
    configButton(EXACT_PARAM,"Exact");
    configParam(DEGREE_PARAM,0,90,1,"Degrees");
    configInput(CLOCK_INPUT,"Clock");
    configInput(DEGREE_INPUT,"Degree");
    configInput(RST_INPUT,"Rst");

    paramDivider.setDivision(32);
  }

  void process(const ProcessArgs &args) override {
    bool advance=false;
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())|rstManualTrigger.process(params[RST_PARAM].getValue())) {
      state=0;
      lastState=-0.001;
      rstPulse.trigger(0.001f);
      advance=true;
    }
    bool rstGate=rstPulse.process(args.sampleTime);
    if(paramDivider.process()) {
      if(inputs[DEGREE_INPUT].isConnected()) {
        setImmediateValue(getParamQuantity(DEGREE_PARAM),inputs[DEGREE_INPUT].getVoltage()*9);
      }
      for(int k=0;k<NUM_SEQ;k++) {
        if(inputs[ON_INPUT+k].isConnected()) {
          setImmediateValue(getParamQuantity(ON_PARAM+k),inputs[ON_INPUT+k].getVoltage()>0.f?1.f:0.f);
        }
        if(inputs[VERTICES_INPUT+k].isConnected()) {
         setImmediateValue( getParamQuantity(VERTICES_PARAM+k),inputs[VERTICES_INPUT+k].getVoltage()*3.2f);
        }
        if(inputs[SKEW_INPUT+k].isConnected()) {
          setImmediateValue(getParamQuantity(SKEW_PARAM+k),inputs[SKEW_INPUT+k].getVoltage()*36);
        }
        if(inputs[ROT_INPUT+k].isConnected()) {
          setImmediateValue(getParamQuantity(ROT_PARAM+k),inputs[ROT_INPUT+k].getVoltage()*36);
        }
        seqs[k].set(params[VERTICES_PARAM+k].getValue(),params[ROT_PARAM+k].getValue(),params[SKEW_PARAM+k].getValue());
        seqs[k].enabled=params[ON_PARAM+k].getValue()>0.f;
      }
    }
    if(clockTrigger.process(inputs[CLOCK_INPUT].getVoltage())&&!rstGate) {
      advance=true;
      lastState=state;
      float degrees=params[DEGREE_PARAM].getValue();
      state=fmodf(state+degrees,360.f);
    }
    if(advance) {
      bool exactHit=params[EXACT_PARAM].getValue()>0.f;
      for(int k=0;k<NUM_SEQ;k++) {
        if(exactHit) {
          seqs[k].nextExactHit(state);
        } else {
          seqs[k].next(state,lastState);
        }
      }
    }
    int maxOn=0;
    for(int k=0;k<NUM_SEQ;k++) {
      if(seqs[k].enabled)
        maxOn=k+1;
      if(seqs[k].processPulse(args.sampleTime)) {
        outputs[TRIG_OUTPUT+k].setVoltage(10.f);
        outputs[POLY_TRIG_OUTPUT].setVoltage(10.f,k);
        lights[k].setSmoothBrightness(1.f,args.sampleTime);
      } else {
        outputs[TRIG_OUTPUT+k].setVoltage(0.f);
        outputs[POLY_TRIG_OUTPUT].setVoltage(0.f,k);
        lights[k].setSmoothBrightness(0.f,args.sampleTime);
      }
      outputs[GATE_OUTPUT+k].setVoltage(seqs[k].gate?10.f:0.f);
      outputs[POLY_GATE_OUTPUT].setVoltage(seqs[k].gate?10.f:0.f,k);
      outputs[CLK_OUTPUT+k].setVoltage(seqs[k].gate?inputs[CLOCK_INPUT].getVoltage():0.f);
      outputs[POLY_CLK_OUTPUT].setVoltage(seqs[k].gate?inputs[CLOCK_INPUT].getVoltage():0.f,k);
    }
    outputs[POLY_TRIG_OUTPUT].setChannels(maxOn);
    outputs[POLY_GATE_OUTPUT].setChannels(maxOn);
    outputs[POLY_CLK_OUTPUT].setChannels(maxOn);
  }
};

struct N3Display : OpaqueWidget {
  N3 *module=nullptr;
  NVGcolor seqColors[8]={nvgRGB(255,0,0),nvgRGB(0,255,0),nvgRGB(0,0,255),nvgRGB(255,255,0),nvgRGB(255,0,255),nvgRGB(0,255,255),nvgRGB(160,160,120),nvgRGB(255,128,0)};
  NVGcolor pointerColor=nvgRGB(255,255,255);

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  N3Display(N3 *_module,Vec pos,Vec size) : OpaqueWidget(),module(_module) {
    box.pos=pos.plus(Vec(0,1));
    box.size=size;
  }

  void _draw(const DrawArgs &args) {
    if(module==nullptr)
      return;
    float mx=box.size.x/2.f;
    float ofs=2;

    float angle,sx,sy;
    for(int k=0;k<NUM_SEQ;k++) {
      Seq *seq=&module->seqs[k];
      if(seq->enabled) {
        int numVertices=seq->numVertices;
        nvgBeginPath(args.vg);
        if(numVertices==1) {
          nvgMoveTo(args.vg,mx,mx);
          angle=(seq->sequence[0]-90.f)*float(M_PI)/180.f;
          sx=cosf(angle)*(mx-ofs);
          sy=sinf(angle)*(mx-ofs);
          nvgLineTo(args.vg,sx+mx,sy+mx);
        } else {
          for(int k=0;k<numVertices;k++) {
            angle=(seq->sequence[k]-90.f)*float(M_PI)/180.f;
            sx=cosf(angle)*(mx-3);
            sy=sinf(angle)*(mx-3);
            if(k==0) {
              nvgMoveTo(args.vg,sx+mx,sy+mx);
            } else {
              nvgLineTo(args.vg,sx+mx,sy+mx);
            }
            nvgClosePath(args.vg);
          }
        }
        if(module->outputs[N3::TRIG_OUTPUT+k].getVoltage()>0.f) {
          nvgStrokeWidth(args.vg,2.6f);
        } else {
          nvgStrokeWidth(args.vg,1.f);
        }
        nvgStrokeColor(args.vg,seqColors[seq->nr]);
        nvgStroke(args.vg);
      }
    }
    angle=(module->state-90.f)*float(M_PI)/180.f;
    sx=cosf(angle)*(mx-ofs);
    sy=sinf(angle)*(mx-ofs);

    nvgBeginPath(args.vg);
    nvgMoveTo(args.vg,mx,mx);
    nvgLineTo(args.vg,sx+mx,sy+mx);
    nvgStrokeColor(args.vg,pointerColor);
    nvgStrokeWidth(args.vg,2.f);
    nvgStroke(args.vg);
  }
};

struct N3Widget : ModuleWidget {
  N3Widget(N3 *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/N3.svg")));
    float x=68;
    for(int k=0;k<NUM_SEQ;k++) {
      addParam(createParam<MLED>(mm2px(Vec(x,TY(115))),module,N3::ON_PARAM+k));
      addInput(createInput<SmallPort>(mm2px(Vec(x,TY(107))),module,N3::ON_INPUT+k));
      addParam(createParam<TrimbotWhite>(mm2px(Vec(x,TY(95))),module,N3::VERTICES_PARAM+k));
      addInput(createInput<SmallPort>(mm2px(Vec(x,TY(87))),module,N3::VERTICES_INPUT+k));
      addParam(createParam<TrimbotWhite>(mm2px(Vec(x,TY(75))),module,N3::ROT_PARAM+k));
      addInput(createInput<SmallPort>(mm2px(Vec(x,TY(67))),module,N3::ROT_INPUT+k));
      addParam(createParam<TrimbotWhite>(mm2px(Vec(x,TY(55))),module,N3::SKEW_PARAM+k));
      addInput(createInput<SmallPort>(mm2px(Vec(x,TY(47))),module,N3::SKEW_INPUT+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(x,TY(31))),module,N3::GATE_OUTPUT+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(x,TY(19))),module,N3::CLK_OUTPUT+k));
      addOutput(createOutput<SmallPort>(mm2px(Vec(x,TY(7))),module,N3::TRIG_OUTPUT+k));
      addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(x+8,TY(6))),module,k));
      x+=12;
    }
    x=56;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,TY(31))),module,N3::POLY_GATE_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,TY(19))),module,N3::POLY_CLK_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x,TY(7))),module,N3::POLY_TRIG_OUTPUT));

    addInput(createInput<SmallPort>(mm2px(Vec(6,TY(35))),module,N3::CLOCK_INPUT));
    addParam(createParam<MLEDM>(mm2px(Vec(17,TY(35))),module,N3::RST_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(26,TY(35))),module,N3::RST_INPUT));
    addParam(createParam<MLED>(mm2px(Vec(39,TY(35))),module,N3::EXACT_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(26,TY(24))),module,N3::DEGREE_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(35,TY(24))),module,N3::DEGREE_INPUT));

    addChild(new N3Display(module,mm2px(Vec(4,4)),mm2px(Vec(60,60))));
  }
};


Model *modelN3=createModel<N3,N3Widget>("N3");