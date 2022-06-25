#include "plugin.hpp"
#include "rnd.h"

#define CCASIZE 32

struct CCA2Matrix {
  double grid[CCASIZE][CCASIZE]={};
  double grid2[CCASIZE][CCASIZE]={};
  double gridSave[CCASIZE][CCASIZE]={};
  float cfParam=0.3f;
  std::vector <std::string> typeLabels={"Wighted","Normal","NWSE only"};
  std::vector<std::string> funcLabels={"a+x","a*(1+x)","a/x","^","test","test2","id"};
  int avgType=0;
  RND rnd;
  std::function<double(double)> funcs[7]={[&](double a) {
    return a*(1+cfParam);
  },[&](double a) {
    return a+cfParam;
  },[&](double a) {
    if(a<cfParam)
      return a/cfParam;
    else
      return a/(1-cfParam);
  },[&](double a) {
    if(a<cfParam)
      return a/cfParam;
    else
      return (1-a)/(1-cfParam);
  },[&](double a) {
    if(a<cfParam)
      return 0.0;
    else
      return (1.0-a)/(1.0-cfParam);
  },[&](double a) {
    if(a<cfParam)
      return a/cfParam;
    else
      return 0.;
  },[&](double a) {
    return a;
  }};

  void randomize(float dens) {
    for(int k=0;k<CCASIZE;k++) {
      for(int j=0;j<CCASIZE;j++) {
        grid[k][j]=rnd.nextWeibull(10-dens);
        gridSave[k][j]=grid[k][j];
      }
    }
  }

  float getValue(int row,int col) {
    return grid[row][col];
  }

  void setValue(int row,int col,float v) {
    gridSave[row][col]=grid[row][col]=v;
  }

  void reset() {
    for(int x=0;x<CCASIZE;x++)
      for(int y=0;y<CCASIZE;y++) {
        grid[x][y]=gridSave[x][y];
      }
  }

  void clear() {
    for(int x=0;x<CCASIZE;x++)
      for(int y=0;y<CCASIZE;y++) {
        grid[x][y]=gridSave[x][y]=0.f;
      }
  }

  float getAvg(uint8_t row,uint8_t col,int type=0) {
    uint8_t top=row==0?CCASIZE-1:row-1;
    uint8_t bottom=row==CCASIZE-1?0:row+1;
    uint8_t left=col==0?CCASIZE-1:col-1;
    uint8_t right=col==CCASIZE-1?0:col+1;
    switch(type) {
      case 1:
        return (grid2[row][left]+grid2[top][col]+grid2[row][right]+grid2[bottom][col]+grid2[top][left]+grid2[top][right]+grid2[bottom][right]+grid2[bottom][left])/8.f;
      case 2:
        return (grid2[row][left]+grid2[top][col]+grid2[row][right]+grid2[bottom][col])/4.f;
      default:
        return (grid2[row][left]+grid2[top][col]+grid2[row][right]+grid2[bottom][col]+0.75*(grid2[top][left]+grid2[top][right]+grid2[bottom][right]+grid2[bottom][left]))/7.f;
    }
  }

  void nextGeneration(int nr=0) {
    for(int x=0;x<CCASIZE;x++)
      for(int y=0;y<CCASIZE;y++)
        grid2[y][x]=grid[y][x];
    for(int k=0;k<CCASIZE;k++) {
      for(int j=0;j<CCASIZE;j++) {
        float r=funcs[nr](getAvg(k,j,avgType));
        grid[k][j]=r-(int)r;
      }
    }
  }
  void fromJson(json_t *jWorld) {
    json_t *jGrid=json_object_get(jWorld,"grid");
    json_t *jGridSave=json_object_get(jWorld,"gridSave");
    if(!jGrid) return;
    for(int k=0;k<CCASIZE;k++) {
      json_t *row=json_array_get(jGrid,k);
      json_t *rowSave=json_array_get(jGridSave,k);
      for(int j=0;j<CCASIZE;j++) {
        json_t *col=json_array_get(row,j);
        json_t *colSave=json_array_get(rowSave,j);
        grid[k][j]=json_real_value(col);
        gridSave[k][j]=json_real_value(colSave);
      }
    }

  }

