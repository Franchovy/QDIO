/*
  ==============================================================================

    MenuItem.cpp
    Created: 9 May 2020 3:55:20pm
    Author:  maxime

  ==============================================================================
*/

#include "MenuItem.h"

MenuItem::MenuItem(int numMenus) {
    for (int i = 0; i < numMenus; i++) {
        menus.add(std::make_unique<PopupMenu>());
    }
}

void MenuItem::addMenuItem(int menuIndex, PopupMenu::Item newItem) {
    menus[menuIndex]->addItem(newItem);
}

int MenuItem::callMenu(int menuIndex) {
    int result = menus[menuIndex]->show();
    return result;
}
