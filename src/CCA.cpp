#include "plugin.hpp"

#define CCASIZE 32
#define LENGTH 32

struct CCAMatrix {
  float grid[CCASIZE][LENGTH]={};

  void cfunc(float *init,const std::function<float( float )> &f) {
    float state[CCASIZE]={};
    for(int j=0;j<CCASIZE;j++) {
      state[j]=init[j];
    }
    for(int k=0;k<LENGTH;k++) {
      for(int j=0;j<CCASIZE;j++) {
        float avg;
        switch(j) {
          case 0:
            avg=(state[0]+state[1]+state[CCASIZE-1])/3.0;
            break;
          case CCASIZE-1:
            avg=(state[CCASIZE-2]+state[CCASIZE-1]+state[0])/3.0;
            break;
          default:
            avg=(state[j-1]+state[j]+state[j+1])/3.0;
        }
        double newval=f(avg);
        grid[j][k]=newval-(long)newval;
      }
      for(int l=0;l<CCASIZE;l++)
        state[l]=grid[l][k];
    }
  }

  float getValue(int row,int col) {
    return grid[row][col];
  }

};

struct CCA : Module {
  enum ParamId {
    CF_PARAM,CV_X_PARAM,CV_Y_PARAM,SCALE_PARAM,OFS_PARAM,F_PARAM,THRS_PARAM,PARAMS_LEN
  };
  enum InputId {
    POLY_0_15_INPUT,POLY_16_31_INPUT,CV_X_INPUT,CV_Y_INPUT,SCALE_INPUT,OFS_INPUT,F_INPUT,CF_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,GATE_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  CCAMatrix ccaMatrix;
  int curRow[16]={};
  int curCol[16]={};
  float init[CCASIZE]={};
  float cfParam=0.f;
  int channels=0;
  int funcParam=0;
  dsp::ClockDivider divider;
  std::function<float(float)> funcs[5]={
    [&](float a) { return a*(1+cfParam);},
    [&](float a) { return a+cfParam;},
    [&](float a) {
      if(a<cfParam) return a/cfParam;
      else return a/(1-cfParam);
    },
    [&](float a) {
      if(a<cfParam) return a/cfParam;
      else return (1-a)/(1-cfParam);
    },
    [&](float a) {
      return a;
    }
  };
  CCA() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(CF_PARAM,0,1,0.5,"Function parameter");
    configSwitch(F_PARAM,0,4,0,"Function",{"a+x","a*(1+x)","a/x","^","id"});
    getParamQuantity(F_PARAM)->snapEnabled=true;
    configParam(CV_X_PARAM,0,CCASIZE-1,0,"X");
    getParamQuantity(CV_X_PARAM)->snapEnabled=true;
    configParam(CV_Y_PARAM,0,CCASIZE-1,0,"Y");
    getParamQuantity(CV_Y_PARAM)->snapEnabled=true;
    configParam(SCALE_PARAM,0,10,2,"Out Scale Factor");
    configParam(OFS_PARAM,-5,5,-1,"Out Offset Factor");
    configInput(CV_X_INPUT,"CV X");
    configInput(CV_Y_INPUT,"CV_Y");
    configInput(POLY_0_15_INPUT,"Value input 0-15");
    configInput(POLY_16_31_INPUT,"Value input 16-31");
    configInput(OFS_INPUT,"Out voltage offset");
    configInput(SCALE_INPUT,"Out scale factor");
    configInput(F_INPUT,"Function");
    configInput(CF_INPUT,"Function parameter");
    configParam(THRS_PARAM,0,1,0,"Gate Threshold");
    configOutput(GATE_OUTPUT,"Gate");
    configOutput(CV_OUTPUT,"CV");
    divider.setDivision(64);
  }

  std::vector<int> getSelected(int row,int col) {
    std::vector<int> ret;
    for(int chn=0;chn<channels;chn++) {
      if(row==curRow[chn]&&col==curCol[chn]) {
        ret.push_back(chn);
      }
    }
    return ret;
  }

  float getCellValue(int row,int col) {
    return ccaMatrix.getValue(row,col);
  }

