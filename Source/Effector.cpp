/*
  ==============================================================================

    Effector.cpp
    Created: 3 Mar 2020 2:11:13pm
    Author:  maxime

  ==============================================================================
*/

#include "Effector.h"

//==============================================================================
// Line Component methods


LineComponent* LineComponent::dragLine = nullptr;

/**
 * @param event modified mouseEvent to use mainComponent coords.
 */
void LineComponent::mouseDown(const MouseEvent &event) {
    auto thisEvent = event.getEventRelativeTo(this);

    if (auto p = dynamic_cast<AudioPort*>(event.originalComponent)){
        port1 = p;
        p1 = getLocalPoint(p, p->centrePoint);
    } else if (auto p = dynamic_cast<InternalConnectionPort*>(event.originalComponent)){
        p1 = getLocalPoint(p, p->centrePoint);
    }
    p2 = thisEvent.getPosition();

    line.setStart(p1);
    line.setEnd(p2);

    setVisible(true);
    repaint();
}

void LineComponent::mouseDrag(const MouseEvent &event) {
    auto thisEvent = event.getEventRelativeTo(this);
    p2 = thisEvent.getPosition();

    line.setEnd(p2);
    repaint();

    // Pass hover detection to MainComponent
    getParentComponent()->mouseDrag(thisEvent);
}

void LineComponent::mouseUp(const MouseEvent &event) {
    setVisible(false);

    // Pass this event to MainComponent
    auto eventMain = event.getEventRelativeTo(this).withNewPosition(
            event.getPosition() - getPosition()
    );
    getParentComponent()->mouseUp(eventMain);

}

void LineComponent::convert(AudioPort *port2) {
    if (port1 != nullptr) {
        // Connect port1 to port2
        if (port2->isInput ^ port1->isInput)
        {
            lastConnectionLine = new ConnectionLine(*port1, *port2);

            // This calls the propertyChange update in MainComponent
            dragLineTree.setProperty("Connection", lastConnectionLine.get(), nullptr);
        }
    } else if (iPort1 != nullptr) {
        // Connect internal port1 to port2

    }

}

void LineComponent::convert(InternalConnectionPort *iPort2) {

}

//==============================================================================
// GUIEffect methods


GUIEffect::GUIEffect (const MouseEvent &event, EffectVT* parentEVT) :
    EVT(parentEVT)
{
    addAndMakeVisible(resizer);

    //TODO assign the tree before creation of GUIEffect for this to work
    if (parentEVT->getTree().getParent().hasType(ID_EFFECT_VT)){
        auto sizeDef = dynamic_cast<GUIEffect*>(parentEVT->getTree().getParent()
                .getProperty(ID_EFFECT_GUI).getObject())->getWidth() / 3;
        setBounds(event.getPosition().x, event.getPosition().y, sizeDef, sizeDef);
    } else {
        setBounds(event.getPosition().x, event.getPosition().y, 200,200);
    }


    menu.addItem("Toggle Edit Mode", [=]() {
        toggleEditMode();
    });
    menu.addItem("Change Effect Image..", [=]() {
        FileChooser imgChooser ("Select Effect Image..",
                               File::getSpecialLocation (File::userHomeDirectory),
                               "*.jpg;*.png;*.gif");

        if (imgChooser.browseForFileToOpen())
        {
            image = ImageFileFormat::loadFrom(imgChooser.getResult());
        }
    });

    editMenu.addItem("Add Input Port", [=](){addPort(EVT->getDefaultBus(), true); resized(); });
    editMenu.addItem("Add Output Port", [=](){addPort(EVT->getDefaultBus(), false); resized(); });
    editMenu.addItem("Toggle Edit Mode", [=]() {
        toggleEditMode();
    });

    Font titleFont(20, Font::FontStyleFlags::bold);
    title.setFont(titleFont);
    title.setText("New Empty Effect", dontSendNotification);
    title.setBounds(30,30,200, title.getFont().getHeight());
    title.setColour(title.textColourId, Colours::black);
    title.setEditable(true);
    addAndMakeVisible(title);

    // Make edit mode by default
    setEditMode(true);
}

GUIEffect::~GUIEffect()
{

}

void GUIEffect::insertEffectGroup() {

}

void GUIEffect::insertEffect() {

}