  json_t *toJson() {
    json_t *jWorld=json_object();
    json_t *dataGridSave=json_array();
    json_t *dataGrid=json_array();
    for(int k=0;k<CCASIZE;k++) {
      json_t *rowSave=json_array();
      json_t *row=json_array();
      for(int j=0;j<CCASIZE;j++) {
        json_array_append_new(rowSave,json_real(gridSave[k][j]));
        json_array_append_new(row,json_real(grid[k][j]));
      }
      json_array_append_new(dataGridSave,rowSave);
      json_array_append_new(dataGrid,row);
    }
    json_object_set_new(jWorld,"gridSave",dataGridSave);
    json_object_set_new(jWorld,"grid",dataGrid);
    return jWorld;
  }

};

struct CCA2 : Module {
  enum ParamId {
    CV_X_PARAM,CV_Y_PARAM,SCALE_PARAM,OFS_PARAM,THRS_PARAM,RND_PARAM,DENS_PARAM,STEP_PARAM,ON_PARAM,RST_PARAM,F_PARAM,CF_PARAM,PARAMS_LEN
  };
  enum InputId {
    CV_X_INPUT,CV_Y_INPUT,SCALE_INPUT,OFS_INPUT,STEP_INPUT,RST_INPUT,ON_INPUT,DENS_INPUT,RND_INPUT,CF_INPUT,INPUTS_LEN
  };
  enum OutputId {
    CV_OUTPUT,GATE_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  CCA2Matrix ccaMatrix;
  int curRow[16]={};
  int curCol[16]={};
  int channels=0;
  dsp::ClockDivider divider;
  dsp::SchmittTrigger stepTrigger;
  dsp::SchmittTrigger manualStepTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::SchmittTrigger rstManualTrigger;
  dsp::PulseGenerator rstPulse;
  dsp::SchmittTrigger rndTrigger;
  dsp::SchmittTrigger manualRndTrigger;

