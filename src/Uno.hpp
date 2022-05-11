#ifndef DBCAEMODULES_UNO_HPP
#define DBCAEMODULES_UNO_HPP
#define NUM_STEPS 8
struct UnoExpanderMessage {
  float cv[NUM_STEPS];
  float prob[NUM_STEPS];
  float setStep=-1;
  bool glide[NUM_STEPS]={};
  bool rst[NUM_STEPS]={};
  bool reset=false;
};
template<typename M>
struct UnoStrip {
  M* module;
  int stepCounter=0;
  dsp::PulseGenerator rstPulse;
  dsp::SchmittTrigger clockTrigger;
  dsp::SchmittTrigger rstTrigger;
  dsp::ClockDivider lightDivider;
  bool forward=true;
  RND rnd;
  int setStep=-1;
  bool masterReset = false;
  void resetCurrentStep(int direction) {
    switch(direction) {
      case 0:  // forward
        stepCounter=NUM_STEPS-1;
        break;
      case 1: // backward
        stepCounter=0;
        break;
      case 2:
        stepCounter=1;
        forward=false;
        break;
    }
    //INFO("RST %d",stepCounter);
    next(direction);
  }

  void next(int direction) {
    if(setStep>=0) {
      stepCounter=setStep;
      setStep=-1;
      return;
    }
    if(module->getRst(stepCounter)) {
      resetCurrentStep(direction);
      return;
    }
    for(int k=0;k<NUM_STEPS*2;k++) {
      switch(direction) {
        case 0:  // forward
          if(stepCounter>=NUM_STEPS-1)
            stepCounter=0;
          else
            stepCounter++;
          break;
        case 1: // backward
          if(stepCounter<=0)
            stepCounter=NUM_STEPS-1;
          else
            stepCounter--;
          break;
        case 2: // <->
          if(forward) {
            if(stepCounter>=NUM_STEPS-1) {
              stepCounter=NUM_STEPS-1;
              forward=false;
            } else {
              stepCounter++;
            }
          } else {
            if(stepCounter<=0) {
              stepCounter=1;
              forward=true;
            } else {
              stepCounter--;
            }
          }
          break;
      }
      float prob=module->getProb(stepCounter);
      double d=rnd.nextDouble();
      //INFO("%d prop %f %lf",stepCounter,prob,d);
      if(d>=(1-prob)) break;
    }
    //INFO("Next %d",stepCounter);
  }
  void process(float sampleTime) {
    int direction=module->params[M::DIR_PARAM].getValue();;
    bool advance=false;
    if(rstTrigger.process(module->inputs[M::RST_INPUT].getVoltage()) | masterReset) {
      rstPulse.trigger(0.001f);
      float seedParam=0;
      if(module->inputs[M::SEED_INPUT].isConnected()) {
        seedParam=module->inputs[M::SEED_INPUT].getVoltage();
        seedParam = floorf(seedParam*10000)/10000;
        seedParam/=10.f;
      }
      auto seedInput = (unsigned long long)(floor((double)seedParam*(double)ULONG_MAX));
      //INFO("%.8f %lld",seedParam,seedInput);
      rnd.reset(seedInput);
      resetCurrentStep(direction);
      advance=true;
    }
    bool resetGate=rstPulse.process(sampleTime);
    if(clockTrigger.process(module->inputs[M::CLK_INPUT].getVoltage())&&!resetGate) {
      next(direction);
      advance=true;
    }
    if(advance) {
      float cv=module->getCV(stepCounter);
      module->outputs[M::CV_OUTPUT].setVoltage(cv);
    }

    if(lightDivider.process()) {
      bool gateOn=false;
      if(module->getGlide(stepCounter)) {
        module->outputs[M::GATE_OUTPUT].setVoltage(10.f);
        gateOn=true;
      } else {
        gateOn=module->inputs[M::CLK_INPUT].getVoltage()>1.f;
        module->outputs[M::GATE_OUTPUT].setVoltage(gateOn?10.0f:0.0f);
      }
      for(int k=0;k<NUM_STEPS;k++) {
        if(k==stepCounter) {
          module->lights[k].setSmoothBrightness(gateOn?1.f:0.f,0.1f);
          module->outputs[M::STEP_OUTPUT].setVoltage(gateOn?10.f:0.f,k);
        }
        else {
          module->lights[k].setSmoothBrightness(0.f,0.1f);
          module->outputs[M::STEP_OUTPUT].setVoltage(0.f,k);
        }
      }
    }
    module->outputs[M::STEP_OUTPUT].setChannels(NUM_STEPS);
  }
};
#endif //DBCAEMODULES_UNO_HPP