// Processor hasEditor? What to do if processor is a predefined plugin
void GUIEffect::setProcessor(AudioProcessor *processor) {
    // Set up ports based on processor buses
    int numInputBuses = processor->getBusCount(true );
    int numBuses = numInputBuses + processor->getBusCount(false);
    for (int i = 0; i < numBuses; i++){
        bool isInput = (i < numInputBuses) ? true : false;
        // Get bus from processor
        auto bus = isInput ? processor->getBus(true, i) :
                processor->getBus(false, i - numInputBuses);
        // Check channel number - if 0 ignore
        if (bus->getNumberOfChannels() == 0)
            continue;
        // Create port - giving audiochannelset info and isInput bool
        addPort(bus, isInput);
    }
    title.setText(processor->getName(), dontSendNotification);

    // Setup parameters
    parameters = &processor->getParameterTree();
    setParameters(parameters);

    setEditMode(false);
    individual = true;

    // Update
    resized();
    repaint();
}

void GUIEffect::paint (Graphics& g)
{
    // Draw outline rectangle
    g.setColour(Colours::black);
    Rectangle<float> outline(10,10,getWidth()-20, getHeight()-20);
    g.drawRoundedRectangle(outline, 10, 3);

    // Hover rectangle
    g.setColour(Colours::blue);
    Path hoverRectangle;
    hoverRectangle.addRoundedRectangle(0, 0, getWidth(), getHeight(), 10, 10);
    PathStrokeType strokeType(3);

    if (hoverMode) {
        float thiccness[] = {5, 7};
        strokeType.createDashedStroke(hoverRectangle, hoverRectangle, thiccness, 2);
    } else if (selectMode)
        strokeType.createStrokedPath(hoverRectangle, hoverRectangle);

    if (selectMode || hoverMode)
        g.strokePath(hoverRectangle, strokeType);

    g.setColour(Colours::whitesmoke);
    if (!editMode) {
        g.setOpacity(1.f);
    } else {
        g.setOpacity(0.3f);
    }
    if (image.isNull())
        g.fillRoundedRectangle(outline, 10);
    else
        g.drawImage(image, outline);
}

void GUIEffect::resized()
{
    inputPortPos = inputPortStartPos;
    outputPortPos = outputPortStartPos;
    for (auto p : inputPorts){
        p->setCentrePosition(portIncrement, inputPortPos);
        inputPortPos += portIncrement;
    }
    for (auto p : outputPorts){
        p->setCentrePosition(getWidth() - portIncrement, outputPortPos);
        outputPortPos += portIncrement;
    }

    title.setBounds(30,30,200, title.getFont().getHeight());
}

void GUIEffect::mouseDown(const MouseEvent &event) {
    setAlwaysOnTop(true);
    if (event.mods.isLeftButtonDown()) {
        dragData.previousParent = getParentComponent();
        dragData.previousPos = getPosition();
        dragger.startDraggingComponent(this, event);
    } else if (event.mods.isRightButtonDown())
        getParentComponent()->mouseDown(event);
}

void GUIEffect::mouseDrag(const MouseEvent &event) {

/*    constrainer.setBoundsForComponent(this,
            Rectangle<int>(newX, newY, getWidth(), getHeight()),
                    false, false, false ,false);*/
    if (event.eventComponent == this) {
        dragger.dragComponent(this, event, &constrainer);

        // Manual constraint
        auto newX = jlimit<int>(0, getParentWidth() - getWidth(), getX());
        auto newY = jlimit<int>(0, getParentHeight() - getHeight(), getY());

        if (newX != getX() || newY != getY())
            if (event.x<-(getWidth() / 2) || event.y<-(getHeight() / 2) ||
                                                     event.x>(getWidth() * 3 / 2) || event.y>(getHeight() * 3 / 2)) {
                auto newPos = dragDetachFromParentComponent();
                newX = newPos.x;
                newY = newPos.y;
            }

        setTopLeftPosition(newX, newY);
    }
    getParentComponent()->mouseDrag(event);
}

void GUIEffect::mouseUp(const MouseEvent &event) {
    setAlwaysOnTop(false);
    getParentComponent()->mouseUp(event);
}

void GUIEffect::mouseEnter(const MouseEvent &event) {
    if (!dynamic_cast<GUIEffect*>(event.eventComponent)->hoverMode)
        hoverMode = true;
    repaint();

    getParentComponent()->mouseEnter(event);
    Component::mouseEnter(event);
}