  CCA2() {
    config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
    configParam(DENS_PARAM,1,10,1,"Random Density");
    configButton(RST_PARAM,"Reset");
    configButton(STEP_PARAM,"Next Step");
    configButton(ON_PARAM,"Generation On");
    configParam(CV_X_PARAM,0,CCASIZE-1,0,"X");
    getParamQuantity(CV_X_PARAM)->snapEnabled=true;
    configParam(CV_Y_PARAM,0,CCASIZE-1,0,"Y");
    getParamQuantity(CV_Y_PARAM)->snapEnabled=true;
    configParam(SCALE_PARAM,0,10,2,"Out Scale Factor");
    configParam(OFS_PARAM,-5,5,-1,"Out Offset Factor");
    configParam(CF_PARAM,0,1,0,"Function Param");
    configSwitch(F_PARAM,0,ccaMatrix.funcLabels.size()-1,0,"Function",ccaMatrix.funcLabels);
    configInput(CV_X_INPUT,"CV X");
    configInput(CV_Y_INPUT,"CV_Y");
    configInput(OFS_INPUT,"Out voltage offset");
    configInput(SCALE_INPUT,"Out scale factor");
    configInput(STEP_INPUT,"Next Step");
    configInput(RST_INPUT,"Reset");
    configInput(ON_INPUT,"Generation On");
    configInput(DENS_INPUT,"Random Density");
    configInput(ON_INPUT,"On");
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

  bool isCurrent(int row,int col) {
    return row==int(params[CV_Y_PARAM].getValue())&&col==int(params[CV_X_PARAM].getValue());
  }

  void setCurrent(int row,int col) {
    getParamQuantity(CV_X_PARAM)->setValue(float(col));
    getParamQuantity(CV_Y_PARAM)->setValue(float(row));
  }

  void process(const ProcessArgs &args) override {
    if(inputs[ON_INPUT].isConnected()) {
      getParamQuantity(ON_PARAM)->setValue(inputs[ON_INPUT].getVoltage()>1.f);
    }
    if(inputs[RND_INPUT].isConnected()) {
      getParamQuantity(RND_PARAM)->setValue(inputs[RND_INPUT].getVoltage()>1.f);
    }
    if(inputs[CF_INPUT].isConnected()) {
      getParamQuantity(CF_PARAM)->setValue(inputs[CF_INPUT].getVoltage()/10.f);
    }
    if(inputs[DENS_INPUT].isConnected()) {
      getParamQuantity(DENS_PARAM)->setValue(inputs[DENS_INPUT].getVoltage());
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
    if(rndTrigger.process(inputs[RND_INPUT].getVoltage())|manualRndTrigger.process(params[RND_PARAM].getValue())) {
      rstPulse.trigger(0.001f);
      ccaMatrix.randomize(params[DENS_PARAM].getValue());
    }
    if(rstTrigger.process(inputs[RST_INPUT].getVoltage())|rstManualTrigger.process(params[RST_PARAM].getValue())) {
      rstPulse.trigger(0.001f);
      ccaMatrix.reset();
    }
    bool rstGate=rstPulse.process(args.sampleTime*32);
    if(!rstGate&&(stepTrigger.process(inputs[STEP_INPUT].getVoltage())&&params[ON_PARAM].getValue()>0.f)|manualStepTrigger.process(params[STEP_PARAM].getValue())) {
      ccaMatrix.cfParam=params[CF_PARAM].getValue();
      ccaMatrix.nextGeneration(params[F_PARAM].getValue());
    }
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
      if(curVal>params[THRS_PARAM].getValue()) {
        outputs[GATE_OUTPUT].setVoltage(10.f,chn);
      } else {
        outputs[GATE_OUTPUT].setVoltage(0.f,chn);
      }
    }
    outputs[CV_OUTPUT].setChannels(channels);
    outputs[GATE_OUTPUT].setChannels(channels);
  }

  void dataFromJson(json_t *root) override {
    json_t *jWorld=json_object_get(root,"world");
    if(jWorld) {
      ccaMatrix.fromJson(jWorld);
    }
  }

  json_t *dataToJson() override {
    json_t *root=json_object();
    json_object_set_new(root,"world",ccaMatrix.toJson());
    return root;
  }
};

struct CellColors2 {
  NVGcolor selectOnColor=nvgRGB(0xff,0xff,0xff);
  NVGcolor selectOffColor=nvgRGB(0x44,0x44,0xaa);
  NVGcolor chnColors[16]={nvgRGB(255,0,0),nvgRGB(0,255,0),nvgRGB(55,55,255),nvgRGB(255,255,0),nvgRGB(255,0,255),nvgRGB(0,255,255),nvgRGB(128,0,0),nvgRGB(196,85,55),nvgRGB(128,128,80),nvgRGB(255,128,0),nvgRGB(255,0,128),nvgRGB(0,128,255),nvgRGB(128,66,128),nvgRGB(128,255,0),nvgRGB(128,128,255),nvgRGB(128,255,255)};
  NVGcolor pallette[11]={nvgRGB(0x0,0x00,0x66),nvgRGB(0x0,0x22,0x99),nvgRGB(0x33,0x44,0xAA),nvgRGB(0x00,0x77,0xBB),nvgRGB(0x22,0x77,0xBB),nvgRGB(0x44,0x77,0xBB),nvgRGB(0x55,0x66,0xBB),nvgRGB(0x66,0x44,0xFF),nvgRGB(0x77,0x44,0xFF),nvgRGB(0x88,0x44,0x88),nvgRGB(0x99,0x44,0x55)};
};

struct CCA2Display : OpaqueWidget {
  CCA2 *module=nullptr;
  int numRows=CCASIZE;
  const int margin=2;
  int oldC=-1;
  int oldR=-1;
  float current;
  int colorMode=0;
  std::vector<std::string> colorModeLabels={"Grey","Palette 1"};
  const int cellXSize=11;
  const int cellYSize=11;
  Vec dragPosition={};
  CellColors2 cellColors;

