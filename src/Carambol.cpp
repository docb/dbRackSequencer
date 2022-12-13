#include "plugin.hpp"

struct VecD {
  double x = 0.f;
  double y = 0.f;

  VecD() {}
  VecD(double xy) : x(xy), y(xy) {}
  VecD(double x, double y) : x(x), y(y) {}

  VecD neg() const {
    return VecD(-x, -y);
  }
  VecD plus(VecD b) const {
    return VecD(x + b.x, y + b.y);
  }
  VecD minus(VecD b) const {
    return VecD(x - b.x, y - b.y);
  }
  VecD mult(double s) const {
    return VecD(x * s, y * s);
  }
  VecD mult(VecD b) const {
    return VecD(x * b.x, y * b.y);
  }
  VecD div(double s) const {
    return VecD(x / s, y / s);
  }
  VecD div(VecD b) const {
    return VecD(x / b.x, y / b.y);
  }
  double dot(VecD b) const {
    return x * b.x + y * b.y;
  }
  double arg() const {
    return std::atan2(y, x);
  }
  double norm() const {
    return std::hypot(x, y);
  }
  VecD normalize() const {
    return div(norm());
  }
  double square() const {
    return x * x + y * y;
  }
  double area() const {
    return x * y;
  }

  VecD rotate(double angle) {
    double sin = std::sin(angle);
    double cos = std::cos(angle);
    return VecD(x * cos - y * sin, x * sin + y * cos);
  }

  VecD flip() const {
    return VecD(y, x);
  }
  VecD min(VecD b) const {
    return VecD(std::fmin(x, b.x), std::fmin(y, b.y));
  }
  VecD max(VecD b) const {
    return VecD(std::fmax(x, b.x), std::fmax(y, b.y));
  }
  VecD abs() const {
    return VecD(std::fabs(x), std::fabs(y));
  }
  VecD round() const {
    return VecD(std::round(x), std::round(y));
  }
  VecD floor() const {
    return VecD(std::floor(x), std::floor(y));
  }
  VecD ceil() const {
    return VecD(std::ceil(x), std::ceil(y));
  }
  bool equals(VecD b) const {
    return x == b.x && y == b.y;
  }
  bool isZero() const {
    return x == 0.f && y == 0.f;
  }
  bool isFinite() const {
    return std::isfinite(x) && std::isfinite(y);
  }

  bool isEqual(VecD b) const {
    return equals(b);
  }

  void add(VecD other) {
    x+=other.x;
    y+=other.y;
  }
  void sub(VecD other) {
    x-=other.x;
    y-=other.y;
  }

  void dv(double d) {
    x/=d;
    y/=d;
  }
};
struct Ball {
  int nr;
  double radius;
  double mass;
  VecD pos;
  VecD vel;
  int wallHit=-1;
  int collision=-1;
  Ball(int _nr,double _radius,double _mass,VecD _pos,VecD _vel) : nr(_nr),radius(_radius),mass(_mass),pos(_pos),vel(_vel) {}
  void process(VecD gravity,double dt) {
    vel.add(gravity.mult(dt));
    pos.add(vel.mult(dt));
  }
};

// Simulation ported and adapted from Ten Minute Physics -- Copyright 2021 Matthias Müller

struct World {
  RND rnd;
  Vec size;
  float scale;
  int division;
  unsigned count=8;
  double radius[16]={0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1};
  double restitution=1.f;
  std::vector<Ball> balls;
  VecD gravity={0,0};
  World(Vec _size,int _division) {
    float sMin=2.;
    scale = std::min(_size.x,_size.y)/sMin;
    size.x = _size.x/scale;
    size.y = _size.y/scale;
    division=_division;
    _init();
  }

  void setRadius(float r,int i) {
    radius[i]=r;
    balls[i].radius=r;
    balls[i].mass=M_PI * r * r;
  }

  void _init() {
    balls.clear();
    for (unsigned i=0;i<count;i++) {
      double mass = M_PI * radius[i] * radius[i];
      VecD pos = VecD(rnd.nextDouble() * size.x, rnd.nextDouble() * size.y);
      VecD vel = VecD(-1.0 + 2.0 * rnd.nextDouble(), -1.0 + 2.0 * rnd.nextDouble());
      balls.emplace_back(i,radius[i], mass, pos, vel);
    }

  }

