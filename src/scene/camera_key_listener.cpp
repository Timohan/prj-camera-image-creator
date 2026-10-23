#include "camera_key_listener.h"

CameraKeyListener::CameraKeyListener(TgItem2d *parent) :
    TgItem2d(parent)
{
}

void CameraKeyListener::connectOnCameraSelected(std::function<void(int index)> cameraSelected)
{
    m_cameraSelected = cameraSelected;
}

void CameraKeyListener::connectOnToggleDirectionLine(std::function<void()> toggleDirectionLine)
{
    m_toggleDirectionLine = toggleDirectionLine;
}

void CameraKeyListener::connectOnToggleCameraObjects(std::function<void()> toggleCameraObjects)
{
    m_toggleCameraObjects = toggleCameraObjects;
}

TgEventResult CameraKeyListener::handleEvent(TgEventData *eventData, const TgWindowInfo *windowInfo)
{
    if (eventData->m_type == TgEventType::EventTypeCharacterCallback
        && eventData->m_event.m_keyEvent.m_pressReleaseKey == TgPressReleaseKey::PressReleaseKey_NormalKey
        && eventData->m_event.m_keyEvent.m_key >= '0'
        && eventData->m_event.m_keyEvent.m_key <= '6') {
        if (m_cameraSelected) {
            m_cameraSelected(static_cast<int>(eventData->m_event.m_keyEvent.m_key - '0'));
        }
        return TgEventResult::EventResultCompleted;
    }
    if (eventData->m_type == TgEventType::EventTypeCharacterCallback
        && eventData->m_event.m_keyEvent.m_pressReleaseKey == TgPressReleaseKey::PressReleaseKey_NormalKey
        && eventData->m_event.m_keyEvent.m_key == 'l') {
        if (m_toggleDirectionLine) {
            m_toggleDirectionLine();
        }
        return TgEventResult::EventResultCompleted;
    }
    if (eventData->m_type == TgEventType::EventTypeCharacterCallback
        && eventData->m_event.m_keyEvent.m_pressReleaseKey == TgPressReleaseKey::PressReleaseKey_NormalKey
        && eventData->m_event.m_keyEvent.m_key == 'c') {
        if (m_toggleCameraObjects) {
            m_toggleCameraObjects();
        }
        return TgEventResult::EventResultCompleted;
    }
    return TgItem2d::handleEvent(eventData, windowInfo);
}