  float getInitValue(int pos) {
    return init[pos];
  }

  bool isCurrent(int row,int col) {
    return row==int(params[CV_Y_PARAM].getValue())&&col==int(params[CV_X_PARAM].getValue());
  }

  void setCurrent(int row,int col) {
    getParamQuantity(CV_X_PARAM)->setValue(float(col));
    getParamQuantity(CV_Y_PARAM)->setValue(float(row));
  }

  void process(const ProcessArgs &args) override {
    if(divider.process()) {
      if(inputs[F_INPUT].isConnected()) {
        getParamQuantity(F_PARAM)->setValue(inputs[F_INPUT].getVoltage()*0.4f);
      }
      if(inputs[CF_INPUT].isConnected()) {
        getParamQuantity(CF_PARAM)->setValue(inputs[CF_INPUT].getVoltage()*0.1f);
      }
      bool changed=false;
      if(inputs[POLY_0_15_INPUT].isConnected()) {
        for(int k=0;k<16;k++) {
          float v=clamp(inputs[POLY_0_15_INPUT].getVoltage(k)/10.f,0.f,1.f);
          if(v!=init[k]) {
            init[k]=v;
            changed=true;
          }
        }
      }
      if(inputs[POLY_16_31_INPUT].isConnected()) {
        for(int k=0;k<16;k++) {
          float v=clamp(inputs[POLY_16_31_INPUT].getVoltage(k)/10.f,0.f,1.f);
          if(v!=init[k+16]) {
            init[k+16]=v;
            changed=true;
          }
        }
      }
      if(params[CF_PARAM].getValue()!=cfParam) {
        cfParam=params[CF_PARAM].getValue();
        changed=true;
      }
      if(params[F_PARAM].getValue()!=funcParam) {
        funcParam=params[F_PARAM].getValue();
        changed=true;
      }
      if(changed) {
        //ccaMatrix.cfunc(&init[0],[&](float a) { return a*(1+cfParam);});
        ccaMatrix.cfunc(&init[0],funcs[funcParam]);
      }
    }
    int channelsX=0;
    if(inputs[CV_X_INPUT].isConnected()) {
      channelsX=inputs[CV_X_INPUT].getChannels();
      for(int chn=0;chn<16;chn++) {
        int index=int(inputs[CV_X_INPUT].getVoltage(chn)/10.f*float(CCASIZE));
        index+=params[CV_X_PARAM].getValue();
        while(index<0)
          index+=CCASIZE;
        index%=CCASIZE;
        curCol[chn]=index;
      }
    } else {
      curCol[0]=(int(params[CV_X_PARAM].getValue())%CCASIZE);
      for(int chn=1;chn<16;chn++)
        curCol[chn]=0;
    }
    int channelsY=0;
    if(inputs[CV_Y_INPUT].isConnected()) {
      channelsY=inputs[CV_Y_INPUT].getChannels();
      for(int chn=0;chn<16;chn++) {
        int index=int(inputs[CV_Y_INPUT].getVoltage(chn)/10.f*float(CCASIZE));
        index+=params[CV_Y_PARAM].getValue();
        while(index<0)
          index+=CCASIZE;
        index%=CCASIZE;
        curRow[chn]=index;
      }
    } else {
      curRow[0]=(int(params[CV_Y_PARAM].getValue())%CCASIZE);
      for(int chn=1;chn<16;chn++)
        curRow[chn]=0;
    }
    channels=std::max(std::max(channelsX,channelsY),1);
    for(int chn=0;chn<channels;chn++) {
      float curVal=getCellValue(curRow[chn],curCol[chn]);
      float scale=params[SCALE_PARAM].getValue();
      if(inputs[SCALE_INPUT].isConnected()) {
        scale+=inputs[SCALE_INPUT].getPolyVoltage(chn);
      }
      float offset=params[OFS_PARAM].getValue();
      if(inputs[OFS_INPUT].isConnected()) {
        offset+=inputs[OFS_INPUT].getPolyVoltage(chn);
      }
      outputs[CV_OUTPUT].setVoltage(curVal*scale+offset,chn);
      if(curVal>=params[THRS_PARAM].getValue()) {
        outputs[GATE_OUTPUT].setVoltage(10.f,chn);
      } else {
        outputs[GATE_OUTPUT].setVoltage(0.f,chn);
      }
    }
    outputs[CV_OUTPUT].setChannels(channels);
    outputs[GATE_OUTPUT].setChannels(channels);
  }
};