  Vec coord(VecD pos) {
    VecD ret = pos.mult(double(scale));
    return Vec(float(ret.x),float(ret.y));
  }

  void processBallCollision(Ball &ball1, Ball &ball2)
  {
    VecD dir=ball2.pos.minus(ball1.pos);
    double d = dir.norm();
    if (d == 0.0 || d > ball1.radius+ball2.radius)
      return;
    ball1.collision=ball2.nr;
    ball2.collision=ball1.nr;
    dir=dir.div(d);

    double corr = (ball1.radius + ball2.radius - d) / 2.f;
    ball1.pos.sub(dir.mult(corr));
    ball2.pos.add(dir.mult(corr));

    double v1 = ball1.vel.dot(dir);
    double v2 = ball2.vel.dot(dir);

    double m1 = ball1.mass;
    double m2 = ball2.mass;

    double newV1 = (m1 * v1 + m2 * v2 - m2 * (v1 - v2) * restitution) / (m1 + m2);
    double newV2 = (m1 * v1 + m2 * v2 - m1 * (v2 - v1) * restitution) / (m1 + m2);

    ball1.vel.add(dir.mult(newV1 - v1));
    ball2.vel.add(dir.mult(newV2 - v2));
  }

  void processWallCollision(Ball &ball)
  {
    if (ball.pos.x < ball.radius) {
      ball.pos.x = ball.radius;
      ball.vel.x = -ball.vel.x;
      ball.wallHit=1;
    }
    if (ball.pos.x > size.x - ball.radius) {
      ball.pos.x = size.x - ball.radius;
      ball.vel.x = -ball.vel.x;
      ball.wallHit=2;
    }
    if (ball.pos.y < ball.radius) {
      ball.pos.y = ball.radius;
      ball.vel.y = -ball.vel.y;
      ball.wallHit=3;
    }

    if (ball.pos.y > size.y - ball.radius) {
      ball.pos.y = size.y - ball.radius;
      ball.vel.y = -ball.vel.y;
      ball.wallHit=4;
    }
  }

  void process(const Module::ProcessArgs& args) {
    for (unsigned i=0;i < count; i++) {
      Ball &ball1 = balls.at(i);
      ball1.process(gravity,division*args.sampleTime);
      for (unsigned j=i+1;j<count;j++) {
        Ball &ball2 = balls.at(j);
        processBallCollision(ball1, ball2);
      }
      processWallCollision(ball1);
    }
  }
};

#define DIVISION 512

struct Carambol : Module {
  enum ParamId {
    COUNT_PARAM,REST_PARAM,RADIUS_PARAM,PARAMS_LEN
  };
  enum InputId {
    CLOCK_INPUT,RST_INPUT,SEED_INPUT,SX_INPUT,SY_INPUT,VX_INPUT,VY_INPUT,RADIUS_INPUT,INPUTS_LEN
  };
  enum OutputId {
    WALL_OUTPUT,COLLISION_OUTPUT,POS_X_OUTPUT,POS_Y_OUTPUT,OUTPUTS_LEN
  };
  enum LightId {
    LIGHTS_LEN
  };
  RND rnd;

  World w={mm2px(Vec(94,94)),DIVISION};
  dsp::ClockDivider divider;
  dsp::PulseGenerator collisionPulse[16];
  dsp::PulseGenerator wallHitPulse[16];
  dsp::SchmittTrigger clock;
  dsp::SchmittTrigger reset;

  int collisions[16];
  int wallHits[16];

