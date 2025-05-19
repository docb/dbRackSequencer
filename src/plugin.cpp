#include "plugin.hpp"


Plugin *pluginInstance;

extern Model *modelJTChords;
extern Model *modelChords;
extern Model *modelAG;
extern Model *modelACC;
extern Model *modelCV;
extern Model *modelKlee;
extern Model *modelM851;
extern Model *modelCYC;
extern Model *modelPwmClock;
extern Model *modelTD4;
extern Model *modelP4;
extern Model *modelP16;
extern Model *modelPXY;
extern Model *modelC42;
extern Model *modelC42E;
extern Model *modelSum;
extern Model *modelSE;
extern Model *modelUno;
extern Model *modelUnoE;
extern Model *modelTheMatrix;
extern Model *modelTheMatrix4;
extern Model *modelN3;
extern Model *modelCCA;
extern Model *modelCCA2;
extern Model *modelP16A;
extern Model *modelMouseSeq;
extern Model *modelTME;
extern Model *modelCSR;
extern Model *modelP16B;
extern Model *modelSEQMod;
extern Model *modelPreset;
extern Model *modelCDiv;
extern Model *modelP16S;
extern Model *modelAnt;
extern Model *modelUnoA;
extern Model *modelCarambol;
extern Model *modelPMod;
extern Model *modelASEQ;
extern Model *modelTXVI;
extern Model *modelM16;
extern Model *modelS16;
extern Model *modelM16S;
extern Model *modelHexSeqP2;
extern Model *modelSEQ22;
extern Model *modelSwen;
extern Model *modelMap;
extern Model *modelBC;
extern Model* modelMP8;
extern Model* modelTT;
extern Model* modelMix8;
extern Model* modelAD;
extern Model* modelVCM8;
extern Model* modelG32;
extern Model* modelMTD4;

void init(Plugin *p) {
    pluginInstance = p;

    p->addModel(modelJTChords);
    p->addModel(modelChords);
    p->addModel(modelAG);
    p->addModel(modelACC);
    p->addModel(modelCV);
    p->addModel(modelKlee);
    p->addModel(modelM851);
    p->addModel(modelCYC);
    p->addModel(modelPwmClock);
    p->addModel(modelTD4);
    p->addModel(modelP4);
    p->addModel(modelP16);
    p->addModel(modelPXY);
    p->addModel(modelC42);
    p->addModel(modelC42E);
    p->addModel(modelSum);
    p->addModel(modelSE);
    p->addModel(modelUno);
    p->addModel(modelUnoE);
    p->addModel(modelTheMatrix);
    p->addModel(modelTheMatrix4);
    p->addModel(modelN3);
    p->addModel(modelCCA);
    p->addModel(modelCCA2);
    p->addModel(modelP16A);
    p->addModel(modelMouseSeq);
    p->addModel(modelTME);
    p->addModel(modelCSR);
    p->addModel(modelP16B);
    p->addModel(modelSEQMod);
    p->addModel(modelPreset);
    p->addModel(modelCDiv);
    p->addModel(modelP16S);
    p->addModel(modelAnt);
    p->addModel(modelUnoA);
    p->addModel(modelCarambol);
    p->addModel(modelPMod);
    p->addModel(modelASEQ);
    p->addModel(modelTXVI);
    p->addModel(modelM16);
    p->addModel(modelS16);
    p->addModel(modelM16S);
    p->addModel(modelHexSeqP2);
    p->addModel(modelSEQ22);
    p->addModel(modelSwen);
    p->addModel(modelMap);
    p->addModel(modelBC);
    p->addModel(modelMP8);
    p->addModel(modelTT);
    p->addModel(modelMix8);
    p->addModel(modelAD);
    p->addModel(modelVCM8);
    p->addModel(modelG32);
    p->addModel(modelMTD4);
}

void SmallButtonWithLabel::draw(const Widget::DrawArgs &args) {
    std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
    SvgSwitch::draw(args);
    if (label.length() > 0) {
        nvgFontSize(args.vg, 8);
        nvgFontFaceId(args.vg, font->handle);
        NVGcolor textColor = nvgRGB(0xff, 0xff, 0xaa);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
        nvgFillColor(args.vg, textColor);
        nvgText(args.vg, 11, 7, label.c_str(), NULL);
    }
}

void SmallButtonWithLabelV::draw(const Widget::DrawArgs &args) {
    std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
    SvgSwitch::draw(args);
    if (label.length() > 0) {
        nvgFontSize(args.vg, 8);
        nvgFontFaceId(args.vg, font->handle);
        NVGcolor textColor = nvgRGB(0xff, 0xff, 0xaa);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, textColor);
        nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, label.c_str(), NULL);
    }
}

void UpButtonWidget::onButton(const ButtonEvent &e) {

    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
        pressed = true;
        auto paramWidget = getAncestorOfType<ParamWidget>();
        assert(paramWidget);
        engine::ParamQuantity *pq = paramWidget->getParamQuantity();
        if (pq) {
            if (pq->getValue() < pq->getMaxValue()) {
                pq->setValue(pq->getValue() + 1);
                ChangeEvent c;
                paramWidget->onChange(c);
            }
        } else {
            INFO("no pq");
        }
        e.consume(this);
    }
    if (e.action == GLFW_RELEASE && e.button == GLFW_MOUSE_BUTTON_LEFT) {
        pressed = false;
    }
}

void DownButtonWidget::onButton(const ButtonEvent &e) {
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
        pressed = true;
        auto paramWidget = getAncestorOfType<ParamWidget>();
        assert(paramWidget);
        engine::ParamQuantity *pq = paramWidget->getParamQuantity();
        if (pq) {
            if (pq->getValue() > pq->getMinValue()) {
                pq->setValue(pq->getValue() - 1);
                ChangeEvent c;
                paramWidget->onChange(c);
            }
        }
        e.consume(this);
    }
    if (e.action == GLFW_RELEASE && e.button == GLFW_MOUSE_BUTTON_LEFT) {
        pressed = false;
    }
}

void SelectButton::onDragEnter(const event::DragEnter &e) {
    if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
        auto origin = dynamic_cast<SelectParam *>(e.origin);
        if (origin) {
            auto paramWidget = getAncestorOfType<ParamWidget>();
            assert(paramWidget);
            engine::ParamQuantity *pq = paramWidget->getParamQuantity();
            if (pq) {
                pq->setValue(_value);
            }
        }
    }
    Widget::onDragEnter(e);
}

void SelectButtonH::onDragEnter(const event::DragEnter &e) {
    if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
        auto origin = dynamic_cast<SelectParamH *>(e.origin);
        if (origin) {
            auto paramWidget = getAncestorOfType<ParamWidget>();
            assert(paramWidget);
            engine::ParamQuantity *pq = paramWidget->getParamQuantity();
            if (pq) {
                pq->setValue(_value);
            }
        }
    }
    Widget::onDragEnter(e);
}
