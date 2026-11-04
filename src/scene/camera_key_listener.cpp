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

void CameraKeyListener::connectOnSaveImages(std::function<void()> saveImages)
{
    m_saveImages = saveImages;
}

void CameraKeyListener::connectOnToggleHideUnseen(std::function<void()> toggleHideUnseen)
{
    m_toggleHideUnseen = toggleHideUnseen;
}

void CameraKeyListener::connectOnTurnStart(std::function<void(int direction)> turnStart)
{
    m_turnStart = turnStart;
}

void CameraKeyListener::connectOnTurnStop(std::function<void(int direction)> turnStop)
{
    m_turnStop = turnStop;
}

void CameraKeyListener::connectOnPitchStart(std::function<void(int direction)> pitchStart)
{
    m_pitchStart = pitchStart;
}

void CameraKeyListener::connectOnPitchStop(std::function<void(int direction)> pitchStop)
{
    m_pitchStop = pitchStop;
}

void CameraKeyListener::connectOnMove(std::function<void(float steps)> move)
{
    m_move = move;
}

TgEventResult CameraKeyListener::handleEvent(TgEventData *eventData, const TgWindowInfo *windowInfo)
{
    if (eventData->m_type == TgEventType::EventTypeMouseScrollMove) {
        // wheel forward is positive, many wheel steps can be summed into one event
        if (m_move && eventData->m_event.m_mouseEvent.m_scroll_move_y != 0.0) {
            m_move(static_cast<float>(eventData->m_event.m_mouseEvent.m_scroll_move_y));
        }
        return TgEventResult::EventResultCompleted;
    }
    if ((eventData->m_type == TgEventType::EventTypeKeyPress || eventData->m_type == TgEventType::EventTypeKeyRelease)
        && (eventData->m_event.m_keyEvent.m_pressReleaseKey == TgPressReleaseKey::PressReleaseKey_Key_Left
            || eventData->m_event.m_keyEvent.m_pressReleaseKey == TgPressReleaseKey::PressReleaseKey_Key_Right)) {
        const int direction = eventData->m_event.m_keyEvent.m_pressReleaseKey == TgPressReleaseKey::PressReleaseKey_Key_Left ? -1 : 1;
        if (eventData->m_type == TgEventType::EventTypeKeyPress && m_turnStart) {
            m_turnStart(direction);
        } else if (eventData->m_type == TgEventType::EventTypeKeyRelease && m_turnStop) {
            m_turnStop(direction);
        }
        return TgEventResult::EventResultCompleted;
    }
    if ((eventData->m_type == TgEventType::EventTypeKeyPress || eventData->m_type == TgEventType::EventTypeKeyRelease)
        && (eventData->m_event.m_keyEvent.m_pressReleaseKey == TgPressReleaseKey::PressReleaseKey_Key_Up
            || eventData->m_event.m_keyEvent.m_pressReleaseKey == TgPressReleaseKey::PressReleaseKey_Key_Down)) {
        const int direction = eventData->m_event.m_keyEvent.m_pressReleaseKey == TgPressReleaseKey::PressReleaseKey_Key_Down ? -1 : 1;
        if (eventData->m_type == TgEventType::EventTypeKeyPress && m_pitchStart) {
            m_pitchStart(direction);
        } else if (eventData->m_type == TgEventType::EventTypeKeyRelease && m_pitchStop) {
            m_pitchStop(direction);
        }
        return TgEventResult::EventResultCompleted;
    }
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
    if (eventData->m_type == TgEventType::EventTypeCharacterCallback
        && eventData->m_event.m_keyEvent.m_pressReleaseKey == TgPressReleaseKey::PressReleaseKey_NormalKey
        && eventData->m_event.m_keyEvent.m_key == 's') {
        if (m_saveImages) {
            m_saveImages();
        }
        return TgEventResult::EventResultCompleted;
    }
    if (eventData->m_type == TgEventType::EventTypeCharacterCallback
        && eventData->m_event.m_keyEvent.m_pressReleaseKey == TgPressReleaseKey::PressReleaseKey_NormalKey
        && eventData->m_event.m_keyEvent.m_key == 'h') {
        if (m_toggleHideUnseen) {
            m_toggleHideUnseen();
        }
        return TgEventResult::EventResultCompleted;
    }
    return TgItem2d::handleEvent(eventData, windowInfo);
}