  Carambol() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configOutput(WALL_OUTPUT,"Wall hits");
    configOutput(COLLISION_OUTPUT,"Collisions");
    configOutput(POS_X_OUTPUT,"X");
    configOutput(POS_Y_OUTPUT,"Y");
    configParam(COUNT_PARAM,1,16,8,"Count");
    configParam(RADIUS_PARAM,0.05,0.15,0.1,"Radius");
    configParam(REST_PARAM,0,2,1,"Coefficient Of Restitution");
    configInput(CLOCK_INPUT,"Clock");
    configInput(RST_INPUT,"Reset");
    configInput(SEED_INPUT,"Seed");
    configInput(SX_INPUT,"Start X (1-16)");
    configInput(SY_INPUT,"Start Y (1-16)");
    configInput(RADIUS_INPUT,"Radius (1-16)");
    configInput(VX_INPUT,"Velocity start X (1-16)");
    configInput(VY_INPUT,"Velocity start Y (1-16)");
    getParamQuantity(COUNT_PARAM)->snapEnabled=true;
    divider.setDivision(DIVISION);
  }

  void process(const ProcessArgs& args) override {
    bool clockConnected=inputs[CLOCK_INPUT].isConnected();
    if(divider.process()) {
      setCount(params[COUNT_PARAM].getValue());
      w.restitution=params[REST_PARAM].getValue();
      double radius=params[RADIUS_PARAM].getValue();
      w.process(args);
      for(unsigned k=0;k<w.count;k++) {
        if(inputs[RADIUS_INPUT].isConnected()) {
          radius=0.1+clamp(inputs[RADIUS_INPUT].getPolyVoltage(k),-5.f,5.f)*0.01;
        }
        w.setRadius(radius,k);
        if(w.balls[k].wallHit>=0) {
          if(!clockConnected) wallHitPulse[k].trigger(0.001f);
          wallHits[k]=1;
          w.balls[k].wallHit=-1;
        }
        if(w.balls[k].collision>=0) {
          if(!clockConnected) collisionPulse[k].trigger(0.001f);
          collisions[k]=1;
          w.balls[k].collision=-1;
        }
      }
    }
    if(reset.process(inputs[RST_INPUT].getVoltage())) {
      ResetEvent e;
      onReset(e);
    }
    if(clockConnected && clock.process(inputs[CLOCK_INPUT].getVoltage())) {
      for(unsigned k=0;k<w.count;k++) {
        if(collisions[k]) {
          collisionPulse[k].trigger(0.001f);
          collisions[k]=0;
        }
        if(wallHits[k]) {
          wallHitPulse[k].trigger(0.001f);
          wallHits[k]=0;
        }
      }
    }
    for(unsigned k=0;k<w.count;k++) {
      outputs[COLLISION_OUTPUT].setVoltage(collisionPulse[k].process(args.sampleTime)?10.f:0.f,k);
      outputs[WALL_OUTPUT].setVoltage(wallHitPulse[k].process(args.sampleTime)?10.f:0.f,k);
      outputs[POS_X_OUTPUT].setVoltage(w.balls[k].pos.x*5-5,k);
      outputs[POS_Y_OUTPUT].setVoltage(w.balls[k].pos.y*5-5,k);
    }
    outputs[COLLISION_OUTPUT].setChannels(w.count);
    outputs[WALL_OUTPUT].setChannels(w.count);
    outputs[POS_X_OUTPUT].setChannels(w.count);
    outputs[POS_Y_OUTPUT].setChannels(w.count);
  }
  void setCount(unsigned _count) {
    if(w.count!=_count) {
      w.count=_count;
      init();
    }
  }

  void init() {
    w.balls.clear();
    bool sxConnected=inputs[SX_INPUT].isConnected();
    bool syConnected=inputs[SY_INPUT].isConnected();
    bool vxConnected=inputs[VX_INPUT].isConnected();
    bool vyConnected=inputs[VY_INPUT].isConnected();
    for (unsigned i=0;i<w.count;i++) {
      //var radius = 0.05 + Math.random() * 0.1;
      double mass = M_PI * w.radius[i] * w.radius[i];
      VecD pos = VecD(rnd.nextDouble() * w.size.x, rnd.nextDouble() * w.size.y);
      if(sxConnected) {
        pos.x=clamp(inputs[SX_INPUT].getVoltage(i),0.f,10.f)*0.1f*w.size.x;
      }
      if(syConnected) {
        pos.y=clamp(inputs[SY_INPUT].getVoltage(i),0.f,10.f)*0.1f*w.size.y;
      }
      VecD vel = VecD(-1.0 + 2.0 * rnd.nextDouble(), -1.0 + 2.0 * rnd.nextDouble());
      if(vxConnected) {
        vel.x=-1.0 + 2.0 * clamp(inputs[VX_INPUT].getVoltage(i),0.f,10.f)*0.1f;
      }
      if(vyConnected) {
        vel.y=-1.0 + 2.0 * clamp(inputs[VY_INPUT].getVoltage(i),0.f,10.f)*0.1f;
      }
      w.balls.emplace_back(i,w.radius[i], mass, pos, vel);
    }
  }
  void onReset(const ResetEvent &e) override {
    float seedParam=0;
    if(inputs[SEED_INPUT].isConnected())
      seedParam=inputs[SEED_INPUT].getVoltage()/10.f;
    rnd.reset((unsigned long long)(floor((double)seedParam*(double)ULONG_MAX)));
    init();
  }

  void onAdd(const AddEvent &e) override {
    Module::onAdd(e);
    ResetEvent re;
    onReset(re);
  }

};