  CCA2Display(CCA2 *_module,Vec pos) : module(_module) {
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

    if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_LEFT) {
      int c=oldC=floor(e.pos.x/(box.size.x/float(numRows)));
      int r=oldR=floor(e.pos.y/(box.size.y/float(numRows)));
      if((e.mods&RACK_MOD_MASK)==0) {
        current=1.f;

      } else if((e.mods&RACK_MOD_MASK)==GLFW_MOD_SHIFT) {
        current=0.f;
      }
      module->ccaMatrix.setValue(r,c,current);
      e.consume(this);
      dragPosition=e.pos;
    } else if(e.action==GLFW_PRESS&&e.button==GLFW_MOUSE_BUTTON_RIGHT) {
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
        else
          module->ccaMatrix.setValue(r,c,current);
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


struct CCA2Widget : ModuleWidget {
  CCA2Display *display=nullptr;

  CCA2Widget(CCA2 *module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance,"res/CCA2.svg")));

    display=new CCA2Display(module,mm2px(Vec(4,4)));
    addChild(display);
    float x=127;
    float x2=131.5;
    float y=16;
    addParam(createParam<MLEDM>(mm2px(Vec(x,y)),module,CCA2::STEP_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+9,y)),module,CCA2::STEP_INPUT));
    addParam(createParam<MLED>(mm2px(Vec(x+18,y)),module,CCA2::ON_PARAM));
    y+=14;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,CCA2::F_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x+9,y)),module,CCA2::CF_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+18,y)),module,CCA2::CF_INPUT));
    y+=12;
    addParam(createParam<MLEDM>(mm2px(Vec(x2,y)),module,CCA2::RST_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x2+9,y)),module,CCA2::RST_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,CCA2::DENS_PARAM));
    addParam(createParam<MLEDM>(mm2px(Vec(x+9,y)),module,CCA2::RND_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x+18,y)),module,CCA2::RND_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x2,y)),module,CCA2::CV_X_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x2+9,y)),module,CCA2::CV_X_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x2,y)),module,CCA2::CV_Y_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x2+9,y)),module,CCA2::CV_Y_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x2,y)),module,CCA2::SCALE_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x2+9,y)),module,CCA2::SCALE_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x2,y)),module,CCA2::OFS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(x2+9,y)),module,CCA2::OFS_INPUT));
    y+=12;
    addParam(createParam<TrimbotWhite>(mm2px(Vec(x,y)),module,CCA2::THRS_PARAM));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+9,y)),module,CCA2::GATE_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(x+18,y)),module,CCA2::CV_OUTPUT));
  }

  void appendContextMenu(Menu *menu) override {
    CCA2 *module=dynamic_cast<CCA2 *>(this->module);
    assert(module);
    menu->addChild(new MenuSeparator);
    auto avgTypeSelect=new LabelIntSelectItem(&module->ccaMatrix.avgType,module->ccaMatrix.typeLabels);
    avgTypeSelect->text="Avg Type";
    avgTypeSelect->rightText=module->ccaMatrix.typeLabels[module->ccaMatrix.avgType]+"  "+RIGHT_ARROW;
    menu->addChild(avgTypeSelect);
    auto colorSelect=new LabelIntSelectItem(&display->colorMode,display->colorModeLabels);
    colorSelect->text="Color Mode";
    colorSelect->rightText=display->colorModeLabels[display->colorMode]+"  "+RIGHT_ARROW;
    menu->addChild(colorSelect);
    struct ClearItem : ui::MenuItem {
      CCA2 *module;
      int nr;

      ClearItem(CCA2 *m) : module(m) {
      }

      void onAction(const ActionEvent &e) override {
        if(!module)
          return;
        module->ccaMatrix.clear();
      }
    };
    auto clearMenu=new ClearItem(module);
    clearMenu->text="Clear";
    menu->addChild(clearMenu);
  }
};


Model *modelCCA2=createModel<CCA2,CCA2Widget>("CCA2");