void GUIEffect::mouseExit(const MouseEvent &event) {
    if (dynamic_cast<GUIEffect*>(event.eventComponent)->hoverMode)
        hoverMode = false;
    repaint();

    getParentComponent()->mouseExit(event);
    Component::mouseExit(event);
}

void GUIEffect::moved() {
    for (auto i : inputPorts)
        i->moved();
    for (auto i : outputPorts)
        i->moved();
    Component::moved();
}

void GUIEffect::visibilityChanged() {
    // Set parent (Wrapper) visibility
    if (getParentComponent())
        getParentComponent()->setVisible(this->isVisible());
}

Point<int> GUIEffect::dragDetachFromParentComponent() {
    auto newPos = getPosition() + getParentComponent()->getPosition();
    auto parentParent = getParentComponent()->getParentComponent();
    getParentComponent()->removeChildComponent(this);
    parentParent->addAndMakeVisible(this);

    for (auto c : getChildren())
        std::cout << "Child position: " << c->getPosition().toString() << newLine;

    return newPos;
}

void GUIEffect::childrenChanged() {
    Component::childrenChanged();
}

void GUIEffect::parentHierarchyChanged() {
    // Children of parents who receive the change signal should ignore it.
    if (currentParent == getParentComponent())
        return;

    if (getParentComponent() == nullptr){
        setTopLeftPosition(getPosition() + currentParent->getPosition());
        currentParent = nullptr;
    } else {

        if (hasBeenInitialised) {
            Component* parent = getParentComponent();
            while (parent != getTopLevelComponent()) {
                setTopLeftPosition(getPosition() - parent->getPosition());
                parent = parent->getParentComponent();
            }

        }
        currentParent = getParentComponent();
    }
    Component::parentHierarchyChanged();
}

/**
 * Get the port at the given location, if there is one
 * @param pos relative to this component (no conversion needed here)
 * @return nullptr if no match, AudioPort* if found
 */
AudioPort *GUIEffect::checkPort(Point<int> pos) {
    for (auto p : inputPorts)
        if (p->getBoundsInParent().contains(pos)) {
            if (p->internalPort.contains(getLocalPoint(this, pos)))
                std::cout << "Internal port connect" << newLine;
            return p;
        }

    for (auto p : outputPorts)
        if (p->getBoundsInParent().contains(pos)) {
            if (p->internalPort.contains(getLocalPoint(this, pos)))
                std::cout << "Internal port connect" << newLine;
            return p;
        }

    return nullptr;
}

void GUIEffect::setEditMode(bool isEditMode) {
    if (isIndividual() || editMode == isEditMode)
        return;
    // Turn on edit mode
    if (isEditMode) {
        for (auto c : getChildren()) {
            c->setAlwaysOnTop(true);
            if (!dynamic_cast<AudioPort*>(c))
                c->setInterceptsMouseClicks(true, true);

        }
        title.setMouseCursor(MouseCursor::IBeamCursor);
        title.setInterceptsMouseClicks(true, true);
    }
    // Turn off edit mode
    else if (!isEditMode) {
        for (auto c : getChildren()) {
            c->setAlwaysOnTop(false);
            if (!dynamic_cast<AudioPort*>(c))
                c->setInterceptsMouseClicks(false, false);
        }
        title.setMouseCursor(getMouseCursor());
        title.setInterceptsMouseClicks(false,false);
    }

    editMode = isEditMode;
    repaint();
}

void GUIEffect::setParameters(const AudioProcessorParameterGroup *group) {
    // Individual
    for (auto param : group->getParameters(false)) {
        addParameter(param);
    }
    for (auto c : getChildren())
        std::cout << c->getName() << newLine;
}