struct CarambolDisplay : OpaqueWidget {
  Carambol *module=nullptr;
  Vec center;
  NVGcolor bgColor=nvgRGB(0x22,0x22,0x22);
  NVGcolor pointColor=nvgRGB(0x22,0xaa,0xaa);
  NVGcolor chnColors[16]={nvgRGB(255,0,0),nvgRGB(0,255,0),nvgRGB(55,55,255),nvgRGB(255,255,0),nvgRGB(255,0,255),nvgRGB(0,255,255),nvgRGB(128,0,0),nvgRGB(196,85,55),nvgRGB(128,128,80),nvgRGB(255,128,0),nvgRGB(255,0,128),nvgRGB(0,128,255),nvgRGB(128,66,128),nvgRGB(128,255,0),nvgRGB(128,128,255),nvgRGB(128,255,255)};

  CarambolDisplay(Carambol *m,Vec pos,Vec size) : module(m) {
    box.size=size;
    box.pos=pos;
    center={box.size.x/2,box.size.y/2};
  }

  void drawBG(const DrawArgs &args,NVGcolor color) {
    nvgBeginPath(args.vg);
    nvgRect(args.vg,0,0,box.size.x,box.size.y);
    nvgFillColor(args.vg,color);
    nvgFill(args.vg);
  }

  void drawCirc(const DrawArgs &args,float x,float y,float radius,int nr) {
    nvgBeginPath(args.vg);
    nvgCircle(args.vg,x,y,radius);
    nvgFillColor(args.vg,chnColors[nr]);
    nvgFill(args.vg);
  }

  void drawLayer(const DrawArgs &args,int layer) override {
    if(layer==1) {
      _draw(args);
    }
    Widget::drawLayer(args,layer);
  }

  void _draw(const DrawArgs &args) {

    drawBG(args,bgColor);
    if(!module) return;
    for(unsigned k=0;k<module->w.count;k++) {
      Ball &ball=module->w.balls[k];
      Vec pos=module->w.coord(ball.pos);
      drawCirc(args,pos.x,pos.y,ball.radius*module->w.scale,k);
    }
  }

};


struct CarambolWidget : ModuleWidget {
	CarambolWidget(Carambol* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Carambol.svg")));

    auto display=new CarambolDisplay(module,mm2px(Vec(4,4)),mm2px(Vec(94,94)));
    addChild(display);
    float y=104;
    addInput(createInput<SmallPort>(mm2px(Vec(6,y)),module,Carambol::RST_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(16,y)),module,Carambol::SX_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(26,y)),module,Carambol::SY_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(36,y)),module,Carambol::VX_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(46,y)),module,Carambol::VY_INPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(78,y)),module,Carambol::POS_X_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(88,y)),module,Carambol::POS_Y_OUTPUT));
    y=116;
    addInput(createInput<SmallPort>(mm2px(Vec(6,y)),module,Carambol::CLOCK_INPUT));
    addInput(createInput<SmallPort>(mm2px(Vec(16,y)),module,Carambol::SEED_INPUT));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(26,y)),module,Carambol::COUNT_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(36,y)),module,Carambol::REST_PARAM));
    addParam(createParam<TrimbotWhite>(mm2px(Vec(46,y)),module,Carambol::RADIUS_PARAM));
    addInput(createInput<SmallPort>(mm2px(Vec(56,y)),module,Carambol::RADIUS_INPUT));

    addOutput(createOutput<SmallPort>(mm2px(Vec(88,y)),module,Carambol::COLLISION_OUTPUT));
    addOutput(createOutput<SmallPort>(mm2px(Vec(78,y)),module,Carambol::WALL_OUTPUT));
	}
};


Model* modelCarambol = createModel<Carambol, CarambolWidget>("Carambol");