struct CellColors {
  NVGcolor selectOnColor=nvgRGB(0xff,0xff,0xff);
  NVGcolor selectOffColor=nvgRGB(0x44,0x44,0xaa);
  NVGcolor chnColors[16]={nvgRGB(255,0,0),nvgRGB(0,255,0),nvgRGB(55,55,255),nvgRGB(255,255,0),nvgRGB(255,0,255),nvgRGB(0,255,255),nvgRGB(128,0,0),nvgRGB(196,85,55),nvgRGB(128,128,80),nvgRGB(255,128,0),nvgRGB(255,0,128),nvgRGB(0,128,255),nvgRGB(128,66,128),nvgRGB(128,255,0),nvgRGB(128,128,255),nvgRGB(128,255,255)};
  NVGcolor pallette[11]={nvgRGB(0x0,0x00,0x66),nvgRGB(0x0,0x22,0x99),nvgRGB(0x33,0x44,0xAA),nvgRGB(0x00,0x77,0xBB),nvgRGB(0x22,0x77,0xBB),nvgRGB(0x44,0x77,0xBB),nvgRGB(0x55,0x66,0xBB),nvgRGB(0x66,0x44,0xFF),nvgRGB(0x77,0x44,0xFF),nvgRGB(0x88,0x44,0x88),nvgRGB(0x99,0x44,0x55)};

};

struct CCADisplay : OpaqueWidget {
  CCA *module=nullptr;
  int numRows=CCASIZE;
  const int margin=2;
  int oldC=-1;
  int oldR=-1;
  const int cellXSize=11;
  const int cellYSize=11;
  Vec dragPosition={};
  CellColors cellColors;
  int colorMode=0;
  std::vector<std::string> colorModeLabels={"Grey","Palette 1"};

  CCADisplay(CCA *_module,Vec pos) : module(_module) {
    box.size=Vec(CCASIZE*cellXSize+margin*2,CCASIZE*cellYSize+margin*2);
    box.pos=pos;
  }
  NVGcolor getPaletteColor(float v) {
    int index=floor(v*10.f);
    if(index==10)
      return cellColors.pallette[index];
    return nvgLerpRGBA(cellColors.pallette[index],cellColors.pallette[index+1],(v-(index/10.f))*10.f);
  }

  NVGcolor getCellColor(int row,int col) {
    std::vector<int> selected=module->getSelected(row,col);
    float v=module->getCellValue(row,col);
    NVGcolor cellColor;
    switch(colorMode) {
      case 1:
        cellColor=getPaletteColor(v);
        break;
      default:
        cellColor=nvgRGB(int(v*255.f),int(v*255.f),int(v*255.f));
    }
    switch(selected.size()) {
      case 0:
        return cellColor;
        break;
      case 1:
        return nvgLerpRGBA(cellColor,cellColors.chnColors[selected[0]],0.5f);
        break;
      default:
        return cellColors.selectOnColor;
    }
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


    Vec size=box.size;
    float cellSize=size.x/numRows-margin;

    float posY=1;
    for(int r=0;r<numRows;r++) {
      float posX=1;
      for(int c=0;c<numRows;c++) {
        NVGcolor dg=getCellColor(r,c);
        nvgBeginPath(args.vg);
        nvgRect(args.vg,posX,posY,cellSize,cellSize);
        nvgFillColor(args.vg,dg);
        if(module->isCurrent(r,c)) {
          nvgStrokeColor(args.vg,nvgRGB(255,255,100));
        } else {
          nvgStrokeColor(args.vg,nvgRGB(64,64,40));
        }
        nvgStrokeWidth(args.vg,2);
        nvgStroke(args.vg);
        nvgFill(args.vg);
        posX+=cellSize+margin;
      }
      posY+=cellSize+margin;
    }
    nvgBeginPath(args.vg);
    float mid=(cellSize+margin)*(numRows/2);
    float end=(cellSize+margin)*(numRows);
    nvgMoveTo(args.vg,0,mid);
    nvgLineTo(args.vg,end,mid);
    nvgMoveTo(args.vg,mid,0);
    nvgLineTo(args.vg,mid,end);
    nvgStrokeColor(args.vg,nvgRGB(80,80,80));
    nvgStrokeWidth(args.vg,2);
    nvgStroke(args.vg);
  }

