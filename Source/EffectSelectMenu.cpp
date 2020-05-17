/*
  ==============================================================================

    EffectSelectMenu.cpp
    Created: 28 Apr 2020 1:20:01pm
    Author:  maxime

  ==============================================================================
*/

#include "EffectSelectMenu.h"


void EffectSelectMenu::mouseDown(const MouseEvent &event) {

    ComboBox::mouseDown(event);
}

void EffectSelectMenu::mouseDrag(const MouseEvent &event) {

    ComboBox::mouseDrag(event);
}

void EffectSelectMenu::mouseUp(const MouseEvent &event) {

    ComboBox::mouseUp(event);
}

EffectSelectMenu::EffectSelectMenu() : ComboBox() {
    onChange = [=] {
        auto selectedIndex = getSelectedItemIndex();
        if (selectedIndex < 0) {
            std::cout << "useless id" << newLine;
            std::cout << getSelectedId() << newLine;
        } else {
            auto effectToLoad = getItemText(selectedIndex);
            EffectLoader::loadEffect(effectToLoad);

            auto effectLoaded = EffectLoader::loadEffect(effectToLoad);
            /*if (effectLoaded.isValid()) {
                auto main = EffectScene::getScene();
                main->createEffect(main.getTree(), effectLoaded);
            }

            effectSelectMenu.setSelectedItemIndex(0, dontSendNotification);
            effectSelectMenu.setText("Select Effect");*/
        }
    };
}