void GUIEffect::addParameter(AudioProcessorParameter *param) {
    if (param->isBoolean()) {
        // add bool parameter
        ToggleButton* button = new ToggleButton();

        ButtonListener* listener = new ButtonListener(param);
        button->addListener(listener);
        button->setName("Button");
        button->setBounds(20,50, 100, 40);
    } else if (param->isDiscrete() && !param->getAllValueStrings().isEmpty()) {
        // add combo parameter
        ComboBox* comboBox = new ComboBox();

        for (auto s : param->getAllValueStrings())
            comboBox->addItem(s, param->getAllValueStrings().indexOf(s));

        ComboListener* listener = new ComboListener(param);

        comboBox->addListener(listener);
        comboBox->setName("ComboBox");
        comboBox->setBounds(20, 50, 100, 40);

        addAndMakeVisible(comboBox);
    } else if (param->isDiscrete()) {
        // add int parameter
        Slider* slider = new Slider();
        auto paramRange = dynamic_cast<RangedAudioParameter*>(param)->getNormalisableRange();

        slider->setNormalisableRange(NormalisableRange<double>(paramRange.start, paramRange.end,
                                                               1, paramRange.skew));
        SliderListener* listener = new SliderListener(param);
        slider->addListener(listener);
        slider->setName("Slider");
        slider->setBounds(20, 50, 100, 40);

        addAndMakeVisible(slider);
    } else {
        // add float parameter
        Slider* slider = new Slider();
        auto paramRange = dynamic_cast<RangedAudioParameter*>(param)->getNormalisableRange();

        slider->setNormalisableRange(NormalisableRange<double>(paramRange.start, paramRange.end,
                                                               paramRange.interval, paramRange.skew));
        SliderListener* listener = new SliderListener(param);
        slider->addListener(listener);
        slider->setName("Slider");
        slider->setBounds(20, 50, 200, 40);

        addAndMakeVisible(slider);
    }
}

//==============================================================================
// InternalConnectionPort methods




// AudioPort methods

AudioPort::AudioPort(bool isInput) : ConnectionPort() {
    if (isInput){
        hoverBox = Rectangle<int>(0,0,60,60);
        outline = Rectangle<int>(20, 20, 20, 20);
        centrePoint = Point<int>(30,30);
        setBounds(0,0,90, 60);
    } else {
        hoverBox = Rectangle<int>(30,0,60,60);
        outline = Rectangle<int>(50, 20, 20, 20);
        centrePoint = Point<int>(60,30);
        setBounds(0,0,90, 60);
    }

    addChildComponent(internalPort);
    if (isInput)
        internalPort.setCentrePosition(centrePoint + Point<int>(40,0));
    else
        internalPort.setCentrePosition(centrePoint + Point<int>(-40,0));

    this->isInput = isInput;
}

GUIEffect *AudioPort::getParent() {
    return dynamic_cast<GUIEffect*>(getParentComponent());
}


// ==============================================================================
// Resizer methods

void Resizer::mouseDrag(const MouseEvent &event) {
    dragger.dragComponent(this, event, nullptr);

    getParentComponent()->setSize(startPos.x + event.getDistanceFromDragStartX(),
                                  startPos.y + event.getDistanceFromDragStartY());
    Component::mouseDrag(event);
}


//==============================================================================
// EffectVT static member variable

AudioProcessorGraph* EffectVT::graph = nullptr;

//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="GUIEffect" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


void ConnectionLine::componentMovedOrResized(Component &component, bool wasMoved, bool wasResized) {
    if (inPort->getParentComponent() == &component){
        line.setStart(component.getPosition() + inPort->getPosition() + inPort->centrePoint);
    } else if (outPort->getParentComponent() == &component) {
        line.setEnd(component.getPosition() + outPort->getPosition() + outPort->centrePoint);
    }
    repaint();
}

bool ConnectionLine::hitTest(int x, int y) {
    auto d1 = line.getStart().getDistanceFrom(Point<int>(x,y));
    auto d2 = line.getEnd().getDistanceFrom(Point<int>(x,y));
    auto d = d1 + d2 - line.getLength();

    if (d < 7) {
        return true;
    } else {
        return false;
    }
}



void ConnectionPort::mouseDown(const MouseEvent &event) {
    LineComponent::getDragLine()->mouseDown(event);

}

void ConnectionPort::mouseDrag(const MouseEvent &event) {
    LineComponent::getDragLine()->mouseDrag(event);
}

void ConnectionPort::mouseUp(const MouseEvent &event) {
    LineComponent::getDragLine()->mouseUp(event);
}

void ConnectionPort::paint(Graphics &g) {
    g.setColour(Colours::black);
    //rectangle.setPosition(10,10);
    g.drawRect(outline,2);

    // Hover rectangle
    g.setColour(Colours::blue);
    Path drawPath;
    drawPath.addRoundedRectangle(hoverBox, 10, 10);
    PathStrokeType strokeType(3);

    if (hoverMode) {
        float thiccness[] = {2, 3};
        strokeType.createDashedStroke(drawPath, drawPath, thiccness, 2);
        g.strokePath(drawPath, strokeType);
    }
}

GUIEffect *InternalConnectionPort::getParent() {
    return dynamic_cast<GUIEffect*>(getParentComponent()->getParentComponent());
}