  virtual void onButton(const event::Button &e) override {
    if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_RIGHT) {
      int c=oldC=floor(e.pos.x/(box.size.x/float(numRows)));
      int r=oldR=floor(e.pos.y/(box.size.y/float(numRows)));
      module->setCurrent(r,c);
      e.consume(this);
      dragPosition=e.pos;
    }
  }

  void onDragMove(const event::DragMove &e) override {

    dragPosition=dragPosition.plus(e.mouseDelta.div(getAbsoluteZoom()));
    if(isMouseInDrawArea(dragPosition)) {
      int c=floor(dragPosition.x/(box.size.x/float(numRows)));
      int r=floor(dragPosition.y/(box.size.y/float(numRows)));
      if(c!=oldC||r!=oldR) {
        if(e.button==GLFW_MOUSE_BUTTON_RIGHT)
          module->setCurrent(r,c);
      }
      oldC=c;
      oldR=r;
    }
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

struct InitDisplay : OpaqueWidget {
  CCA *module=nullptr;
  int numValues=CCASIZE;
  const int margin=2;

  InitDisplay(CCA *_module,Vec pos) : module(_module) {
    box.size=Vec(11,CCASIZE*11+margin*2);
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
    nvgBeginPath(args.vg);
    nvgRect(args.vg,0,0,box.size.x,box.size.y);
    nvgFillColor(args.vg,nvgRGB(128,128,128));
    nvgFill(args.vg);
    Vec size=box.size;
    float cellSize=size.y/numValues-margin;
    float posY=1;
    for(int r=0;r<numValues;r++) {
      float posX=1;
      float v=module->getInitValue(r);
      NVGcolor valColor=nvgRGB(int(v*255.f),int(v*255.f),int(v*255.f));
      nvgBeginPath(args.vg);
      nvgRect(args.vg,posX,posY,cellSize,cellSize);
      nvgFillColor(args.vg,valColor);
      nvgFill(args.vg);
      posY+=cellSize+margin;
    }
  }
};


struct CCAWidget : ModuleWidget {
  CCADisplay *display=nullptr;
  CCAWidget(CCA *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/CCA.svg")));
    display=new CCADisplay(module,mm2px(Vec(10,MHEIGHT-124.5)));
    addChild(display);
    auto initDisplay=new InitDisplay(module,mm2px(Vec(4,MHEIGHT-124.5)));
    addChild(initDisplay);
    float x=134;
    float y=TY(106);
    addInput(createInput<SmallPort>(mm2px(Vec(x,y)),module,CCA::POLY_0_15_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,CCA::POLY_16_31_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,CCA::F_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,CCA::F_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,CCA::CF_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,CCA::CF_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,CCA::CV_X_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,CCA::CV_X_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,CCA::CV_Y_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,CCA::CV_Y_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,CCA::SCALE_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,CCA::SCALE_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,CCA::OFS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+8,y)),module,CCA::OFS_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,CCA::THRS_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+8,y)),module,CCA::GATE_OUTPUT));
    y+=12;
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+8,y)),module,CCA::CV_OUTPUT));
  }

  void appendContextMenu(Menu *menu) override {
    //CCA *module=dynamic_cast<CCA *>(this->module);
    //assert(module);
    menu->addChild(new MenuSeparator);
    auto colorSelect=new LabelIntSelectItem(&display->colorMode,display->colorModeLabels);
    colorSelect->text="Color Mode";
    colorSelect->rightText=display->colorModeLabels[display->colorMode]+"  "+RIGHT_ARROW;
    menu->addChild(colorSelect);
  }

};


Model *modelCCA=createModel<CCA,CCAWidget>("CCA");